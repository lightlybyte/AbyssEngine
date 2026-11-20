import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Fix compilation errors in ui.cpp, engine.cpp, main.cpp\n\n')
    f.write('- Fixed double dot syntax error in ui.cpp (ImGui::.SetCursorPosX -> ImGui::SetCursorPosX)\n')
    f.write('- Added missing semicolon after ImGui::SetWindowPos\n')
    f.write('- Fixed switch/case brace structure in DrawContentArea\n')
    f.write('- Fixed RenderMenu signature in engine.h (removed nk_context*)\n')
    f.write('- Fixed MenuScreen to Ui::Screen conversion in engine.cpp\n')
    f.write('- Fixed main.cpp menu rendering call\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout[-500:] if result.stdout else '')
if result.stderr:
    print('STDERR:', result.stderr[:300])

# Cleanup
if os.path.exists(msg_file):
    os.remove(msg_file)