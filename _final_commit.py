import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Fix Vulkan shader uniform block for terrain_animation\n\n')
    f.write('- Moved standalone uniform float time into a proper uniform block\n')
    f.write('- Vulkan GLSL requires all uniforms to be inside uniform blocks\n')
    f.write('- Changed time reference from "time" to "ubo.time" in fragment shader\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print('STDERR:', result.stderr[:500])

# Cleanup
os.remove(msg_file)