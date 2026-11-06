# ArtPro+ v250 - Step & Repeat Analysis

## Overview
The Step & Repeat module in ArtPro+ v250 handles the creation of multiple copies of objects in a structured layout on a page or form. This is essential for panel layouts, business cards, label sheets, and other repetitive designs common in prepress.

## Key DLLs
- **StepAndRepeat.dll** (311,336 bytes) - Primary step and repeat module
- **StepAndRepeatCAD.dll** (458,792 bytes) - CAD-oriented step and repeat
- **StudioCommon.dll** (334,888 bytes) - Common studio operations
- **StudioDesignerCoreController.dll** (987,176 bytes) - Design controller
- **StudioDesignerCoreRenderEngine.dll** (865,680 bytes) - Rendering engine

## Step & Repeat Operations

### 1. Basic Parameters
#### Repetition Grid Parameters
- **Number of Repetitions**: 
  - Columns (horizontal count)
  - Rows (vertical count)
  - Total count = columns × rows

- **Distances**:
  - **Horizontal Distance**: Spacing between columns
  - **Vertical Distance**: Spacing between rows

- **Margins**:
  - **Margin Left**: Space from page left edge to first column
  - **Margin Right**: Space from page right edge to last column
  - **Margin Top**: Space from page top edge to first row
  - **Margin Bottom**: Space from page bottom edge to last row

- **Gap**:
  - **Gap X**: Space between adjacent objects in same row
  - **Gap Y**: Space between adjacent objects in same column
  - Gap = Distance - Object Width/Height (depending on interpretation)

### 2. Rotation
#### Rotation Parameters
- **Angle**: Rotation in degrees (typically 0-360)
- **Center**: Point around which rotation occurs
  - Page center
  - Selected object center
  - Custom point

#### Rotation Modes
- **No Rotation**: 0° or 360° (aligned grid)
- **Uniform Rotation**: All objects rotated by same angle
- **Variable Rotation**: Each row/column has different angle

#### Rotation Algebra
```
Rotate point (x, y) by angle θ around origin:
x' = x*cos(θ) - y*sin(θ)
y' = x*sin(θ) + y*cos(θ)
```

#### Rotation with Translation
- Rotate around center, then translate to position

### 3. Stagger (Offset)
#### Stagger Parameters
- **X Stagger**: Horizontal offset per row
- **Y Stagger**: Vertical offset per column
- **Stagger Pattern**: Periodic or random offset

#### Stagger Patterns
- **Linear Stagger**: Constant offset per step
- **Sinusoidal Stagger**: Sinusoidal variation
- **Random Stagger**: Random offset within range
- **Custom Pattern**: User-defined offset sequence

#### Stagger Algorithm
```
For step n (0-indexed):
x_offset = n * stagger_x
y_offset = f(n) * stagger_y  where f(n) is pattern function
```

#### Common Patterns
- **Brick Stagger**: Alternating rows offset by half-object width
- **Triangular Stagger**: Equilateral triangle pattern
- **Packing**: Optimized packing without gaps

### 4. Rollover (Wrapping)
#### Rollover Parameters
- **Row Rollover**: Move to next row after N objects
- **Column Rollover**: Move to next column after N objects
- **Wrap Behavior**: How wrap interacts with grid edges

#### Rollover Modes
- **Hard Boundary**: Stop at grid edge (no wrap)
- **Soft Wrap**: Wrap to opposite side
- **Spiral**: Continue in spiral pattern
- **Custom**: User-defined wrap rule

#### Rollover Algebra
```
If (column + rollover_column) >= total_columns:
    column = 0
    row = row + 1
If (row + rollover_row) >= total_rows:
    Stop (or wrap depending on mode)
```

### 5. Head Turn (Orientation Flip)
#### Head Turn Parameters
- **Horizontal Flip**: Mirror across vertical axis
- **Vertical Flip**: Mirror across horizontal axis
- **Both Axes**: Mirror across both

#### Head Turn Operations
- **Flip X**: x' = -x (relative to center)
- **Flip Y**: y' = -y (relative to center)
- **Rotation + Flip**: Combine with rotation

#### Head Turn in Step & Repeat
- Apply flip to each replicated object
- Maintain consistent orientation pattern
- Optional per-row/column variation

### 6. Filling Sequence
#### Fill Order Options
- **Left-to-Right, Top-to-Bottom**: Row-major order
- **Right-to-Left, Bottom-to-Top**: Reverse row-major
- **Column-Major**: Fill columns first, then rows
- **Spiral**: Clockwise or counter-clockwise spiral
- **Zig-Zag**: Back-and-forth pattern

#### Fill Sequence Algorithms

##### Row-Major (Left-to-Right, Top-to-Bottom)
```
for row in 0..total_rows-1:
    for col in 0..total_cols-1:
        place object at (col, row)
```

##### Column-Major (Top-to-Bottom, Left-to-Right)
```
for col in 0..total_cols-1:
    for row in 0..total_rows-1:
        place object at (col, row)
```

##### Spiral Fill
```
Define spiral starting point and direction
for step in 1..total:
    place object at current position
    move to next spiral position
```

### 7. Anchor
#### Anchor Parameters
- **Anchor Point**: Reference point for positioning
  - Top-Left
  - Center
  - Bottom-Right
  - Custom point

- **Alignment**: How objects align to anchor
  - Align-to-anchor center
  - Align-to-anchor edges

#### Anchor Operations
- **Set Anchor**: Define reference point
- **Calculate Position**: Position based on anchor
- **Reset Anchor**: Return to default

#### Anchor in Repetition
- Each row/column can have independent anchor
- Or global anchor for entire grid

### 8. Use Bleed
#### Bleed Integration
- **Use Bleed**: Include bleed in repetition
- **Bleed Distance**: How far beyond edge
- **Bleed per Edge**: Different bleed per side

#### Bleed Algorithm
```
For each object at position (x, y):
    If use_bleed:
        x_extend = x + bleed_right
        y_extend = y + bleed_bottom
        Render with extended bounds
```

#### Bleed Benefits
- Ensures no white edges after trimming
- Accounts for press misregistration
- Standard prepress requirement (typically 3mm-5mm)

### 9. One-Up Mask
#### One-Up Operations
- **Single Copy**: Generate one copy for proofing
- **Mask Generation**: Create mask for proof output
- **Positioning**: Align mask to original layout

#### One-Up Mask Uses
- Proof generation without full repetition
- Client review of single layout
- Proofing mark generation

### 10. Aproveitamento de Chapa (Imposition)
#### Imposition Operations
- **Sheet Optimization**: Maximize usage of printing sheet
- **Nesting**: Pack objects efficiently
- **Signature Layout**: Booklet signature arrangement
- **Perforation/Cut Lines**: Add cutting guidelines
- **Drill Holes**: For binding

#### Imposition Types
- **Full Sheet**: One-up on full sheet
- **Half Sheet**: Two-up, split sheet
- **Quarter Sheet**: Four-up, etc.
- **Custom N-up**: N objects per sheet
- **Brochure**: Folded signature layout

#### Imposition Parameters
- **Sheet Size**: Paper dimensions
- **Margins**: Printable area within sheet
- **Bleed**: Bleed area
- **Cutting Marks**: Crop marks, perforations
- **Binding**: Fold line placement

#### Common Imposition Schemes
- **Work-and-Turn**: Print both sides, cut, turn
- **Work-and-Fold**: Print, fold into signatures
- **Perfect Binding**: Spine placement
- **Saddle Stitch**: Staple fold

## Architecture Summary

### Step & Repeat Modules and Functions

| Module | Size | Likely Functions |
|--------|------|-----------------|
| StepAndRepeat.dll | 311,336 | Core step and repeat |
| StepAndRepeatCAD.dll | 458,792 | CAD-oriented variants |
| StudioCommon.dll | 334,888 | Common operations |
| StudioDesignerCoreController.dll | 987,176 | Design control |
| StudioDesignerCoreRenderEngine.dll | 865,680 | Rendering preview |

### Step & Repeat Parameter Matrix

| Parameter | Type | Typical Range | Default |
|-----------|------|---------------|---------|
| Columns | Integer | 1-100+ | 1 |
| Rows | Integer | 1-100+ | 1 |
| Horz Distance | Length | 1mm-1000mm | 0 |
| Vert Distance | Length | 1mm-1000mm | 0 |
| Margin Left | Length | 0-1000mm | 0 |
| Margin Right | Length | 0-1000mm | 0 |
| Margin Top | Length | 0-1000mm | 0 |
| Margin Bottom | Length | 0-1000mm | 0 |
| Gap X | Length | 0-100mm | 0 |
| Gap Y | Length | 0-100mm | 0 |
| Rotation Angle | Angle | 0-360° | 0° |
| Stagger X | Length | 0-100mm | 0 |
| Stagger Y | Length | 0-100mm | 0 |
| Fill Sequence | Enum | Various | LR-TB |
| Anchor Point | Enum | 9 positions | Center |
| Use Bleed | Boolean | True/False | False |
| One-Up Mask | Boolean | True/False | False |
| Imposition Scheme | Enum | Various | None |

### Layout Algorithms

#### Rectangular Grid Algorithm
```
for row = 0 to rows-1:
    for col = 0 to cols-1:
        x = margin_left + col * (obj_width + gap_x)
        y = margin_top + row * (obj_height + gap_y)
        place object at (x + rotation, y + rotation)
```

#### Hexagonal Grid Algorithm
```
for row = 0 to rows-1:
    for col = 0 to cols-1:
        x = margin_left + col * (obj_width + gap_x)
        y = margin_top + row * (obj_height * 0.75 + gap_y)
        if row is odd:
            x += obj_width / 2  # staggered
        place object at (x, y)
```

#### Spiral Algorithm
```
center = (start_x, start_y)
direction_cycle = [(0,1), (1,0), (0,-1), (-1,0)]  # N, E, S, W
direction_idx = 0
steps_in_dir = 1
steps_taken = 0
turns_since_direction_change = 0
current_pos = center
current_dir_idx = 0

for i = 1 to total_objects:
    place object at current_pos
    steps_taken++
    if steps_taken >= steps_in_dir:
        direction_idx = (direction_idx + 1) % 4
        turns_since_direction_change++
        if turns_since_direction_change % 2 == 0:
            steps_in_dir++
        steps_taken = 0
    dx, dy = direction_cycle[direction_idx]
    current_pos = (current_pos.x + dx, current_pos.y + dy)
```

### Integration with Other Engines

#### Step & Repeat <- Document Engine
- Object selection for repetition
- Document page boundaries
- Undo/redo of repetition operations
- Export with step-and-repeat data

#### Step & Repeat <- Geometry Engine
- Object positioning calculations
- Transformation matrices
- Distance and angle computations
- Bounds calculation for repeated objects

#### Step & Repeat <- Color Engine
- Color copies consistency
- Tint variations per step
- Spot color handling in repetition

#### Step & Repeat <- Trapping Engine
- Trap generation for all object boundaries
- Corner handling at overlaps
- Trap width consistency across repetitions

#### Step & Repeat <- PDF Engine
- PDF export with step-and-repeat data
- Preservation of vector data
- Imposition integration
- PDF/X compliance for n-up

### Algorithm Investigation Priority

1. **Grid Layout Algorithm**: Core placement calculation
2. **Stagger Pattern Implementation**: Offset computation
3. **Fill Sequence Order**: Enumeration logic
4. **Rollover Behavior**: Wrap/stop logic
5. **Imposition Nesting**: Sheet optimization
6. **Bleed Integration**: Edge handling
7. **Anchor Positioning**: Reference point calculation

### Next Steps for Step & Repeat Analysis

1. Use Ghidra to decompile StepAndRepeat.dll functions
2. Identify core layout algorithm
3. Trace parameter validation routines
4. Document grid and stagger computation
5. Analyze fill sequence enumeration
6. Investigate imposition nesting logic
7. Map bleed and margin handling
8. Examine anchor point calculation

## Observations from Import Log
- **StepAndRepeat.dll exports extracted**: Core functions available
- **StepAndRepeatCAD.dll**: CAD-specific variants with likely stricter parameters
- **Studio* modules**: Suggest integration with broader Esko studio suite
- Module sizes indicate substantial logic for layout computation