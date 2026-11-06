@echo off
start "" "H:\Desktop\artplus-lite\build-msvc-nmake\bin\ArtPlus.exe"
timeout /t 3 >nul
tasklist | find "ArtPlus.exe" >nul
if errorlevel 1 (
    echo ArtPlus.exe NÃO encontrado na lista de processos
) else (
    echo ArtPlus.exe encontrado na lista de processos
)
timeout /t 2 >nul
echo Verificando interface...