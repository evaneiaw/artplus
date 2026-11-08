@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if exist H:\Desktop\artplus-lite\build-msvc rmdir /s /q H:\Desktop\artplus-lite\build-msvc
cmake -S H:\Desktop\artplus-lite -B H:\Desktop\artplus-lite\build-msvc -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
if errorlevel 1 exit /b 1
cmake --build H:\Desktop\artplus-lite\build-msvc --config Release
if errorlevel 1 exit /b 1
echo Build complete!