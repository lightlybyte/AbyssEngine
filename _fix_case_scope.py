import os

# Read and fix ui.cpp - wrap each case in braces
ui_path = r'c:/AbyssEngine/source/ui.cpp'
with open(ui_path, 'r') as f:
    content = f.read()

# The issue is that rows array is declared in Archive case but we jump over it
# Solution: wrap each case block that has local variables in braces

# Fix Archive case - add opening brace after case label, closing before next case
old_archive = '''        case Screen::Archive:
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"MISSION LOG");'''
new_archive = '''        case Screen::Archive: {
            ImGui::TextColored(ImVec4(248/255.0f,248/255.0f,248/255.0f,1),"MISSION LOG");'''
content = content.replace(old_archive, new_archive)

# Fix closing of Archive case (add } before Credits)
old_credits_boundary = '''            break;
        case Screen::Credits: {'''
new_credits_boundary = '''            break;
        }
        case Screen::Credits: {'''
content = content.replace(old_credits_boundary, new_credits_boundary)

# Fix closing of Credits case
old_login_boundary = '''            break;
        case Screen::Login: {'''
new_login_boundary = '''            break;
        }
        case Screen::Login: {'''
content = content.replace(old_login_boundary, new_login_boundary)

# Fix closing of Login case
old_profile_boundary = '''            break;
        case Screen::Profile: {'''
new_profile_boundary = '''            break;
        }
        case Screen::Profile: {'''
content = content.replace(old_profile_boundary, new_profile_boundary)

# Fix closing of Profile case  
old_default_boundary = '''            break;
        default: break;'''
new_default_boundary = '''            break;
        }
        default: break;'''
content = content.replace(old_default_boundary, new_default_boundary)

# Write fixed content
with open(ui_path, 'w') as f:
    f.write(content)

print('Fixed case scope issues in ui.cpp')