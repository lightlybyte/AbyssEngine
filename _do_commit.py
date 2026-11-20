import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Fix ImGui Vulkan initialization and MSAASamples placement\n\n')
    f.write('- Added PipelineInfoMain.MSAASamples to recreateSwapchain for correct Vulkan API usage\n')
    f.write('- Removed duplicate MSAASamples assignment in init() block\n')
    f.write('- Ensures proper MSAA settings are applied during swapchain recreation\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print('STDERR:', result.stderr[:500])

# Cleanup
os.remove(msg_file)