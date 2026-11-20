import subprocess
import os
os.chdir(r'c:/AbyssEngine')
subprocess.run(['git', 'add', '-A'])
result = subprocess.run(['git', 'commit', '-m', 'Fix ui.cpp missing semicolon and switch brace'], capture_output=True, text=True)
print(result.stdout[-300:] if result.stdout else '')