import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Create commit message file
msg_file = '_commit_msg.txt'
with open(msg_file, 'w') as f:
    f.write('Remove temporary fix scripts\n\n')
    f.write('- Cleaned up _final_commit.py and other temp files\n')
    f.write('- Kept only the necessary shader fixes committed\n')

result = subprocess.run(['git', 'commit', '-F', msg_file], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print('STDERR:', result.stderr[:500])

# Cleanup
if os.path.exists(msg_file):
    os.remove(msg_file)