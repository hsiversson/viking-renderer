@echo off
cd /d %~dp0

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
vcpkg\vcpkg.exe install directx-dxc
if errorlevel 1 goto ERROR_END

:: CMAKE BUILD
if not exist "build" (
    mkdir build
)

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake -A x64
if errorlevel 1 goto ERROR_END

goto SUCCESS_END

:ERROR_END
echo Build failed!
pause
exit /b 1

:SUCCESS_END
echo Build succeeded!
exit /b 0
