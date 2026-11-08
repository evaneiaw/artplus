@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake --build H:\Desktop\artplus-lite\build-msvc --target ArtPlus --config Release