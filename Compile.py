import os
import sys
import subprocess
import glob
from concurrent.futures import ThreadPoolExecutor

# Paths
ROOT = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(ROOT, "source")
INCLUDE = os.path.join(ROOT, "include")
LIB_DIR = os.path.join(ROOT, "lib") # GLFW static libs are here
WREN_SRC = os.path.join(ROOT, "compiler", "src")
BUILD_DIR = os.path.join(ROOT, "Build", "Windows")
VULKAN_ROOT = os.path.join(ROOT, "vulkansdk")
VULKAN_INCLUDE = os.path.join(VULKAN_ROOT, "multiarch", "include")
VULKAN_LIB = os.path.join(VULKAN_ROOT, "windows", "lib")

os.makedirs(BUILD_DIR, exist_ok=True)

CXX = "g++"
CXX_FLAGS = [
    "-std=c++17", "-O2", 
    "-I", VULKAN_INCLUDE,
    "-I", INCLUDE,                                  # GLFW and ImGui headers
    "-I", os.path.join(SOURCE, "backends"),         # ImGui backends
    "-I", os.path.join(WREN_SRC, "include"),
    "-I", os.path.join(WREN_SRC, "vm")
]

# ImGui source files
CPP_FILES = [
    os.path.join(SOURCE, "imgui.cpp"),
    os.path.join(SOURCE, "imgui_draw.cpp"),
    os.path.join(SOURCE, "imgui_tables.cpp"),
    os.path.join(SOURCE, "imgui_widgets.cpp"),
    os.path.join(SOURCE, "backends", "imgui_impl_glfw.cpp"),
    os.path.join(SOURCE, "backends", "imgui_impl_vulkan.cpp")
]

# Wren C files
C_FILES = [
    os.path.join(WREN_SRC, "vm", f) for f in [
        "wren_compiler.c", "wren_core.c", "wren_debug.c", 
        "wren_primitive.c", "wren_utils.c", "wren_value.c", "wren_vm.c"
    ]
] + [
    os.path.join(WREN_SRC, "optional", "wren_opt_meta.c"),
    os.path.join(WREN_SRC, "optional", "wren_opt_random.c")
]

# Libraries
LIBS = [
    os.path.join(VULKAN_LIB, "vulkan-1.lib"),
    "-L", LIB_DIR,      # Tells linker to look in C:\AbyssEngine\lib
    "-lglfw3",          # Links libglfw3.a (static)
    "-lgdi32", "-luser32", "-lshell32", "-lole32"
]

def find_cpp_files():
    # Find main.cpp in source/ (excluding ImGui files already listed)
    all_cpp = glob.glob(os.path.join(SOURCE, "*.cpp"))
    return [f for f in all_cpp if "imgui" not in os.path.basename(f)]

def compile_c_file(src):
    obj = os.path.join(BUILD_DIR, os.path.basename(src) + ".o")
    cmd = ["gcc", "-std=c99", "-O2", "-c", src, 
           "-I", os.path.join(WREN_SRC, "include"),
           "-I", os.path.join(WREN_SRC, "vm"),
           "-I", os.path.join(WREN_SRC, "optional"),
           "-o", obj]
    print(f"[C] {os.path.basename(src)}")
    subprocess.run(cmd, check=True)
    return obj

def compile_cpp_file(src):
    obj = os.path.join(BUILD_DIR, os.path.basename(src) + ".o")
    cmd = [CXX] + CXX_FLAGS + ["-c", src, "-o", obj]
    print(f"[C++] {os.path.basename(src)}")
    subprocess.run(cmd, check=True)
    return obj

def main():
    print("Abyss Engine Builder")
    
    cpp_files = find_cpp_files() + CPP_FILES
    
    with ThreadPoolExecutor() as pool:
        c_objects = list(pool.map(compile_c_file, C_FILES))
        cpp_objects = list(pool.map(compile_cpp_file, cpp_files))

    all_objects = c_objects + cpp_objects

    exe = os.path.join(BUILD_DIR, "AbyssEngine.exe")
    link_cmd = [CXX] + all_objects + ["-o", exe] + LIBS
    print(f"Linking {exe}...")
    subprocess.run(link_cmd, check=True)
    print("Build Complete.")

if __name__ == "__main__":
    main()