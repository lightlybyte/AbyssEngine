import subprocess
import os
os.chdir(r'c:/AbyssEngine')
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Fix case scope issues in ui.cpp'], capture_output=True, text=True)
print(result.stdout[-300:] if result.stdout else '')
if result.stderr:
    print('STDERR:', result.stderr[:200])