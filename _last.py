import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Final stage and commit
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Remove temp cleanup script'], capture_output=True, text=True)
print(result.stdout[-200:] if result.stdout else '')