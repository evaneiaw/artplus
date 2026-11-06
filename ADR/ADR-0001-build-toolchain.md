# ADR-0001 — Build toolchain: MSVC + Qt msvc2022_64 + MSVC PDFium

- Status: accepted (2026-11-02)
- Context: milestone 21, foundation build

## Problem

The repository contained a `build/` tree configured with **MinGW g++** while
linking **Qt 6.8.3 msvc2022_64** and **PDFium MSVC import libraries**. MinGW
cannot consume MSVC `.lib` import libraries, so linking `ArtPlus.exe` always
failed, regardless of source fixes.

## Decision

Standardise on the Microsoft toolchain that all prebuilt dependencies already
use:

- compiler/linker: MSVC 19.44 (Visual Studio 2022 Build Tools, x64),
- Qt: `C:\Qt\6.8.3\msvc2022_64`,
- PDFium: `third_party` (msvc build),
- generator: Ninja, `CMAKE_BUILD_TYPE=Release`,
- build directory: `build-msvc/` (the legacy `build/` is not used).

## Consequences

- Positive: one coherent ABI; linking works; Qt plugins and PDFium load.
- Negative: the project is tied to MSVC on Windows for now. Portability to
  MinGW would require a matching MinGW Qt build and a MinGW PDFium build.
- Follow-up: delete the legacy `build/` directory after review; document the
  rebuild command in `BUILD_INSTRUCTIONS.md` (done).
