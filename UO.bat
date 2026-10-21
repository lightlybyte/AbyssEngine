@echo off
setlocal
cd /d "%~dp0"
echo UO: Booting Abyss Engine...

REM Add stripped Vulkan and Wren to PATH for this session
set PATH=%CD%\vulkansdk\windows\bin;%CD%\vulkansdk\windows\dll;%CD%\compiler;%PATH%

REM Check if Compile.py exists. If not, call the medic.
if not exist Compile.py (
    echo UO: Compile.py missing. Calling FetchFixedFiles...
    python FetchFixedFiles.py Compile.py
)

REM The parent calls the child by full name
python Compile.py --target Windows --config Release

if %errorlevel% neq 0 (
    echo UO: Compile.py failed. The engine is a brick.
    pause
)

echo UO: Build successful. Launching...
start "" "Build\Windows\AbyssEngine.exe"
pause