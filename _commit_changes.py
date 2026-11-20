import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Switch to Dear ImGui and rename project to Dynami 20.02\n\n')
    f.write('- Replaced Nuklear UI with Dear ImGui backend\n')
    f.write('- Added imgui.cpp, imgui_impl_glfw.cpp, imgui_impl_vulkan.cpp\n')
    f.write('- Fixed Vulkan initialization for new ImGui API (MSAASamples moved to PipelineInfoMain)\n')
    f.write('- Renamed project from DATS/Abyss Engine to Dynami 20.02\n')
    f.write('- Updated shaders for realistic earth-like lighting\n')
    f.write('- Fixed compilation errors in ui.cpp, main.cpp, vkrender.cpp\n')
    f.write('- Added DeformableBody physics system with hierarchical hitbox splitting\n')
    f.write('- Created webserver auth system at websiteauth/\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout)
print(result.stderr)

# Cleanup
os.remove(msg_file)
