@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
path %PATH%;C:\Qt\6.8.3\msvc2022_64\bin
cd H:\Desktop\artplus-lite\build-msvc-nmake\bin
ctest --test-dir H:\Desktop\artplus-lite\build-msvc-nmake --output-on-failure