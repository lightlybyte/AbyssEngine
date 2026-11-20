import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# Stage all changes
subprocess.run(['git', 'add', '-A'])

# Commit deletion of temp file
result = subprocess.run(['git', 'commit', '-m', 'Remove temp build scripts'], capture_output=True, text=True)
print(result.stdout[-300:] if result.stdout else '')
if result.stderr:
    print('STDERR:', result.stderr[:200])
