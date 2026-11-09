with open('source/ui.cpp') as f:
    content = f.read()
print('Has Session rows:', 'SESSION' in content)
print('Has Screen_Login:', 'Screen_Login' in content)
print('Has Screen::Login enum:', 'Screen::Login' in content)
print('Has MenuScreen::LOGIN:', 'MenuScreen::LOGIN' in content)
print('ui.cpp length:', len(content))

with open('source/engine.cpp') as f:
    content = f.read()
print()
print('Has IsLoggedIn Engine:::', 'bool Engine::IsLoggedIn()' in content)
print('Has auth_login callback:', 'auth_login' in content)
print('Has auth_logout callback:', 'auth_logout' in content)

with open('source/main.cpp') as f:
    content = f.read()
print()
print('Has beginUIFrame call:', 'renderer.beginUIFrame()' in content)
print('Has endUIFrame call:', 'renderer.endUIFrame()' in content)
print('Has RenderMenu call:', 'Engine::RenderMenu(renderer, renderer.ctx())' in content)

with open('source/vkrender.h') as f:
    content = f.read()
print()
print('Has beginUIFrame decl:', 'void beginUIFrame();' in content)
print('Has endUIFrame decl:', 'void endUIFrame();' in content)
