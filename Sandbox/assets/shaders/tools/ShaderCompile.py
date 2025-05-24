import os
import sys
import subprocess
import re
from pathlib import Path
from typing import Dict, List, Tuple


def log_error(message: str):
    print(f"\033[91m[Error] {message}\033[0m", file=sys.stderr)  # red format


def log_warning(message: str):
    print(f"\033[93m[Warning] {message}\033[0m", file=sys.stderr)  # yellow format


class ShaderCompiler:
    def __init__(self, slangc_path: str, output_dir: str):
        self.slangc_path = slangc_path
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def detect_entry_points(self, shader_path: Path) -> Dict[str, bool]:
        """Detect entry points like vsMain, psMain, computeMain in the shader source"""
        try:
            content = shader_path.read_text(encoding="utf-8")
            return {
                "vsMain": bool(re.search(r'\bvsMain\b', content)),
                "psMain": bool(re.search(r'\bpsMain\b', content)),
                "computeMain": bool(re.search(r'\bcomputeMain\b', content))
            }
        except Exception as e:
            log_error(f"Failed to read {shader_path}: {e}")
            return {}

    def compile_shader(self, shader_path: Path, profile: str, entry: str, output_file: Path) -> bool:
        """Invoke slangc compiler for given shader entry point"""
        # slangc.exe ../slang/Texture.slang -profile glsl_450 -target glsl -o ../generated/glsl/Texture.vert -entry vsMain
        cmd = [
            self.slangc_path,
            str(shader_path),
            "-profile", profile,
            "-target", "glsl",
            "-entry", entry,
            "-o", str(output_file)
        ]

        try:
            result = subprocess.run(
                cmd, check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            if result.stderr:
                log_warning(result.stderr)
            return True
        except subprocess.CalledProcessError as e:
            log_error(f"Compilation failed for entry '{entry}':\n"
                      f"    Command: {' '.join(cmd)}\n"
                      f"    Exit Code: {e.returncode}\n"
                      f"    Error Output:\n{e.stderr}")
            return False
        except Exception as e:
            log_error(f"Unexpected error while compiling {entry}: {e}")
            return False

    def compile_and_merge_shader(self, shader_path: Path):
        """Compile all valid entry points and merge into a single .glsl file"""
        shader_name = shader_path.stem
        entry_points = self.detect_entry_points(shader_path)
        if not entry_points:
            log_warning(f"No valid entry points found in {shader_path}")
            return

        compiled_parts: List[Tuple[str, str]] = []
        temp_files: List[Path] = []

        entries = {
            "vsMain": ("vertex", f"{shader_name}.vert"),
            "psMain": ("fragment", f"{shader_name}.frag"),
            "computeMain": ("compute", f"{shader_name}.comp")
        }

        for entry, (shader_type, filename) in entries.items():
            if not entry_points.get(entry):
                continue
            output_file = self.output_dir / filename
            if self.compile_shader(shader_path, "glsl_450", entry, output_file):
                compiled_code = output_file.read_text(encoding="utf-8")
                compiled_parts.append((f"#type {shader_type}", compiled_code))
                temp_files.append(output_file)

        # Write merged .glsl file
        if compiled_parts:
            merged_path = self.output_dir / f"{shader_name}.glsl"
            with merged_path.open("w", encoding="utf-8") as f:
                for tag, code in compiled_parts:
                    f.write(f"{tag}\n{code}\n")

        # Cleanup
        for temp in temp_files:
            try:
                temp.unlink()
            except Exception as e:
                log_warning(f"Could not delete temp file {temp}: {e}")


def main():
    if len(sys.argv) != 4:
        log_error("Usage: python ShaderCompile.py <slangc_path> <output_dir> <shader1>")
        sys.exit(1)

    slangc_path = sys.argv[1]
    output_dir = sys.argv[2]
    shader_file = Path(sys.argv[3])

    compiler = ShaderCompiler(slangc_path, output_dir)
    if shader_file.exists():
        compiler.compile_and_merge_shader(shader_file)
    else:
        log_error(f"Shader file not found: {shader_file}")


if __name__ == "__main__":
    main()
