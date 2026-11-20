import subprocess
import os
os.chdir(r'c:/AbyssEngine')
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Remove temporary fix scripts'], capture_output=True, text=True)
print(result.stdout[-200:] if result.stdout else '')