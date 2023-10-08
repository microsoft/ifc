@echo off

:: Build file for CodeQL

cd ..

cmake --preset=ci-windows ^
-D CMAKE_TOOLCHAIN_FILE= ^
-D "CMAKE_MODULE_PATH:PATH=%cd%\cmake\find"
if not %errorlevel%==0 exit /b %errorlevel%

cmake --build build --config Debug
if not %errorlevel%==0 exit /b %errorlevel%
