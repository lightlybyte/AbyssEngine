import subprocess
result = subprocess.run(['git', 'add', '-A'], capture_output=True, text=True)
result = subprocess.run(['git', 'commit', '-m', 'Fix duplicate source entries in ENGINE_CPP'], capture_output=True, text=True)
print(result.stdout[-300:] if result.stdout else '')
if result.stderr:
    print('STDERR:', result.stderr[:200])