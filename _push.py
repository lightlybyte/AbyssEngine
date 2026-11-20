import subprocess
result = subprocess.run(['git', 'push', 'origin', 'main'], capture_output=True, text=True)
print('Push result:', result.returncode)
if result.stdout: print(result.stdout[-200:])
if result.stderr: print(result.stderr[:150])