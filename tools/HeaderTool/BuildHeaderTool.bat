@echo off
setlocal

REM Root folder of the header tool
set TOOL_ROOT=%~dp0%\source

REM Build folder for the header tool
set BUILD_DIR=%TOOL_ROOT%\build

REM Make sure build folder exists
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Step 1: Generate Visual Studio solution using CMake
echo Generating solution for VikingHeaderTool...
cmake -S "%TOOL_ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64

if %errorlevel% neq 0 (
    echo CMake generation failed!
    exit /b 1
)

REM Step 2: Build the EXE
echo Building VikingHeaderTool.exe...
cmake --build "%BUILD_DIR%" --config Shipping --target VikingHeaderTool

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
 
cd /d %~dp0
echo Copying exe from %BUILD_DIR%\Shipping\VikingHeaderTool.exe
copy /Y %TOOL_ROOT%\bin\Shipping\VikingHeaderTool.exe .\VikingHeaderTool.exe

rmdir /S /Q %TOOL_ROOT%\bin
rmdir /S /Q %TOOL_ROOT%\lib
rmdir /S /Q %TOOL_ROOT%\build
echo Build complete!
endlocal