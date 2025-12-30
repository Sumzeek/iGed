import os
import sys
import subprocess
import re
import argparse
from pathlib import Path
from typing import Dict, List, Tuple
import shutil
import hashlib


# ------------------------------------------------------------
# Logging
# ------------------------------------------------------------

def log_error(message: str):
    print(f"\033[91m[Error] {message}\033[0m", file=sys.stderr)


def log_warning(message: str):
    print(f"\033[93m[Warning] {message}\033[0m", file=sys.stderr)


# ------------------------------------------------------------
# Dependency parsing
# ------------------------------------------------------------

IMPORT_PATTERN = re.compile(
    r'^\s*(?:import|#include)\s+[<"]?([^">;]+)',
    re.MULTILINE
)


def collect_dependencies(file: Path, visited=None) -> List[Path]:
    if visited is None:
        visited = set()

    if file in visited or not file.exists():
        return []

    visited.add(file)
    deps = []

    try:
        content = file.read_text(encoding="utf-8")
    except Exception:
        return deps

    for match in IMPORT_PATTERN.findall(content):
        dep = file.parent / match
        if dep.suffix == "":
            dep = dep.with_suffix(".slang")

        if dep.exists():
            deps.append(dep)
            deps.extend(collect_dependencies(dep, visited))

    return deps


# ------------------------------------------------------------
# Hash utilities
# ------------------------------------------------------------

def hash_files(files: List[Path], extra: str) -> str:
    h = hashlib.sha256()

    for f in sorted(files, key=lambda p: str(p)):
        h.update(f.read_bytes())

    h.update(extra.encode("utf-8"))
    return h.hexdigest()


# ------------------------------------------------------------
# Slang Compiler
# ------------------------------------------------------------

class SlangCompiler:
    def __init__(self, slangc_path: str, output_dir: Path, copy_dir: Path, target_platform: str):
        self.slangc_path = slangc_path
        self.output_dir = output_dir / target_platform
        self.copy_dir = copy_dir / target_platform
        self.target_platform = target_platform.lower()

        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.copy_dir.mkdir(parents=True, exist_ok=True)

        if self.target_platform == "hlsl":
            self.target = "hlsl"
            self.profile = "sm_6_6"
            self.extension = "hlsl"
        elif self.target_platform == "spv":
            self.target = "spirv"
            self.profile = "spirv_1_5"
            self.extension = "spv"
        elif self.target_platform == "glsl":
            self.target = "glsl"
            self.profile = "glsl_460"
            self.extension = "glsl"
        else:
            raise ValueError("Unsupported target platform")

        self.shader_suffixs = {
            "vertex": "_vs",
            "fragment": "_fs",
            "geometry": "_gs",
            "tesscontrol": "_tcs",
            "tesseval": "_tes",
            "hull": "_hs",
            "domain": "_ds",

            "compute": "_cs",
            "raytracing": "_rt",
        }

    # --------------------------------------------------------

    def detect_entries(self, slang_file: Path) -> Dict[str, str]:
        content = slang_file.read_text(encoding="utf-8")

        pattern = re.compile(
            r'((?:\s*\[[^\]]+\]\s*)+)(?:\w+\s+)+(\w+)\s*\(',
            re.VERBOSE
        )

        entries = {}
        for full_anno, func in pattern.findall(content):
            m = re.search(r'\[shader\("(\w+)"\)\]', full_anno)
            if m:
                entries[m.group(1).lower()] = func

        return entries

    # --------------------------------------------------------

    def should_recompile(self, slang_file: Path) -> Tuple[bool, str]:
        deps = [slang_file] + collect_dependencies(slang_file)

        extra = f"{self.target_platform}|{self.profile}|{self.target}"
        current_hash = hash_files(deps, extra)

        hash_file = self.output_dir / f"{slang_file.stem}.shaderhash"

        if hash_file.exists():
            if hash_file.read_text().strip() == current_hash:
                return False, current_hash

        return True, current_hash

    # --------------------------------------------------------

    def compile_stage(self, slang_file: Path, stage: str, entry: str) -> Path:
        import_dir = slang_file.parent / "modules"
        out = self.output_dir / f"{slang_file.stem}{self.shader_suffixs[stage]}.{self.extension}"
        reflection_out = self.output_dir / f"{slang_file.stem}{self.shader_suffixs[stage]}_reflection.json"
        cmd = [
            self.slangc_path,
            str(slang_file),
            "-target", self.target,
            "-profile", self.profile,
            "-stage", stage,
            "-entry", entry,
            "-I", import_dir,
            "-o", str(out),
            "-reflection-json", str(reflection_out)
        ]

        subprocess.run(cmd, check=True)
        return out

    # --------------------------------------------------------

    def compile_shader(self, slang_file: Path):
        need, shader_hash = self.should_recompile(slang_file)

        if not need:
            print(f"    Skip (unchanged): {slang_file.name}")
            return

        print(f"    Compile: {slang_file.name}")

        entries = self.detect_entries(slang_file)
        if not entries:
            return

        outputs = []
        for stage, entry in entries.items():
            outputs.append(self.compile_stage(slang_file, stage, entry))

        (self.output_dir / f"{slang_file.stem}.shaderhash").write_text(shader_hash)

        for f in outputs:
            shutil.copy2(f, self.copy_dir)


# ------------------------------------------------------------
# Entry
# ------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("slangc")
    parser.add_argument("slang_file")
    parser.add_argument("output_dir")
    parser.add_argument("copy_dir")
    parser.add_argument("-target", default="hlsl")
    args = parser.parse_args()

    compiler = SlangCompiler(
        args.slangc,
        Path(args.output_dir),
        Path(args.copy_dir),
        args.target
    )

    slang_file = Path(args.slang_file)
    compiler.compile_shader(slang_file)


if __name__ == "__main__":
    main()
