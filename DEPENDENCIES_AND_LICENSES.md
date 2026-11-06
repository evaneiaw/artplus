# ArtPlus — DEPENDENCIES_AND_LICENSES

This file records every third-party component used by ArtPlus and its licence,
so that redistribution decisions stay explicit.

| Component | Version | Use | Licence | Redistribution notes |
|---|---|---|---|---|
| Qt 6.8.3 (`Core`, `Gui`, `Widgets`) | 6.8.3 msvc2022_64 | GUI, images, file/IO | LGPL-3.0 (or commercial) | Dynamic linking used. Must ship Qt DLLs and the LGPL notice; allow relinking (DLLs) to comply. If ArtPlus is distributed commercially under a closed licence, evaluate the Qt commercial licence or strict LGPL compliance. |
| PDFium (Chromium) | 158.0.8125.0 | PDF parsing + rasterisation | BSD-3-Clause (PDFium) with additional Chromium/third-party notices | Binary in `third_party/bin/pdfium.dll`; keep the `licenses/` folder and the upstream notices with the distribution. |
| MSVC runtime | 19.44 | C++ runtime | Microsoft Visual C++ redistributable | Ship VCRUNTIME140(_1).dll or require the VC++ redistributable. |
| CMake | 4.4.4 | Build only | BSD-3-Clause | Build-time only; not distributed. |
| Ninja | 1.13 | Build only | Apache-2.0 | Build-time only; not distributed. |

## Components analysed but NOT embedded

The Esko ArtPro+ binaries were analysed as a **functional and behavioural
reference** for the ArtPlus project. No Esko code, pseudocode, resources,
icons, or assets are included in this repository or linked into ArtPlus.

## Rules for adding dependencies

1. Record the component, version, purpose and licence in this file **before**
   merging its use.
2. Reject dependencies whose licence is incompatible with the intended
   distribution model (commercial closed-source application, dynamic linking).
3. Prefer widely used, actively maintained libraries.
4. Do not link two libraries for the same responsibility without an ADR.
5. Keep upstream licence texts in `third_party/licenses/`.
