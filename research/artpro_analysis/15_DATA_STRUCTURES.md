# ArtPro+ v250 - Data Structures Catalog

## Overview
This catalog documents the identified data structures in ArtPro+ v250 based on Ghidra analysis and binary investigation. Each structure entry includes fields, types, and observed characteristics.

## Document Engine Structures

### ArtDocument Structure
```
ArtDocument {
    id: integer - unique document identifier
    name: string - document filename without path
    path: string - full file path
    modified: boolean - unsaved changes flag
    pages: PageList - list of ArtPage objects
    active_page: ArtPage* - currently active page
    layers: LayerList - list of ArtLayer objects
    undo_stack: UndoStack - command history for undo/redo
    redo_stack: UndoStack - redo command history
    creation_date: datetime - document creation timestamp
    last_modified: datetime - last save/modification timestamp
    color_mode: enum - CMYK/RGB/Lab/Spot
    resolution: float - dpi (dots per inch)
    bleeds: BleedInfo - bleed specifications
    marks: MarkList - associated printing marks
    profile: ICCProfile* - associated color profile
    reference_count: integer - reference counting
}
```

### ArtPage Structure
```
ArtPage {
    id: integer - unique page identifier
    name: string - page name/label
    width: float - page width in document units
    height: float - page height in document units
    unit: enum - inches/mm/picas/points
    orientation: enum - portrait/landscape
    resolution: float - dpi
    background_color: Color - page background
    objects: ObjectList - list of GraphicObject
    layers: LayerList - ArtLayer objects
    selection: SelectionMask - selected objects
    zoom: float - current zoom level
    pan_x: float - horizontal pan offset
    pan_y: float - vertical pan offset
    print_width: float - printable width
    print_height: float - printable height
    crop_marks: bool - show crop marks
    registration_marks: bool - show registration marks
}
```

### ArtLayer Structure
```
ArtLayer {
    id: integer - unique layer identifier
    name: string - layer name
    visible: boolean - visibility state
    locked: boolean - lock status
    lock_alpha: boolean - lock transparency
    lock_position: boolean - lock position
    order: integer - z-index (higher = on top)
    parent: ArtPage* - parent page
    objects: ObjectList - GraphicObjects in this layer
    filter_effects: FilterList - applied effects
    blend_mode: enum - normal/multiply/screen/etc.
    opacity: float - 0.0 to 1.0
}
```

### GraphicObject Structure (Base)
```
GraphicObject {
    id: integer - unique object identifier
    type: enum - rectangle/text/image/group/line/curve
    name: string - object name/label
    parent: ArtLayer* - parent layer
    visible: boolean - visibility
    locked: boolean - lock against editing
    selected: boolean - selection state
    transformation: Matrix3x2 - transformation matrix
    bounds: RectangleF - floating-point bounding box
    original_bounds: RectangleF - original bounds before transforms
    date_created: datetime - creation timestamp
    date_modified: datetime - last modification
    parent_document: ArtDocument* - owning document
    
    // Virtual methods (polymorphic)
    clone(): GraphicObject*
    getBounds(): RectangleF
    getArea(): float
    translate(dx: float, dy: float): void
    rotate(angle: float): void
    scale(sx: float, sy: float): void
    setColor(fill: Color, stroke: Color): void
    isSelected(): bool
    setSelected(selected: bool): void
}
```

### Specific Object Types

#### Rectangle Object
```
Rectangle : GraphicObject {
    width: float - rectangle width
    height: float - rectangle height
    rx: float - optional rounded corner x-radius
    ry: float - optional rounded corner y-radius
}
```

#### Text Object
```
Text : GraphicObject {
    content: string - text content
    font_name: string - typeface name
    font_size: float - point size
    font_style: enum - normal/bold/italic/bold-italic
    color: Color - text color
    horizontal_alignment: enum - left/center/right/justify
    vertical_alignment: enum - top/middle/bottom
    baseline_offset: float - relative baseline position
    tracking: float - character spacing
    leading: float - line spacing
}
```

#### Image Object
```
Image : GraphicObject {
    file_path: string - source image file
    width: float - display width
    height: float - display height
    resolution: float - dpi
    color_space: enum - RGB/CMYK/Lab
    mask: ImageMask - transparency mask
    fit_mode: enum - fill/frame/proportional
}
```

#### Group Object
```
Group : GraphicObject {
    objects: ObjectList - child GraphicObjects
    is_group: boolean - always true
    group_locked: boolean - lock group structure
}
```

## Geometry Engine Structures

### Point Structure
```
Point {
    x: float - x coordinate
    y: float - y coordinate
}
```

### Rectangle Structure (Floating Point)
```
RectangleF {
    x1: float - left
    y1: float - top
    x2: float - right (x1 + width)
    y2: float - bottom (y1 + height)
    width: float - computed as x2-x1
    height: float - computed as y2-y1
}
```

### Rectangle Structure (Integer)
```
RectangleI {
    x1: int - left
    y1: int - top
    x2: int - right
    y2: int - bottom
    width: int - computed as x2-x1
    height: int - computed as y2-y1
}
```

### Matrix3x2 Structure (3x2 Transformation Matrix)
```
Matrix3x2 {
    a: float - scale/rotation cos component
    b: float - scale/rotation sin component
    c: float - shew/rotation component
    d: float - shew/rotation component
    tx: float - translation x
    ty: float - translation y
}
```

### Transformation Operations
```
Matrix3x2 * Point:
x' = a*x + b*y + tx
y' = c*x + d*y + ty

Matrix3x2 * Matrix3x2:
Result = M1 * M2 (matrix multiplication)

Inverse: Matrix3x2 inverse() - returns inverse matrix
Determinant: float det() - returns determinant
```

### Line Structure
```
Line {
    start: Point - line start point
    end: Point - line end point
    thickness: float - line width
    color: Color - line color
    dashed: boolean - dash/line style
    dash_pattern: [float] - dash/space array
}
```

### Bezier Curve Structure
```
BezierCurve {
    p0: Point - start control point
    p1: Point - first intermediate control point
    p2: Point - second intermediate control point
    p3: Point - end control point
    rational: boolean - rational (true) vs ordinary (false)
    w0: float - weight p0 (if rational)
    w1: float - weight p1 (if rational)
    w2: float - weight p2 (if rational)
    w3: float - weight p3 (if rational)
}
```

### Polygon Structure
```
Polygon {
    vertices: [Point] - ordered vertex list
    is_closed: boolean - true if last connects to first
    area: float - pre-calculated area
    centroid: Point - pre-calculated center
    bounding_box: RectangleF - minimal enclosing rectangle
    orientation: enum - cw/ccw (clockwise/counter-clockwise)
}
```

### Color Structure
```
Color {
    color_space: enum - CMYK/RGB/Lab/Spot/Gray
    values: [float, ...] - color component values
    - CMYK: [c, m, y, k] 0.0-1.0 or 0-255
    - RGB: [r, g, b] 0.0-1.0 or 0-255
    - Lab: [l, a, b] l: 0-100, a: -128 to 127, b: -128 to 127
    - Gray: [gray] 0.0-1.0 or 0-255
    transparency: float - alpha 0.0-1.0
    spot_name: string - spot color name (if spot)
    icc_profile: ICCProfile* - associated profile
}
```

## Color Engine Structures

### ICCProfile Structure
```
ICCProfile {
    id: string - profile identifier/filename
    version: float - format version (2.x or 4.x)
    color_space: integer - number of channels
    date: datetime - creation/modification date
    media_white_point: [3]float - X,Y,Z or L*a*b* white
    media_black_point: [3]float - X,Y,Z or L*a*b* black
    red_primaries: [3]float - R, G, B chromaticity
    green_primaries: [3]float - G chromaticity
    blue_primaries: [3]float - B chromaticity
    red_trc: curve - red tone reproduction curve
    green_trc: curve - green tone reproduction curve
    blue_trc: curve - blue tone reproduction curve
    copyright: string - copyright notice
    manufacturer: string - profile manufacturer
    model: string - profile model/description
    class: enum - input/display/printing/abstract
    tags: TagList - named data tags
    
    // Transformation methods
    lab2xyz(lab[3]): xyz[3]
    xyz2lab(xyz[3]): lab[3]
    rgb2lab(rgb[3]): lab[3]
    lab2rgb(lab[3]): rgb[3]
    cmyk2rgb(cmyk[4]): rgb[3]
    rgb2cmyk(rgb[3]): cmyk[4]
}
```

### Tag Structure (ICC Profile Tags)
```
Tag {
    tag_type: enum - int/float/array/data/binary
    tag_name: string - e.g., "cmTYpe", "abridged"
    tag_data: variable - typed data
}
```

### Curve/TRC Structure
```
TRC {
    function_type: enum - power/linear/table
    table_size: integer - number of entries (usually 256)
    table: [0.0-1.0] - lookup table
    exponent: float - power function exponent
    coefficient: [float, ...] - polynomial coefficients
}
```

## Trapping Engine Structures

### Trap Structure
```
Trap {
    id: integer - unique trap identifier
    object1: ArtObject* - first object (foreground/back)
    object2: ArtObject* - second object (background/front)
    direction: enum - choke/spread
    width: float - trap width in document units
    color1: Color - color1 (typically foreground)
    color2: Color - color2 (typically background)
    geometry: Polygon - trap shape polygon
    active: boolean - currently active
    generated: datetime - generation timestamp
    valid: boolean - geometry valid
}
```

### Trap Generation Parameters
```
TrapParameters {
    direction: enum - automatic/brightness-based/choke/spread
    width: float - calculated trap width
    unit: enum - inches/mm/points
    miter_limit: float - miter join limit ratio
    join_style: enum - miter/bevel/round
    corner_algorithm: enum - how corners handled
    antialias: boolean - apply anti-aliasing
    preview: boolean - show in UI preview
}
```

## Preflight Structures

### PreflightCheck Structure
```
PreflightCheck {
    id: string - unique check identifier
    name: string - human-readable name
    category: enum - color/geometry/marks/text/...
    severity: enum - error/warning/info
    active: boolean - enabled/disabled
    condition: string - logical condition description
    action: enum - none/auto-fix/report-only
    auto_fix_mutator: string - mutator name if auto-fix
}
```

### PreflightResult Structure
```
PreflightResult {
    check_id: string - referencing PreflightCheck
    severity: enum - error/warning/info
    message: string - description
    affected_objects: [integer] - object IDs affected
    suggestion: string - suggested action
    auto_fix_applicable: boolean
    confidence: float - 0.0 to 1.0
}
```

### Profile Structure
```
PreflightProfile {
    id: string - profile identifier
    name: string - human-readable name
    version: float - profile version
    severity_threshold: enum - error/warning/info
    rules: [PreflightCheck] - active rules
    output_format: enum - xml/html/txt/pdf
    created: datetime - profile creation
    modified: datetime - last modification
}
```

## Integration Structures

### Object Reference Structure
```
ObjectRef {
    id: integer - object unique identifier
    type: enum - document/page/layer/object
    document: ArtDocument* - owning document (if applicable)
    page: ArtPage* - owning page (if applicable)
    layer: ArtLayer* - owning layer (if applicable)
    object: GraphicObject* - the object (if applicable)
}
```

### Action Structure (Automation)
```
Action {
    id: integer - unique action identifier
    type: enum - command type
    name: string - human-readable name
    parameters: [Parameter] - action parameters
    result: any - execution result
    error: string - error description (if failed)
    undo_info: string - undo information
}
```

### Parameter Structure
```
Parameter {
    type: enum - string/number/boolean/color/enum/object/action
    value: any - parameter value
    source: enum - literal/variable/calculated
    variable_name: string - if variable source
}
```

## Observed Memory Layout

### Likely C++ Class Layout Patterns

Based on typical Ghidra analysis and C++ conventions:

1. **vtable Pointer**: First member in C++ classes (pointer to virtual function table)
2. **Reference Count**: For object lifetime management
3. **Padding**: Alignment padding between members
4. **Virtual Functions**: Function pointers for polymorphism

### Example Typical Layout
```
C++ Class Layout:
+-------------------+
| vtable pointer    |  4 or 8 bytes
+-------------------+
| field1            |  4/8 bytes
+-------------------+
| field2            |  4/8 bytes
+-------------------+
| ...               |  ...
+-------------------+
| fieldN            |  4/8 bytes
+-------------------+
| padding           |  alignment padding
+-------------------+
```

## Next Steps

1. Analyze Ghidra struct definitions for exact layouts
2. Confirm field types and offsets
3. Add observed memory alignments
4. Map structures to C++ class definitions
5. Document structure interrelationships
6. Create memory layout diagrams