import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Final cleanup and commit
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Final cleanup of temp files'], capture_output=True, text=True)
print(result.stdout[-300:] if result.stdout else '')
print(result.stderr[-200:] if result.stderr else '')