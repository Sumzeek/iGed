import os
import sys
import subprocess
import re

LOG_FILE = "shader_compile.log"

def log(message):
    pass
    # with open(LOG_FILE, "a", encoding="utf-8") as f:
    #     f.write(message + "\n")
    # print(message)

def detect_entry_points(shader_path):
    """Simple detection of which entry points exist in the shader file: vsMain, psMain, computeMain"""
    with open(shader_path, "r", encoding="utf-8") as f:
        content = f.read()
    entry_points = {
        "vsMain": bool(re.search(r'\bvsMain\b', content)),
        "psMain": bool(re.search(r'\bpsMain\b', content)),
        "computeMain": bool(re.search(r'\bcomputeMain\b', content))
    }
    return entry_points

def compile_and_merge(slangc_path, output_dir, shaders):
    os.makedirs(output_dir, exist_ok=True)
    log(f"Output directory ensured: {output_dir}")

    for shader_path in shaders:
        shader_name = os.path.splitext(os.path.basename(shader_path))[0]
        entry_points = detect_entry_points(shader_path)

        vert_file = os.path.join(output_dir, f"{shader_name}.vert")
        frag_file = os.path.join(output_dir, f"{shader_name}.frag")
        comp_file = os.path.join(output_dir, f"{shader_name}.comp")
        glsl_file = os.path.join(output_dir, f"{shader_name}.glsl")

        compiled_parts = []

        # compile vertex shader
        # slangc.exe ../slang/Texture.slang -profile glsl_450 -target glsl -o ../generated/glsl/Texture.vert -entry vsMain
        if entry_points["vsMain"]:
            log(f"Compiling vertex shader for {shader_name}...")
            cmd_vert = [
                slangc_path,
                shader_path,
                "-profile", "glsl_450",
                "-target", "glsl",
                "-entry", "vsMain",
                "-o", vert_file
            ]
            res = subprocess.run(cmd_vert, capture_output=True, text=True)
            if res.returncode != 0:
                log(f"Error compiling vertex shader for {shader_name}:\n{res.stderr}")
            else:
                with open(vert_file, "r", encoding="utf-8") as vf:
                    vert_code = vf.read()
                compiled_parts.append(("#type vertex", vert_code))
                os.remove(vert_file)

        # compile fragment shader
        # slangc.exe ../slang/Texture.slang -profile glsl_450 -target glsl -o ../generated/glsl/Texture.frag -entry psMain
        if entry_points["psMain"]:
            log(f"Compiling fragment shader for {shader_name}...")
            cmd_frag = [
                slangc_path,
                shader_path,
                "-profile", "glsl_450",
                "-target", "glsl",
                "-entry", "psMain",
                "-o", frag_file
            ]
            res = subprocess.run(cmd_frag, capture_output=True, text=True)
            if res.returncode != 0:
                log(f"Error compiling fragment shader for {shader_name}:\n{res.stderr}")
            else:
                with open(frag_file, "r", encoding="utf-8") as ff:
                    frag_code = ff.read()
                compiled_parts.append(("#type fragment", frag_code))
                os.remove(frag_file)

        # compile compute shader
        # slangc.exe hello-world.slang -profile glsl_450 -target glsl -o hello-world.glsl -entry computeMain
        if entry_points["computeMain"]:
            log(f"Compiling compute shader for {shader_name}...")
            cmd_comp = [
                slangc_path,
                shader_path,
                "-profile", "glsl_450",
                "-target", "glsl",
                "-entry", "computeMain",
                "-o", comp_file
            ]
            res = subprocess.run(cmd_comp, capture_output=True, text=True)
            if res.returncode != 0:
                log(f"Error compiling compute shader for {shader_name}:\n{res.stderr}")
            else:
                with open(comp_file, "r", encoding="utf-8") as cf:
                    comp_code = cf.read()
                compiled_parts.append(("#type compute", comp_code))
                os.remove(comp_file)

        if compiled_parts:
            with open(glsl_file, "w", encoding="utf-8") as gf:
                for tag, code in compiled_parts:
                    gf.write(tag + "\n")
                    gf.write(code + "\n")
            log(f"Compiled and merged shader: {glsl_file}")
        else:
            log(f"No valid entry points found in {shader_name}, skipped.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        log(f"Usage: python ShaderCompile.py <slangc_path> <output_dir> <shader1> [<shader2> ...]")
        sys.exit(1)

    slangc_path = sys.argv[1]
    output_dir = sys.argv[2]
    shaders = sys.argv[3:]

    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)

    compile_and_merge(slangc_path, output_dir, shaders)
