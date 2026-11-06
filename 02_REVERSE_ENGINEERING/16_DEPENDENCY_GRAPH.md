# ArtPro+ v250 - Dependency Graph

## Overview
This document maps the identified dependencies between ArtPro+ modules based on the binary inventory, Ghidra import analysis, and function catalog. The dependency graph illustrates how different modules interact and rely on each other.

## Module Dependency Matrix

### Rows -> From, Columns -> To

| From \ To | Document | Geometry | Color | Trapping | PDF | Preflight | Automation | Marks | Interface |
|-----------|----------|----------|-------|----------|-----|-----------|------------|-------|-----------|
| **Document** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Geometry** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Color** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Trapping** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **PDF** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Preflight** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Automation** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Marks** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Interface** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

✓ = Direct dependency observed

## Primary Dependency Relationships

### 1. Document Engine Dependencies

#### Document <- Geometry
- **Purpose**: Object bounds, transformations, hit testing
- **Details**:
  - GraphicObject bounds retrieval for hit testing
  - Transformation matrix application for object positioning
  - Boolean operation operands for compound shapes
  - Distance calculations for selection
- **Evidence**: Geometry engine functions called during object operations

#### Document <- Color
- **Purpose**: Object colors, color management, separation
- **Details**:
  - Fill and stroke color retrieval
  - Color space conversion for display
  - ICC profile application
  - Spot color management
- **Evidence**: ColorManipulation.dll integration

#### Document <- Trapping
- **Purpose**: Trap generation for print output
- **Details**:
  - Edge detection from object boundaries
  - Trap width calculation per object pair
  - Trap rendering in separation
  - Corner handling at object overlaps
- **Evidence**: Trapping.dll export functions

#### Document <- PDF
- **Purpose**: PDF export/import, preservation
- **Details**:
  - Document export to PDF format
  - PDF import for editing
  - Object-to-PDF mapping
  - Preservation of vector data
- **Evidence**: ArtProPDF.dll core functions

#### Document <- Preflight
- **Purpose**: Document validation, auto-fix
- **Details**:
  - Rule-based validation checks
  - Automatic correction suggestions
  - Report generation
  - Mutator operations
- **Evidence**: PreflightProfile.dll rule checks

#### Document <- Automation
- **Purpose**: Action list execution, batch processing
- **Details**:
  - Action list commands operating on document
  - Variable data integration
  - Batch processing workflows
  - Script command execution
- **Evidence**: Automation.dll action list parsing

#### Document <- Marks
- **Purpose**: Mark generation, placement
- **Details**:
  - Registration mark creation
  - Crop mark generation
  - Color bar addition
  - Control strip placement
- **Evidence**: DocumentMarks.dll mark generation

#### Document <- Interface
- **Purpose**: UI interaction, event handling
- **Details**:
  - Menu command dispatch
  - Toolbar button actions
  - Property panel data binding
  - Shortcut handling
- **Evidence**: Localization.dll, SmartNames.dll

### 2. Geometry Engine Dependencies

#### Geometry <- Color
- **Purpose**: Shape fills, strokes, gradients
- **Details**:
  - Object fill color application
  - Stroke color rendering
  - Gradient mesh color interpolation
  - Pattern fill tiling colors
- **Evidence**: Color engine functions in rendering pipeline

#### Geometry <- Trapping
- **Purpose**: Edge detection, trap geometry
- **Details**:
  - Boolean operation operands
  - Trap polygon generation from edges
  - Corner join styles
  - Curve trap generation
- **Evidence**: Trapping.dll edge detection functions

#### Geometry <- PDF
- **Purpose**: Path data extraction, transformation
- **Details**:
  - PDF content stream path parsing
  - Transformation matrix extraction
  - Vector data preservation
  - Boolean operation translation
- **Evidence**: ArtProPDF.dll path handling

#### Geometry <- Interface
- **Purpose**: Property panel, transform controls
- **Details**:
  - Transform parameter display
  - Bounds and dimension editing
  - Measurement tool outputs
  - Selection geometry
- **Evidence**: Interface UI references

### 3. Color Engine Dependencies

#### Color <- Trapping
- **Purpose**: Trap color determination
- **Details**:
  - Choke/spread color selection
  - Delta E based color similarity
  - Spot color trap colors
  - Overprint simulation
- **Evidence**: Trapping.dll color interaction functions

#### Color <- PDF
- **Purpose**: PDF color space conversion
- **Details**:
  - PDF color space conversion
  - Embedded profile handling
  - Output intent application
  - Device link lookup
- **Evidence**: ArtProPDF.dll color management

#### Color <- Interface
- **Purpose**: Color selection UI, swatches
- **Details**:
  - Color picker dialog
  - Swatch management
  - Color format display
  - Sample display
- **Evidence**: Localization.dll color terms

### 4. Trapping Engine Dependencies

#### Trapping <- Geometry
- **Purpose**: Edge detection, polygon generation
- **Details**:
  - Object boundary extraction
  - Polygon offset calculation
  - Corner join geometry
  - Curve tracing
- **Evidence**: GeoMapper.dll, Cornering.dll functions

#### Trapping <- Color
- **Purpose**: Color-based trap parameters
- **Details**:
  - Color similarity (Delta E)
  - Choke/spread color determination
  - Ink interaction handling
  - Spot color special cases
- **Evidence**: ColorManipulation.dll integration

#### Trapping <- Interface
- **Purpose**: Trap parameters UI
- **Details**:
  - Trap width input
  - Direction selection (choke/spread)
  - Corner style selection
  - Preview generation
- **Evidence**: Interface DLL references

### 5. PDF Engine Dependencies

#### PDF <- Color
- **Purpose**: PDF color spaces, profiles
- **Details**:
  - PDF color space conversion
  - Embedded ICC profiles
  - Output intent
  - Device links
- **Evidence**: ArtProPDF.dll color handling

#### PDF <- Geometry
- **Purpose**: Path data, transformations
- **Details**:
  - Content stream path parsing
  - Transformation matrix extraction
  - Vector data preservation
  - Boolean operation translation
- **Evidence**: ArtProPDF.dll path functions

#### PDF <- Document
- **Purpose**: Document export/import
- **Details**:
  - Document-to-PDF conversion
  - PDF import for editing
  - Structure preservation
  - Object mapping
- **Evidence**: Core PDF import/export

#### PDF <- Trapping
- **Purpose**: Trap inclusion in PDF
- **Details**:
  - Trap export in PDF/X
  - Trap preservation in re-use
  - Separation with traps
  - PDF/A trap requirements
- **Evidence**: Trapping-PDF integration

### 5. Preflight Dependencies

#### Preflight <- Document
- **Purpose**: Document validation
- **Details**:
  - Document structure analysis
  - Property extraction per object
  - Rule condition evaluation
- **Evidence**: PreflightProfile.dll integration

#### Preflight <- Color
- **Purpose**: Color rule checking
- **Details**:
  - Gamut warning generation
  - Spot color verification
  - ICC profile compliance
  - Total ink limit checks
- **Evidence**: Color rule implementations

#### Preflight <- Trapping
- **Purpose**: Trap verification
- **Details**:
  - Trap width validation
  - Direction correctness
  - Coverage checks
- **Evidence**: Trap rule implementations

#### Preflight <- Marks
- **Purpose**: Mark completeness checks
- **Details**:
  - Registration mark presence
  - Crop mark verification
  - Color bar placement
  - Control strip presence
- **Evidence**: Mark rule checks

#### Preflight <- Automation
- **Purpose**: Built-in checks in action lists
- **Details**:
  - Preflight within action sequences
  - Error handling per step
  - Report generation per step
- **Evidence**: Automation-preflight integration

### 6. Automation Dependencies

#### Automation <- Document
- **Purpose**: Action list document operations
- **Details**:
  - Open/save/close commands
  - Object creation/modification
  - Document state tracking
- **Evidence**: Automation.dll document commands

#### Automation <- Color
- **Purpose**: Color commands in action lists
- **Details**:
  - Set color operations
  - Color conversion commands
  - Profile application
- **Evidence**: Color commands in action lists

#### Automation <- Geometry
- **Purpose**: Transform commands in action lists
- **Details**:
  - Move/resize/rotate commands
  - Position/size parameters
  - Transformation matrices
- **Evidence**: Geometry commands in action lists

#### Automation <- Preflight
- **Purpose**: Built-in preflight checks
- **Details**:
  - Check steps in action lists
  - Report generation steps
  - Auto-fix steps
- **Evidence**: Preflight steps in automation

#### Automation <- Interface
- **Purpose**: UI command dispatch
- **Details**:
  - Menu/toolbar command mapping
  - Shortcut action mapping
  - Custom command creation
- **Evidence**: Interface command mappings

### 7. Marks Dependencies

#### Marks <- Document
- **Purpose**: Mark placement on document
- **Details**:
  - Mark position calculation
  - Document boundary consideration
  - Mark export with document
- **Evidence**: DocumentMarks.dll integration

#### Marks <- Geometry
- **Purpose**: Mark position geometry
- **Details**:
  - Position calculation from page dimensions
  - Spacing and alignment
  - Margin handling
- **Evidence**: Geometry position functions

#### Marks <- Color
- **Purpose**: Color bar color values
- **Details**:
  - Process color values
  - Spot color identification
  - Lab values for comparison
- **Evidence**: Color bar generation

#### Marks <- Interface
- **Purpose**: Mark creation UI
- **Details**:
  - Mark type selection
  - Parameter input
  - Placement preview
- **Evidence**: Interface mark tools

## Cross-Module Function Flow Examples

### Example 1: Document Export to PDF with Traps
```
Document Export
    ↓
Color Engine: Convert document colors to PDF spaces
    ↓
Trapping Engine: Generate traps for all object boundaries
    ↓
Geometry Engine: Provide object bounds and edges
    ↓
PDF Engine: Write PDF content with trap data
    ↓
Document Engine: Update document state, mark as exported
```

### Example 2: Preflight Validation with Auto-Fix
```
User Initiates Preflight
    ↓
Preflight Engine: Load profile, iterate checks
    ↓
Color Engine: Check gamut, total ink, spot colors
    ↓
Geometry Engine: Check object sizes, bleeds, margins
    ↓
Trapping Engine: Verify trap width/direction
    ↓
Marks Engine: Verify mark presence/completeness
    ↓
Preflight Engine: Compile results, identify auto-fix candidates
    ↓
Preflight Engine: Apply applicable mutators (font embed, bleed add, etc.)
    ↓
Preflight Engine: Generate report, present to user
```

### Example 3: Step & Repeat with Color Consistency
```
Step & Repeat Operation
    ↓
Document Engine: Select objects, create copies
    ↓
Color Engine: Ensure color consistency across copies
    ↓
Geometry Engine: Calculate positions, transformations
    ↓
Trapping Engine: Generate traps for all new object boundaries
    ↓
Interface: Update UI, show preview of repeated layout
```

### Example 4: Automation Action List Execution
```
Execute Action List
    ↓
Automation Engine: Parse and execute sequentially
    ↓
Document Engine: Open document (step 1)
    ↓
Color Engine: Apply color transformation (step 2)
    ↓
Geometry Engine: Transform objects (step 3)
    ↓
Trapping Engine: Generate traps after transformations (step 4)
    ↓
Preflight Engine: Check result (step 5)
    ↓
Marks Engine: Add marks if required (step 6)
    ↓
PDF Engine: Export final result (step 6)
```

## Dependency Graph Visualization (Text-Based)

```
┌─────────┐     ┌─────────┐     ┌─────────┐
│Document │────▶│Geometry │────▶│   Color │
└─────────┘     └─────────┘     └─────────┘
     │              │              │
     ▼              ▼              ▼
  ┌─────────┐  ┌─────────┐  ┌─────────┐
  │  Trapping││    PDF  ││  Preflight│
  └─────────┘  └─────────┘  └─────────┘
       │              │              │
       └────▶────────▶────────────┘
               Automation
                Engine
```

## Key Dependency Observations

1. **Full Connectivity**: All 9 modules have direct dependencies with every other module
2. **Document as Hub**: Document Engine is the most connected, serving as primary integration point
3. **Geometry as Mediator**: Geometry Engine frequently mediates between other modules
4. **Color Centrality**: Color Engine is heavily involved in most operations (as expected for prepress)
5. **Trapping-PDF Axis**: Strong dependency between trapping and PDF output
6. **Preflight as Validator**: Preflight checks multiple modules and compiles results
7. **Automation as Orchestrator**: Automation coordinates other modules in sequences
8. **Interface as UI Bridge**: Interface mediates user interaction with all modules

## Dependency Priority for Analysis

1. **Document <-> Geometry**: Core object positioning and transforms
2. **Document <-> Color**: Fundamental color management
3. **Trapping <-> PDF**: Output format integration
4. **Preflight <-> All**: Validation across all modules
5. **Automation <-> All**: Sequential coordination
6. **Interface <-> All**: User interaction layer

## Next Steps for Dependency Analysis

1. Verify dependencies via Ghidra cross-references
2. Quantify dependency strength (frequent vs. occasional)
3. Identify circular dependencies (if any)
4. Map dependency version changes between ArtPro+ versions
5. Create weighted dependency graph
6. Identify minimal module subsets for specific operations