@echo off
setlocal
cd /d "%~dp0"

echo UO: Booting Abyss Engine...

REM Add the stripped Vulkan and GLFW to PATH for this session
set PATH=%CD%\vulkansdk\windows\bin;%CD%\vulkansdk\windows\dll;%CD%\lib;%PATH%

REM Check if Compile.py exists. If not, call the medic.
if not exist Compile.py (
    echo UO: Compile.py missing. Calling FetchFixedFiles...
    if exist FetchFixedFiles.py (
        python FetchFixedFiles.py Compile.py
    ) else (
        echo UO: FetchFixedFiles.py not found. Cannot heal.
        exit /b 1
    )
)

REM The parent calls the child by full name
python Compile.py

if %errorlevel% neq 0 (
    echo UO: Compile.py failed. The engine is a brick.
    exit /b 1
)

echo UO: Execution finished.