import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Update shaders to realistic earth-like lighting\n\n')
    f.write('- Added atmospheric scattering and fog to advanced_lighting.frag\n')
    f.write('- Implemented height-based terrain coloring in terrain.frag\n')
    f.write('- Added animated water effects in terrain_animation.frag\n')
    f.write('- All shaders now use realistic sun position and ambient lighting\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print('STDERR:', result.stderr[:500])

# Cleanup
os.remove(msg_file)