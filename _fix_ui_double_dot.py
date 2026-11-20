import os

ui_path = r'c:/AbyssEngine/source/ui.cpp'

# Read current content
with open(ui_path, 'r') as f:
    content = f.read()

# Fix double dot issue
content = content.replace('ImGui::.SetCursorPosX', 'ImGui::SetCursorPosX')

# Write fixed content back
with open(ui_path, 'w') as f:
    f.write(content)

print('Fixed ui.cpp double dot issue')
