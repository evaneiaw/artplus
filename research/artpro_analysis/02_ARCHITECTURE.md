# ArtPro+ v250 - Architecture Analysis

## Overall Architecture

### Executable Structure
- **File Type**: Portable Executable (PE)
- **Detected Architecture**: x86:64 (by Ghidra loader, despite 32-bit PE headers)
- **Entry Point**: As identified by Ghidra import
- **Total Functions**: [count to be determined from catalog]
- **Library Functions**: [count]
- **Thunk Functions**: [count]

### Loader Configuration
The Ghidra import used the following loader settings:
- **Program Loader**: Portable Executable (PE)
- **Language/Compiler**: x86:LE:64:default:windows
- **Library Search Path**: Extensive path including system directories, Esko-specific paths, and common runtime paths
- **TLS Callbacks**: 14+ TLS callbacks detected and processed
- **Export Processing**: Many DLL exports extracted and saved as .exports files

### Module Organization (preliminary)

Based on the binary inventory and Ghidra import, the following main modules can be identified:

1. **ArtProCore.dll** - Core engine module
2. **ArtSys.dll** - System interface module
3. **ArtProPDF.dll** - PDF processing engine
4. **Document.dll** - Document manipulation
5. **LayoutEngine.dll** - Layout engine
6. **Pack3D.dll** - 3D packing operations
7. **Cornering.dll** - Corner detection/handling
8. **CurveApprox.dll** - Curve approximation algorithms
9. **GeoMapper.dll** - Geometric mapping
10. **Offset.dll** - Offset calculation
11. **Euclid.dll** - Euclidean operations
12. **Mutator*.dll** - Mutation operations layer
13. **StepAndRepeat.dll** - Step and repeat functionality
14. **Various Qt6 modules** - GUI and framework

### Imported System DLLs
- kernel32.dll, user32.dll, gdi32.dll, advapi32.dll
- shell32.dll, ole32.dll, oleaut32.dll
- winspool.drv, winspool.drv, etc.

### Key Observations from Import Log

1. **TLS Callbacks**: Multiple Thread Local Storage callbacks were processed, indicating initialization routines that run before `main()`.

2. **Export Analysis**: The Ghidra loader successfully extracted exports from many ArtPro+ DLLs, providing entry points for further analysis.

3. **Library Distinction**: Functions were categorized as library functions vs. application functions.

4. **Wrong Architecture**: Many system DLLs (kernel32, user32, etc.) were skipped because they are x64 while the ArtPro+ binary is detected as x86:64 (likely running under WOW64 emulation).

5. **Third-party Libraries**: Significant use of:
   - Boost library components (filesystem, iostreams, locale, thread, etc.)
   - OpenCV for image processing
   - Little CMS (lcms.dll) for color management
   - OpenEXR and OpenImageIO for HDR imaging
   - Qt6 framework for GUI components

6. **Esko-Specific DLLs**: The application loads numerous Esko-specific DLLs organized by function:
   - ArtDocument*, ArtPage*, ArtLayer* families
   - Document* families (BarCode, InkEater, OCR, Structure, etc.)
   - Color* and CMS* modules
   - Trapping and preflight modules
   - Step and repeat modules

## Memory Layout (preliminary)

### Code Sections
- TLS callbacks at addresses: 0x14161cb00, 0x180060498, 0x1800165b8, etc.
- Export table functions at various addresses
- Code density suggests moderate optimization

### Data Sections
- Global data structures for document management
- Color management tables
- Geometry/transformation matrices
- Font and image caches

### Resource Sections
- Strings and UI resources (Localization.dll references)
- Icon and menu resources
- Configuration data

## Pending Analysis

The following require deeper investigation:
1. Full function list with names and signatures
2. Cross-references between modules
3. Vtables and class hierarchies (if C++).
4. Decompiled pseudocode for key functions
5. Memory layout and object models
6. Algorithm implementations (geometry, color, trapping)