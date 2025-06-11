@echo off
cd /d %~dp0

for /f "usebackq tokens=*" %%i in (`tools\vswhere.exe -latest -property installationPath`) do set "VS_INSTALL_DIR=%%i"
call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvarsall.bat" x64

:: INSTALL VCPKG
if not exist "vcpkg" (
    git clone https://github.com/microsoft/vcpkg.git
	cd vcpkg
	git remote set-url origin https://github.com/microsoft/vcpkg.git
	call bootstrap-vcpkg.bat
	cd ..
) else (
	cd vcpkg
	git pull
	call bootstrap-vcpkg.bat
	cd ..
)
:: INSTALL_DEPENDENCIES
vcpkg\vcpkg.exe install directx-headers
vcpkg\vcpkg.exe install directx12-agility
vcpkg\vcpkg.exe install directx-dxc
vcpkg\vcpkg.exe install directxtex
vcpkg\vcpkg.exe install cgltf
if errorlevel 1 goto ERROR_END

:: CMAKE BUILD
if not exist "build" (
    mkdir build
)

cmake -G "Visual Studio 17 2022" -A x64 -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
if errorlevel 1 goto ERROR_END

goto SUCCESS_END

:ERROR_END
echo Build failed!
pause
exit /b 1

:SUCCESS_END
echo Build succeeded!
exit /b 0
