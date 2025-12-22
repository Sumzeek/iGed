import os
import sys
import subprocess
import re
import argparse
from pathlib import Path
from typing import Dict, List, Tuple
import json
import shutil


def log_error(message: str):
    print(f"\033[91m[Error] {message}\033[0m", file=sys.stderr)  # red format


def log_warning(message: str):
    print(f"\033[93m[Warning] {message}\033[0m", file=sys.stderr)  # yellow format


class SlangCompiler:
    def __init__(self, slangc_path: str, spirvcross_path: str, output_dir: Path, copy_dir: Path,
                 target_platform: str = "hlsl"):
        self.slangc_path = slangc_path
        self.spirvcross_path = spirvcross_path
        self.output_dir = output_dir / target_platform
        self.copy_dir = copy_dir / target_platform

        # Ensure directory are exists
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.copy_dir.mkdir(parents=True, exist_ok=True)

        # Validate and set target format
        if target_platform.lower() not in ["hlsl", "spv", "glsl"]:
            raise ValueError(f"Unsupported target format: {target_platform}.")

        self.target_platform = target_platform.lower()

        # Configure format-specific settings
        if self.target_platform == 'hlsl':  # hlsl
            self.target = "hlsl"
            self.profile = "sm_6_6"
            self.extension = "hlsl"
        elif self.target_platform == "spv":  # spir-v
            self.target = "spirv"
            self.profile = "spirv_1_5"
            self.extension = "spv"
        elif self.target_platform == "glsl":  # glsl
            self.target = "glsl"
            self.profile = "glsl_460"
            self.extension = "glsl"

        self.shader_suffixs = {
            "vertex": "_vs",
            "tesscontrol": "_tcs",
            "tesseval": "_tes",
            "geometry": "_gs",
            "fragment": "_fs",
            "compute": "_cs",

            "hull": "_hs",
            "domain": "_ds",
            "pixel": "_ps",

            "amplification": "_as",
            "task": "_ts",
            "mesh": "_ms"
        }

    def detect_entries(self, slang_file: Path) -> Dict[str, str]:
        """
        Detect shader entry points by collecting all [shader("...")] blocks and their associated function names.
        Handles multi-line annotations.
        """
        try:
            content = slang_file.read_text(encoding="utf-8")

            # Match a [shader("xxx")] block and following lines up to function definition
            # We extract shader type and function name
            pattern = re.compile(
                r"""
                (
                    (?:\s*\[[^\]]+\]\s*)+      # One or more annotation lines
                )
                \s*
                (?:\w+\s+)+                  # Return type (e.g., TessControlOutput)
                (\w+)\s*\(                   # Function name followed by '('
                """,
                re.VERBOSE
            )

            entry_points = {}
            for full_annotations, func_name in pattern.findall(content):
                shader_match = re.search(r'\[shader\("(\w+)"\)\]', full_annotations)
                if shader_match:
                    shader_type = shader_match.group(1).lower()
                    entry_points[shader_type] = func_name

            return entry_points

        except Exception as e:
            log_error(f"Failed to read {slang_file}: {e}")
            return {}

    def compile_stage(self, slang_file: Path, stage: str, entry: str) -> bool:
        """Invoke slangc compiler for given shader entry point."""
        output_file_path = self.output_dir / f"{slang_file.stem}{self.shader_suffixs[stage]}.{self.extension}"
        cmd = [
            self.slangc_path,
            str(slang_file),
            "-target", self.target,
            "-profile", self.profile,
            "-stage", stage,
            "-entry", entry,
            "-o", str(output_file_path)
        ]

        try:
            result = subprocess.run(
                cmd, check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            if result.stderr:
                log_warning(f"Warnings from slangc:\n{result.stderr.strip()}")

            print(f"    Successfully compiled: {output_file_path}")
            return True
        except subprocess.CalledProcessError as e:
            log_error(
                f"Compilation failed for entry '{entry}'\n"
                f"Command: {' '.join(cmd)}\n"
                f"Exit Code: {e.returncode}\n"
                f"stderr:\n{e.stderr.strip()}"
            )
            return False
        except Exception as ex:
            log_error(f"Unexpected error compiling '{entry}': {ex}")
            return

    def compile_shader(self, slang_file: Path):
        """Compile all valid entry points to separate shader files"""
        entries = self.detect_entries(slang_file)
        if not entries:
            # log_warning(f"No valid entries found in {slang_file}")
            return

        compiled_count = 0
        copied_files = []

        for (stage, entry) in entries.items():
            if self.compile_stage(slang_file, stage, entry):
                compiled_count += 1
                output_file_path = self.output_dir / f"{slang_file.stem}{self.shader_suffixs[stage]}.{self.extension}"
                copied_files.append(output_file_path)
            else:
                log_error(f"    Failed to compile {stage} shader: {entry}")

        if compiled_count > 0:
            print(f"    Compilation completed: {compiled_count} shader(s) generated in {self.output_dir}")

            # Write entry map JSON
            shader_name = slang_file.stem
            entry_map = {
                stage: f"{shader_name}{self.shader_suffixs[stage]}.{self.extension}"
                for stage in entries.keys()
            }

            json_path = self.output_dir / f"{shader_name}.json"
            try:
                with json_path.open("w", encoding="utf-8") as f:
                    json.dump(entry_map, f, indent=4)
                copied_files.append(json_path)
                print(f"    Entry map written to: {json_path}")
            except Exception as e:
                log_error(f"Failed to write entry map JSON: {e}")

            # Copy compiled file to runtime directory
            self.copy_dir.mkdir(parents=True, exist_ok=True)
            for file_path in copied_files:
                try:
                    shutil.copy2(file_path, self.copy_dir)
                    print(f"    Copied {file_path.name} to {self.copy_dir}")
                except Exception as e:
                    log_error(f"Failed to copy {file_path} to {self.copy_dir}: {e}")

    def convert_stage(self, spv_file: Path):
        glsl_file = self.output_dir / f"{spv_file.stem}.{self.extension}"

        cmd = [
            self.spirvcross_path,
            str(spv_file),
            "--version", "460",
            "--output", str(glsl_file)
        ]

        try:
            result = subprocess.run(
                cmd,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            if result.stderr.strip():
                log_warning(f"SPIRV-Cross stderr:\n{result.stderr.strip()}")

            print(f"    Successfully converted: {glsl_file}")
            return True
        except subprocess.CalledProcessError as e:
            log_error(
                f"SPIRV-Cross conversion failed:\n"
                f"  Input: {spv_file}\n"
                f"  Command: {' '.join(cmd)}\n"
                f"  Exit Code: {e.returncode}\n"
                f"  Error Output:\n{e.stderr.strip()}"
            )
            return False
        except Exception as ex:
            log_error(f"Unexpected error converting '{spv_file}' to GLSL: {ex}")
            return False

    def convert_shader(self, slang_file: Path) -> bool:
        """Compile all valid entry points to separate shader files"""
        entries = self.detect_entries(slang_file)
        if not entries:
            # log_warning(f"No valid entries found in {slang_file}")
            return

        converted_count = 0
        copied_files = []

        for (stage, entry) in entries.items():
            shader_name = slang_file.stem
            input_filename = f"{shader_name}{self.shader_suffixs[stage]}.spv"
            input_file_path = self.output_dir.parent / "spv" / input_filename
            output_filename = f"{shader_name}{self.shader_suffixs[stage]}.glsl"
            output_file_path = self.output_dir / output_filename

            if self.convert_stage(input_file_path):
                converted_count += 1
                copied_files.append(output_file_path)
            else:
                log_error(f"    Failed converte {input_file_path} to {output_file_path}")

        if converted_count > 0:
            print(f"    Convert completed: {converted_count} shader(s) generated in {self.output_dir}")

            # Write entry map JSON
            entry_map = {
                stage: f"{shader_name}{self.shader_suffixs[stage]}.{self.extension}"
                for stage in entries.keys()
            }

            json_path = self.output_dir / f"{shader_name}.json"
            try:
                with json_path.open("w", encoding="utf-8") as f:
                    json.dump(entry_map, f, indent=4)
                print(f"    Entry map written to: {json_path}")
                copied_files.append(json_path)
            except Exception as e:
                log_error(f"Failed to write entry map JSON: {e}")

            # Copy compiled file to runtime directory
            self.copy_dir.mkdir(parents=True, exist_ok=True)
            for file_path in copied_files:
                try:
                    shutil.copy2(file_path, self.copy_dir)
                    print(f"    Copied {file_path.name} to {self.copy_dir}")
                except Exception as e:
                    log_error(f"Failed to copy {file_path} to {self.copy_dir}: {e}")


def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description="Compile Slang shaders to target format language",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python ShaderCompile.py slangc.exe shader.slang output_dir
    python ShaderCompile.py slangc.exe shader.slang output_dir -target hlsl
    python ShaderCompile.py slangc.exe shader.slang output_dir --target spv
          """
    )

    parser.add_argument("slangc_path", help="Path to slangc compiler executable")
    parser.add_argument("spirvcross_path", help="Path to spirv-cross compiler executable")
    parser.add_argument("slang_file", help="Input slang shader file to compile")
    parser.add_argument("output_dir", help="Directory where compiled shaders are placed")
    parser.add_argument("copy_dir", help="Directory where compiled shaders are copied to after compilation")
    parser.add_argument(
        "-target", "--target",
        choices=["hlsl", "spv", "glsl"],
        default="hlsl",
        help="Target shader format: hlsl, spv (SPIR-V), or glsl (default: hlsl)"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output"
    )

    return parser.parse_args()


def main():
    try:
        args = parse_arguments()

        slangc_path = args.slangc_path
        spirvcross_path = args.spirvcross_path
        slang_file = Path(args.slang_file)
        output_dir = Path(args.output_dir)
        copy_dir = Path(args.copy_dir)
        target_platform = args.target

        if args.verbose:
            print(f"Slang compiler: {slangc_path}")
            print(f"Shader file: {slang_file}")
            print(f"Output directory: {output_dir}")
            print(f"Copy directory: {copy_dir}")
            print(f"Target format: {target_platform.upper()}")

        if not slang_file.exists():
            log_error(f"Shader file not found: {slang_file}")
            sys.exit(1)

        compiler = SlangCompiler(slangc_path, spirvcross_path, output_dir, copy_dir, target_platform)
        if target_platform == "glsl":
            print(f"Convert shader to {target_platform.upper()} format: {slang_file}")
            compiler.convert_shader(slang_file)
        else:
            print(f"Compiling shader to {target_platform.upper()} format: {slang_file}")
            compiler.compile_shader(slang_file)

    except ValueError as e:
        log_error(str(e))
        sys.exit(1)
    except KeyboardInterrupt:
        log_error("Compilation interrupted by user")
        sys.exit(130)
    except Exception as e:
        log_error(f"Unexpected error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
