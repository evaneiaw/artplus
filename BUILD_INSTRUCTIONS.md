# ArtPlus — BUILD_INSTRUCTIONS

## Prerequisites (already installed on this machine)

| Component | Version | Location |
|---|---|---|
| Windows | 11 Pro x64 | — |
| Visual Studio Build Tools | 2022 (MSVC 19.44) | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| Qt | 6.8.3 (msvc2022_64) | `C:\Qt\6.8.3\msvc2022_64` |
| PDFium | 158.0.8125.0 | `third_party\` (`bin\pdfium.dll`, `include\fpdfview.h`) |
| CMake | 4.4.4 | on PATH |
| Ninja | 1.13 | on PATH |

> Do **not** configure with MinGW: Qt and PDFium in this repository are MSVC
> builds and MinGW cannot link them. That mismatch produced the legacy,
> unbuildable `build/` tree.

## Configure (once)

Open **"x64 Native Tools Command Prompt for VS 2022"**, or prefix commands with
`vcvars64.bat` as below:

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -S H:\Desktop\artplus-lite -B H:\Desktop\artplus-lite\build-msvc ^
      -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ^
      -DPDFium_DIR=H:/Desktop/artplus-lite/third_party
```

## Build

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake --build H:\Desktop\artplus-lite\build-msvc --parallel 8
```

From PowerShell:

```powershell
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "`"$vcvars`" >nul 2>&1 && cmake --build H:\Desktop\artplus-lite\build-msvc --parallel 8"
```

## Outputs

| Artifact | Path |
|---|---|
| Application | `build-msvc\bin\ArtPlus.exe` |
| PDF runtime | `build-msvc\bin\pdfium.dll` (copied automatically) |
| Qt runtime | `build-msvc\bin\Qt6*.dll`, `platforms\qwindows.dll` (via `windeployqt`) |
| Tests | `build-msvc\bin\artplus_pdf_smoke.exe`, `artplus_pageview_test.exe`, `artplus_mainwindow_test.exe` |

`pdfium.dll` is copied next to each consumer automatically by CMake.

## Deploy Qt DLLs (needed once, or after a Qt upgrade)

```bat
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe --release --no-translations ^
    --no-opengl-sw H:\Desktop\artplus-lite\build-msvc\bin\ArtPlus.exe
```

`--no-opengl-sw` prints a deprecation warning and the tool exits non-zero even
though deployment succeeded; verify that `Qt6Core.dll` and
`platforms\qwindows.dll` exist in `bin\`.

## Run

```bat
H:\Desktop\artplus-lite\build-msvc\bin\ArtPlus.exe
H:\Desktop\artplus-lite\build-msvc\bin\ArtPlus.exe "C:\path\to\file.pdf"
```

The PDF argument is optional. Drag & drop and *File → Open PDF* also work.

## Clean rebuild

```bat
rmdir /s /q H:\Desktop\artplus-lite\build-msvc
```

Then repeat *Configure* + *Build* + *Deploy Qt DLLs*.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `LNK1104: cannot open file bin\ArtPlus.exe` | The app is still running; close it, rebuild. |
| `Could not find Qt6` | Wrong kit; point `CMAKE_PREFIX_PATH` at `msvc2022_64`, not `mingw_64`. |
| App starts and exits immediately | Missing Qt DLLs; run `windeployqt` (see above). |
| Blank page after open | `pdfium.dll` missing next to the exe; rebuild or copy it. |
| `Qt platform plugin "windows" not found` | Deploy `platforms\qwindows.dll` with `windeployqt`. |

## Run the registered regression suite

After building all six test targets and deploying the existing runtime, use the
`ctest.exe` next to the same installed Kitware `cmake.exe` used for configuration:

```powershell
& $kitCtest --test-dir build-msvc --output-on-failure --parallel 2
& $kitCtest --test-dir build-msvc -R '^artplus.smoke$' --output-on-failure
```

Set `$kitCtest` to that executable's absolute path. The filtered smoke run also
runs the edit fixture setup automatically. Tests write only under the build
folder (`tests/runtime`); each has a 60-second timeout. A failed fixture prevents
its consumers from running against stale PDFs. Build tests before invoking CTest;
CTest does not compile them. See TEST_STRATEGY.md for current runtime blockers.
