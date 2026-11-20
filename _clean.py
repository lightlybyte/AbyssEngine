import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage deletion and commit
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Clean up temp push script'], capture_output=True, text=True)
print(result.stdout[-200:] if result.stdout else '')