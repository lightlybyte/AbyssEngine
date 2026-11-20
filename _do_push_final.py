import subprocess
import os
os.chdir(r'c:/AbyssEngine')
result = subprocess.run(['git', 'push', 'origin', 'main'], capture_output=True, text=True)
print('Push result:', result.returncode)
if result.stdout: print(result.stdout[-300:])
if result.stderr: print(result.stderr[:200])