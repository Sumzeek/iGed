import os
import sys
import subprocess
import re
import json
import argparse
from pathlib import Path
from typing import Dict, List


# ------------------------------------------------------------
# Logging
# ------------------------------------------------------------

def log_error(message: str):
    print(f"\033[91m[Error] {message}\033[0m", file=sys.stderr)


def log_info(message: str):
    print(f"    {message}")


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
# Reflection data extraction
# ------------------------------------------------------------

SLANG_TYPE_MAP = {
    "float32": {1: "float", 2: "float2", 3: "float3", 4: "float4"},
    "int32": {1: "int", 2: "int2", 3: "int3", 4: "int4"},
    "uint32": {1: "uint", 2: "uint2", 3: "uint3", 4: "uint4"},
}


def slang_type_to_string(type_info: dict) -> str:
    kind = type_info.get("kind", "")

    if kind == "matrix":
        rows = type_info.get("rowCount", 4)
        cols = type_info.get("columnCount", 4)
        if rows == cols:
            return f"mat{rows}"
        return f"mat{rows}x{cols}"

    if kind == "vector":
        count = type_info.get("elementCount", 1)
        elem = type_info.get("elementType", {})
        scalar = elem.get("scalarType", "float32")
        return SLANG_TYPE_MAP.get(scalar, {}).get(count, f"vec{count}")

    if kind == "scalar":
        scalar = type_info.get("scalarType", "float32")
        return SLANG_TYPE_MAP.get(scalar, {}).get(1, "float")

    return "unknown"


def extract_resources(reflection: dict) -> list:
    resources = []
    params = reflection.get("parameters", [])

    for param in params:
        binding_info = param.get("binding", {})
        binding_kind = binding_info.get("kind", "")

        resource = {
            "name": param.get("name", ""),
            "type": binding_kind,
            "binding": binding_info.get("index", 0),
        }

        # Extract stages from entryPoints that use this resource
        stages = []
        for ep in reflection.get("entryPoints", []):
            for b in ep.get("bindings", []):
                if b.get("name") == resource["name"]:
                    stages.append(ep.get("stage", ""))
        resource["stages"] = stages if stages else ["vertex"]

        # Extract members for constant buffers
        type_info = param.get("type", {})
        if binding_kind == "constantBuffer":
            elem_type = type_info.get("elementType", {})
            fields = elem_type.get("fields", [])
            members = []
            for field in fields:
                member_binding = field.get("binding", {})
                members.append({
                    "name": field.get("name", ""),
                    "type": slang_type_to_string(field.get("type", {})),
                    "offset": member_binding.get("offset", 0),
                    "size": member_binding.get("size", 0),
                })
            resource["members"] = members

        resources.append(resource)

    return resources


def extract_vertex_inputs(reflection: dict) -> list:
    vertex_inputs = []

    for ep in reflection.get("entryPoints", []):
        if ep.get("stage") != "vertex":
            continue

        for param in ep.get("parameters", []):
            if param.get("stage") != "vertex":
                continue

            param_type = param.get("type", {})
            if param_type.get("kind") != "struct":
                continue

            for field in param_type.get("fields", []):
                binding = field.get("binding", {})
                if binding.get("kind") != "varyingInput":
                    continue

                vertex_inputs.append({
                    "name": field.get("name", ""),
                    "location": binding.get("index", 0),
                    "format": slang_type_to_string(field.get("type", {})),
                    "semantic": field.get("semanticName", ""),
                })

    # Sort by location
    vertex_inputs.sort(key=lambda x: x["location"])
    return vertex_inputs


# ------------------------------------------------------------
# Slang Compiler
# ------------------------------------------------------------

SHADER_SUFFIXES = {
    "vertex": "_vs",
    "fragment": "_fs",
    "geometry": "_gs",
    "tesscontrol": "_tcs",
    "tesseval": "_tes",
    "hull": "_hs",
    "domain": "_ds",
    "compute": "_cs",
}

PLATFORM_CONFIG = {
    "dxil": {"target": "dxil", "profile": "sm_6_6", "extension": "dxil"},
    "spirv": {"target": "spirv", "profile": "spirv_1_5", "extension": "spv"},
}


class SlangCompiler:
    def __init__(self, slangc_path: str, output_dir: Path, platforms: List[str]):
        self.slangc_path = slangc_path
        self.output_dir = output_dir
        self.platforms = platforms

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

    def compile_stage(self, slang_file: Path, stage: str, entry: str,
                      platform: str, shader_dir: Path) -> Path:
        config = PLATFORM_CONFIG[platform]
        suffix = SHADER_SUFFIXES.get(stage, f"_{stage}")
        out_file = shader_dir / f"{slang_file.stem}{suffix}.{config['extension']}"
        reflection_file = shader_dir / f"{slang_file.stem}{suffix}_reflection.json"

        import_dir = slang_file.parent / "modules"
        cmd = [
            self.slangc_path,
            str(slang_file),
            "-target", config["target"],
            "-profile", config["profile"],
            "-stage", stage,
            "-entry", entry,
            "-I", str(import_dir),
            "-o", str(out_file),
            "-reflection-json", str(reflection_file),
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            log_error(f"slangc failed for {slang_file.name} ({stage}/{platform}):\n{result.stderr}")
            raise RuntimeError(f"Compilation failed: {slang_file.name} {stage} {platform}")

        return out_file, reflection_file

    def compile_shader(self, slang_file: Path, depfile_path: Path = None):
        shader_name = slang_file.stem
        shader_dir = self.output_dir / shader_name
        shader_dir.mkdir(parents=True, exist_ok=True)

        log_info(f"Compiling: {slang_file.name}")

        entries = self.detect_entries(slang_file)
        if not entries:
            log_error(f"No shader entries found in {slang_file.name}")
            return

        # Build stages info and compile for all platforms
        stages = []
        all_outputs = [shader_dir / f"{shader_name}.shader.json"]
        first_reflection = None

        for stage, entry in entries.items():
            stage_info = {
                "stage": stage,
                "entry": entry,
                "bytecode": {},
            }

            for platform in self.platforms:
                config = PLATFORM_CONFIG[platform]
                suffix = SHADER_SUFFIXES.get(stage, f"_{stage}")

                out_file, reflection_file = self.compile_stage(
                    slang_file, stage, entry, platform, shader_dir
                )
                all_outputs.append(out_file)

                stage_info["bytecode"][platform] = out_file.name

                # Use first platform's reflection (platform-agnostic at abstract level)
                if first_reflection is None:
                    first_reflection = reflection_file

            stages.append(stage_info)

        # Extract reflection data from first platform's vertex stage reflection
        reflection_data = {"resources": [], "vertexInputs": []}
        if first_reflection and first_reflection.exists():
            try:
                raw = json.loads(first_reflection.read_text(encoding="utf-8"))
                reflection_data["resources"] = extract_resources(raw)
                reflection_data["vertexInputs"] = extract_vertex_inputs(raw)
            except Exception as e:
                log_error(f"Failed to parse reflection: {e}")

        # Generate .shader.json
        shader_json = {
            "name": shader_name,
            "stages": stages,
            "reflection": reflection_data,
        }

        shader_json_path = shader_dir / f"{shader_name}.shader.json"
        shader_json_path.write_text(
            json.dumps(shader_json, indent=2, ensure_ascii=False),
            encoding="utf-8"
        )

        # Clean up per-stage reflection JSON files (already merged into .shader.json)
        for stage, entry in entries.items():
            for platform in self.platforms:
                suffix = SHADER_SUFFIXES.get(stage, f"_{stage}")
                config = PLATFORM_CONFIG[platform]
                ref_file = shader_dir / f"{shader_name}{suffix}_reflection.json"
                if ref_file.exists():
                    ref_file.unlink()

        # Generate depfile
        if depfile_path:
            deps = [slang_file] + collect_dependencies(slang_file)
            targets = " ".join(str(o).replace("\\", "/") for o in all_outputs)
            dep_list = " \\\n  ".join(str(d).replace("\\", "/") for d in deps)
            depfile_path.parent.mkdir(parents=True, exist_ok=True)
            depfile_path.write_text(f"{targets}: \\\n  {dep_list}\n", encoding="utf-8")

        log_info(f"  -> {shader_dir.name}/ ({len(stages)} stages, {len(self.platforms)} platforms)")


# ------------------------------------------------------------
# Entry
# ------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Slang shader compiler")
    parser.add_argument("slangc", help="Path to slangc executable")
    parser.add_argument("slang_file", help="Path to .slang source file")
    parser.add_argument("output_dir", help="Output directory for compiled shaders")
    parser.add_argument("-targets", default="dxil,spirv",
                        help="Comma-separated target platforms (dxil,spirv)")
    parser.add_argument("-depfile", default=None,
                        help="Path to output .d dependency file")
    args = parser.parse_args()

    platforms = [p.strip() for p in args.targets.split(",")]
    for p in platforms:
        if p not in PLATFORM_CONFIG:
            log_error(f"Unsupported platform: {p}. Supported: {', '.join(PLATFORM_CONFIG.keys())}")
            sys.exit(1)

    compiler = SlangCompiler(
        args.slangc,
        Path(args.output_dir),
        platforms
    )

    slang_file = Path(args.slang_file)
    depfile = Path(args.depfile) if args.depfile else None
    compiler.compile_shader(slang_file, depfile)


if __name__ == "__main__":
    main()
