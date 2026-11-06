# ArtPro+ v250 - Implementation Specification

## Overview
This specification provides independent implementation guidelines for ArtPlus based on the analysis of ArtPro+ v250. Each functionality entry describes behavior, data structures, inputs, outputs, algorithms, and implementation notes. All implementations must be original and not copy proprietary Esko components.

## 1. Document Engine Specification

### 1.1 Behavior Observed/Compromised
- Document creation with configurable dimensions and color mode
- Page management with layer hierarchy
- Object selection and transformation
- Undo/Redo command history support
- Document serialization/deserialization

### 1.2 Data Structures Necessary
- **ArtDocument**: id, name, path, modified flag, pages list, active_page, layers list, undo/redo stacks, color_mode, resolution, bleeds, marks, profile, reference_count
- **ArtPage**: id, name, width, height, unit, orientation, resolution, background_color, objects list, layers list, selection mask, zoom, pan_x, pan_y, print dimensions, mark flags
- **ArtLayer**: id, name, visible, locked, lock_alpha, lock_position, order, parent, objects list, filter_effects, blend_mode, opacity
- **GraphicObject** (base): id, type, name, parent_layer, visible, locked, selected, transformation matrix, bounds, original_bounds, creation/modification dates, parent_document, virtual methods for clone, getBounds, getArea, translate, rotate, scale, setColor, isSelected, setSelected
- **Specific Types**: Rectangle (width, height, rx, ry), Text (content, font_name, font_size, font_style, color, alignments), Image (file_path, width, height, resolution, color_space, mask, fit_mode), Group (objects list, is_group, group_locked)

### 1.3 Entradas
- Document creation: dimensions, unit, color_mode, resolution
- Page creation: width, height, orientation
- Object addition: parent_page, parent_layer, object_type, geometry, style
- Layer creation: name, visible, locked
- Object transformation: object_id, transformation_type, parameters

### 1.4 Saídas
- Created ArtDocument/ArtPage/ArtObject references
- Serialization output (XML/binary format)
- Undo command records
- Selection state updates
- Modification flags

### 1.5 Regras de Negócio
- Undo/Redo must maintain consistent document state
- Layer hierarchy must be preserved during operations
- Object selection is exclusive to active layer unless otherwise specified
- Document modification flag set on any structural change
- Color mode applies globally but per-object overrides possible

### 1.6 Algoritmos Identificados
- Command pattern for undo/redo operations
- Object cloning with deep copy of transformation matrices
- Z-order management for layer stacking
- Boundary hit-testing using bounding box collision

### 1.7 Dependências
- Geometry Engine: for bounds calculations, transformations
- Color Engine: for object color management
- Interface: for UI event handling
- Preflight: for document validation

### 1.8 Casos Especiais
- Large document performance (lazy page loading)
- Undo stack memory management
- Multi-language support via localization
- Print preset configurations

### 1.9 Testes Necessários
- Document creation with various dimensions
- Undo/Redo cycle consistency
- Layer show/hide toggling
- Object selection/deselection
- Serialization/deserialization round-trip
- Multi-language string handling

### 1.10 Grau de Confiança
- High (observed through Ghidra import and binary analysis)

### 1.11 Prioridade de Implementação
- High - Foundation module for all others

### 1.12 Implementation Notes
- Implement original algorithms, do not copy Esko code
- Use MIT/BSD or similar open license
- Ensure cross-platform compatibility (Windows/macOS considered)
- Follow test-driven development approach

## 2. PDF Engine Specification

### 2.1 Behavior Observed/Compromised
- PDF opening and validation
- Object graph parsing with cross-reference tables
- Resource catalog extraction (fonts, images, colors)
- PDF export with vector preservation
- PDF/X and PDF/A compliance checking

### 2.2 Data Structures Necessary
- **PDF_Document**: object_table, catalog, page_tree, resources, metadata
- **PDF_Page**: mediabox, resources dictionary, contents stream, annotations list
- **PDF_Stream**: raw_data, filter_chain, decoded_cache, size_info
- **PDF_Font**: dictionary, font_program, encoding_map, to_unicode_cmap, embedding_status
- **PDF_Color**: color_space reference, icc_profile reference, device values, named color reference
- **PDF_Rectangle**: lower_left (x,y), upper_right (x,y)
- **PDF_Name**: atomic symbol
- **PDF_Array**: ordered list of objects
- **PDF_Dictionary**: key-value pairs
- **PDF_Boolean**: true/false
- **PDF_Null**: null value

### 2.3 Entradas
- PDF file path or memory buffer
- Page numbers for specific page extraction
- Export parameters (compression, metadata, security)
- Import filters (PDF version, compliance level)
- Color conversion parameters (working space, intent)

### 2.4 Saídas
- Parsed PDF structure object model
- Exported PDF file bytes
- Extracted content (text, vectors, images)
- Validation report (errors, warnings)
- Color space conversions
- Font embedding data

### 2.5 Regras de Negócio
- All fonts must be embedded or subset for PDF/A compliance
- Total ink limit must not exceed device capabilities
- Color spaces must be valid for output condition
- Embedded files must be legal for target PDF version
- Annotation preservation during editing

### 2.6 Algoritmos Identificados
- PDF object graph parsing with recursive dictionary traversal
- Cross-reference table reading and object resolution
- Stream decompression with filter chain composition
- Font program parsing (CFF/TrueType data)
- Color space conversion (RGB/CMYK/Lab/Device)
- ICC profile application and rendering intents
- Gamut checking against profile boundaries

### 2.7 Dependências
- Color Engine: for color space conversions
- Geometry Engine: for path data and transformations
- Preflight Engine: for compliance checking
- Interface: for PDF open/save dialogs

### 2.9 Casos Especiais
- Corrupted PDF recovery
- PDF/E (engineering) format support
- Large PDF optimization (object stream handling)
- PDF/A-1a, 1b, 2a, 2b, 2u, 3a, 3b, 3u levels
- Mixed content (vector + raster + text)

### 2.10 Testes Necessários
- PDF open/save round-trip with various versions
- Font embedding and subset verification
- Gamut checking against profiles
- PDF/X compliance verification
- PDF/A compliance checks
- Corrupted file recovery

### 2.11 Grau de Confiança
- Medium-High (strong evidence from import log and ArtProPDF.dll size)

### 2.12 Prioridade de Implementação
- High - Critical for output workflow

### 2.13 Implementation Notes
- Implement original PDF parser, do not reverse-engineer Esko code
- Support major PDF versions (1.3-2.0)
- Use open license (MIT/BSD or Apache 2.0)
- Consider using Little CMS 2/3 for color management
- Handle edge cases: corrupted streams, missing objects

## 3. Geometry Engine Specification

### 3.1 Behavior Observed/Compromised
- Line, curve, and polygon operations
- Boolean operations (union, intersect, subtract, XOR)
- Offset/inset calculations
- Transformations (translate, scale, rotate)
- Area and centroid calculations
- Intersection calculations

### 3.2 Data Structures Necessary
- **Point**: x, y floating-point coordinates
- **RectangleF**: x1, y1, x2, y2 floating-point bounds
- **RectangleI**: x1, y1, x2, y2 integer bounds
- **Matrix3x2**: 3x2 transformation matrix (a,b,c,d,tx,ty)
- **Line**: start_point, end_point, thickness, color, dashed_pattern
- **BezierCurve**: p0,p1,p2,p3 control points, rational flag, weights
- **Polygon**: vertices list, is_closed flag, area, centroid, bounding_box, orientation
- **Color**: color_space, values array, transparency

### 3.3 Entradas
- Point coordinates, dimensions
- Transformation matrices
- Polygon vertex lists
- Line specifications
- Boolean operation operands
- Offset distances and styles

### 3.4 Saídas
- Calculated areas, centroids
- Intersection points
- Transformed coordinates
- Offset polygons
- Flattened Bézier segments
- Bounding boxes

### 3.5 Regras de Negócio
- Area calculations use surveyor's formula (shoelace)
- Orientation determination: CW = negative area, CCW = positive
- Point-in-polygon: ray casting or winding number
- Line intersections: handle parallel/coincident cases
- Offset joins: miter/bevel/round with miter limit
- Curve flattening: within specified tolerance

### 3.6 Algoritmos Identificados
- de Casteljau algorithm for Bézier evaluation
- Shoelace formula for polygon area
- Ray casting for point-in-polygon
- Line-line intersection via Cramer's rule
- Polygon boolean operations (likely Clipper library)
- Offset computation (medial axis or buffer-based)
- Hexagonal grid layout algorithm
- Spiral fill enumeration

### 3.7 Dependências
- Color Engine: for filled/stroked shapes
- Interface: for user geometry input
- Document Engine: for object geometry access
- Trapping Engine: for edge detection and trap generation

### 3.8 Casos Especiais
- Numerical robustness for nearly-coincident edges
- Hole detection in polygons
- Self-intersecting polygon handling
- Zero-area polygon detection
- Floating-point precision issues
- Grid alignment snapping

### 3.9 Testes Necessários
- Area calculation for various polygon types
- Point-in-polygon for inside/outside test
- Line intersection for various orientations
- Boolean operations for union/intersect/subtract/XOR
- Offset generation for various polygon shapes
- Grid layout positioning accuracy
- Spiral fill correct ordering

### 3.10 Grau de Confiança
- High (standard computational geometry algorithms)

### 3.11 Prioridade de Implementação
- High - Core module for spatial operations

### 3.12 Implementation Notes
- Implement original geometry algorithms
- Use double precision for calculations
- Handle robust predicates for computational geometry
- Consider using established library (CGAL, Clipper) with modifications
- Document all algorithm assumptions and limitations

## 4. Color Engine Specification

### 4.1 Behavior Observed/Compromised
- RGB/CMYK/Lab color space conversions
- ICC profile loading and application
- Spot color management
- Delta E color difference calculation
- Gamut mapping and checking
- Total ink limiting and UCR/GCR

### 4.2 Data Structures Necessary
- **Color**: color_space (CMYK/RGB/Lab/Spot/Gray), values array, transparency, spot_name, icc_profile
- **ICCProfile**: id, version, color_space, date, media_white_point, media_black_point, red/green/blue primaries, red/green/blue TRCs, copyright, manufacturer, model, class, tags
- **TRC**: function_type, table_size, table[256], exponent, coefficients
- **Lab**: l (0-100), a (-128 to 127), b (-128 to 127)
- **XYZ**: x, y, z (floating-point)
- **DeltaEResult**: delta_e value, color difference type

### 4.3 Entradas
- Color values per colorspace
- ICC profile paths/files
- Target color space
- Gamut mapping intent
- Reference white point

### 4.4 Saídas
- Converted color values per target colorspace
- Delta E color difference value
- Gamut check result (in/out/boundary point)
- Spot color name/index
- Transformed color values
- Profile metadata

### 4.5 Regras de Negócio
- RGB->CMYK requires gamut mapping for printing
- Lab Deltas E measure perceptual difference
- ICC profiles must have valid header and tag table
- Total ink limiting typically 300-350% for process colors
- Spot colors use library database for matching
- Delta E 2000 most accurate, Delta E 1976 fastest

### 4.6 Algoritmos Identificados
- RGB -> XYZ: matrix multiplication with white point
- XYZ -> Lab: CIELAB transformation
- Lab -> XYZ: inverse CIELAB transformation
- RGB -> Lab: via XYZ intermediate
- Lab -> RGB: via XYZ intermediate
- CMYK -> RGB: inverse with gamut expansion
- CMYK -> Lab: via XYZ intermediate
- Delta E 1976: sqrt((L2-L1)^2 + (a2-a1)^2 + (b2-b1)^2)
- Delta E 1994: weighted version for graphics
- Delta E 2000: most accurate, complex weighting
- ICC profile gamut boundary checking
- UCR/GCR implementation with mode selection
- Total ink limiting with proportional scaling

### 4.7 Dependências
- Geometry Engine: for filled/stroked shapes
- PDF Engine: for PDF color space handling
- Trapping Engine: for trap color determination
- Preflight Engine: for color rule checking

### 4.8 Casos Especiais
- Device n-color (spot color + process)
- High gamut warning colors
- Black point compensation
- Multiple white points (D50, D65, etc.)
- Profile link connection chain
- Gray component replacement variants

### 4.9 Testes Necessários
- RGB/CMYK conversion round-trip
- Delta E calculation comparisons
- ICC profile loading and application
- Gamut checking accuracy
- Spot color library lookup
- UCR/GCR mode behavior
- Total ink limiting enforcement

### 4.10 Grau de Confiança
- High (CMS.dll indicates full CMM implementation, standard algorithms)

### 4.10 Prioridade de Implementação
- High - Critical for prepress quality

### 4.10 Implementation Notes
- Original implementation, do not copy Esko code
- Consider integrating Little CMS 2/3 (CMS.dll suggests capabilities)
- Use open color management license
- Support major ICC profile formats (2.x and 4.x)
- Handle edge cases: invalid profiles, out-of-range values
- Provide both perceptual and colorimetric rendering intents

## 5. Trapping Engine Specification

### 5.1 Behavior Observed/Compromised
- Automatic trap generation between overlapping objects
- Manual trap creation with user parameters
- Trap width calculation based on ink/paper/press
- Trap direction (choke/spread) determination
- Corner join styles (miter/bevel/round)
- Curve trap generation along Bézier paths

### 5.2 Data Structures Necessary
- **Trap**: id, object1, object2, direction (choke/spread), width, color1, color2, geometry_polygon, active, generated, valid
- **TrapParameters**: direction, width, unit, miter_limit, join_style, corner_algorithm, antialias, preview
- **EdgeProfile**: sampled points from object boundary, orientation

### 5.3 Entradas
- Two overlapping objects
- Trap width parameter
- Direction preference (choke/spread)
- Corner join style
- Curve parameters for boundary tracing

### 5.4 Saídas
- Generated trap geometry (polygon)
- Trap rendering instructions
- Width calculation result
- Direction determination result
- Corner treatment geometry
- Preview geometry data

### 5.5 Regras de Negócio
- Trap width typically 0.1pt to 0.5pt (0.35-1.75mm)
- Direction based on brightness (L* in Lab)
- Choke: trap underlaps foreground
- Spread: trap overflies foreground
- Corners: miter limit (typically 4:1 ratio)
- Anti-aliasing for smooth transitions
- Traps must not exceed ink limits

### 5.5 Algoritmos Identificados
- Boundary detection from object silhouettes
- Trap width calculation: k*sqrt(ps) or fixed user value
- Brightness comparison: L*ab comparison for direction
- Trap color: foreground for spread, background for choke
- Geometry generation: vector polygon for trap area
- Corner handling: miter/bevel/round with limit
- Curve tracing: along Bézier boundaries

### 5.7 Dependências
- Color Engine: for color values and Delta E
- Geometry Engine: for edge detection, polygon generation
- Document Engine: for object boundaries
- Interface: for trap parameter UI

### 5.8 Casos Especiais
- Same-color objects (no trap needed)
- Very thin objects (minimum trap width)
- Large ink coverage adjustments
- Spot color + process color trapping
- Transparent object trapping
- Variable width traps along boundaries
- Corner miter limit exceeded (bevel instead)

### 5.9 Testes Necessários
- Trap generation for various object overlaps
- Width calculation validation
- Direction determination correctness
- Corner join style handling
- Curve trap generation
- Trap preview rendering
- Auto vs. manual trap comparison

### 5.10 Grau de Confiança
- Medium (complex algorithm, observed but exact implementation needs verification)

### 5.10 Prioridade de Implementação
- Medium-High - Critical for print quality

### 5.9 Implementation Notes
- Original algorithm, do not copy Esko trapping code
- Consider Clipper library for polygon operations
- Use Lab color space for brightness-based direction
- Allow user override of automatic decisions
- Provide visual preview in UI
- Handle edge cases gracefully

## 6. Step & Repeat Specification

### 6.1 Behavior Observed/Compromised
- Rectangular and hexagonal grid layout
- Staggered row/column offsets
- Rotation of repeated objects
- Fill sequence enumeration (various patterns)
- Anchor point positioning
- Bleed integration
- Imposition/nesting for sheet optimization

### 6.2 Data Structures Necessary
- **StepRepeatParams**: columns, rows, horz_distance, vert_distance, margins[4], gap_x, gap_y, rotation_angle, stagger_x, stagger_y, fill_sequence, anchor_point, use_bleed, one_up_mask, imposition_scheme
- **GridPosition**: x, y, row, col
- **ImpositionLayout**: sheet_size, margins, bleed, nesting_objects, cut_lines, perforations

### 6.3 Entradas
- Object to repeat
- Grid parameters (columns, rows, spacing)
- Rotation angle
- Fill order preference
- Anchor point selection
- Bleed usage flag
- Imposition scheme (if applicable)

### 6.4 Saídas
- Array of positioned object copies
- Transformation matrices per copy
- Bounding box of entire layout
- Bleed area geometry (if enabled)
- Imposition layout (if specified)

### 6.5 Regras de Negócio
- Total count = columns × rows
- Horizontal distance between column centers
- Vertical distance between row centers
- Gap = distance - object dimension (depending on interpretation)
- Rotation applied uniformly or per-row/column
- Anchor point affects positional calculation origin
- Bleed extends objects beyond page edge
- Imposition optimizes sheet usage with cut/perforation lines

### 6.6 Algoritmos Identificados
- Rectangular grid: nested loops row/col with position calculation
- Hexagonal grid: staggered row offset (odd rows shifted)
- Fill sequences: row-major, column-major, spiral, zig-zag
- Stagger patterns: linear, sinusoidal, random, brick pattern
- Rollover: grid boundary wrap or stop
- Head turn: flip X, flip Y, or both across repetition
- Spiral fill: directional cycling with step increments

### 6.7 Dependências
- Geometry Engine: for position calculations, transformations
- Color Engine: for consistent color copies
- Interface: for parameter UI input
- Marks Engine: for mark generation in repetition

### 6.8 Casos Especiais
- Large repetition counts (100+ copies)
- Non-uniform spacing variations
- Irregular grid shapes (polygonal, custom)
- Mixed object types within one repetition
- Variable object sizes in repetition
- Cut line generation for imposition
- Perforation positioning for binding

### 6.9 Testes Necessários
- Grid layout with various parameters
- Stagger pattern correctness
- Fill sequence enumeration
- Rotation application
- Anchor point positioning
- Bleed extension correctness
- Imposition nesting optimization

### 6.10 Grau de Confiança
- Medium (observed patterns, exact algorithm details need verification)

### 6.11 Prioridade de Implementação
- Medium - Important for layout workflows

### 6.10 Implementation Notes
- Original implementation, do not copy Esko code
- Support common grid types: rectangular, hexagonal
- Provide visual preview of layout
- Allow parameter binding to document units
- Handle edge cases: zero gaps, negative distances
- Provide undo capability for layout changes

## 7. Marks Engine Specification

### 7.1 Behavior Observed/Compromised
- Registration mark generation
- Crop mark creation
- Color bar addition
- Control strip generation
- Custom mark creation and placement

### 7.2 Data Structures Necessary
- **RegistrationMark**: id, type (corner/side/cross), position (x,y), size, line_weight, fill, visible, printer_assignment
- **CropMark**: id, position (trim corner), direction, length, weight, visible
- **ColorBar**: id, color_space, color_values, label, location (x,y), width, height, show_label, target_values
- **ControlStrip**: id, strip_type, position (x,y), length, data (binary/string), labels list, visible
- **CustomMark**: id, name, type, geometry (vector), associated_data, position (x,y), size (width,height), visible

### 7.3 Entradas
- Page/form dimensions
- Mark type selection
- Parameter values (size, position, spacing)
- Color values for color bars
- Printer assignment

### 7.4 Saídas
- Mark geometry vectors
- Placement coordinates
- Rendering instructions
- Export data for PDF inclusion
- Mark position verification

### 7.5 Regras de Negócio
- Registration marks at page corners + optional side positions
- Crop marks extend beyond bleed area (typically 3-5mm)
- Color bars placed outside printable area (typically bottom)
- Control strips along sheet edge or interleaved
- Marks must be outside final trim area
- Mark line weights appropriate for output resolution
- Color bar labels readable at small sizes

### 7.6 Algoritmos Identificados
- Position calculation from page dimensions
- Standard mark template geometry
- Barcode/generation integration
- Custom mark geometry parsing
- Layout spacing and alignment

### 7.7 Dependências
- Geometry Engine: for position calculations
- Color Engine: for color bar color values
- Interface: for mark creation UI
- Preflight Engine: for mark completeness checks

### 7.8 Casos Especiais
- Different page sizes (A4, A3, letter, custom)
- Multiple pages per document
- Different printer requirements (offset/digital)
- Mark language/localization
- Minimum mark size for output resolution
- Mark visibility toggling

### 7.8 Testes Necessários
- Registration mark placement for various page sizes
- Crop mark extension beyond bleed
- Color bar color accuracy
- Control strip data format
- Custom mark geometry handling
- Mark export to PDF/X

### 7.10 Grau de Confiança
- Medium (observed through DocumentMarks.dll analysis)

### 7.11 Prioridade de Implementação
- Medium - Important for print quality control

### 7.10 Implementation Notes
- Original mark generation, do not copy Esko code
- Support standard mark types: registration, crop, color bars, control strips
- Marks must be printable outside trim area
- Provide UI for mark parameter input
- Consider PDF/X mark requirements
- Allow custom mark creation

## 8. Preflight Specification

### 8.1 Behavior Observed/Compromised
- Rule-based document validation
- Profile loading and application
- Error/warning/information reporting
- Automatic corrections (mutators)
- Report generation in multiple formats

### 8.2 Data Structures Necessary
- **PreflightCheck**: id, name, category, severity, active, condition, action, auto_fix_mutator
- **PreflightResult**: check_id, severity, message, affected_objects, suggestion, auto_fix_applicable, confidence
- **PreflightProfile**: id, name, version, severity_threshold, rules list, output_format, created, modified
- **Mutator**: applicability check, execution routine, undo routine, confidence score

### 8.3 Entradas
- Document to validate
- Profile name or path
- Severity threshold
- Output format preference
- Mutator enablement

### 8.4 Saídas
- Check results list (severity, message, objects)
- Profile loading success/failure
- Auto-fix applied status
- Report file/string
- Priority-ordered issue list

### 8.5 Regras de Negócio
- Error severity: must fix before output
- Warning severity: should fix, may proceed with caution
- Information severity: for awareness only
- Profile severity threshold determines minimum reported
- Auto-fix applicable based on mutator availability
- Report formats: HTML, XML, TXT, PDF
- Rules can be enabled/disabled per profile

### 8.6 Algoritmos Identificados
- Rule condition evaluation against document state
- Profile XML/JSON parsing (likely XML based on size)
- Check result compilation and priority ordering
- Mutator applicability determination
- Report formatting per output format
- Severity threshold comparison

### 8.7 Dependências
- Document Engine: for document property access
- Color Engine: for color rule checking
- Geometry Engine: for geometric rule checking
- Trapping Engine: for trap verification
- Marks Engine: for mark completeness
- Automation Engine: for built-in check steps

### 8.8 Casos Especiais
- Large documents with many objects
- Profile conflicts between rules
- Mutator undoability requirements
- Batch preflight for multiple documents
- Custom user-defined rules
- Integration with external QA systems

### 8.9 Testes Necessários
- Profile loading with various rule sets
- Check execution on sample documents
- Auto-fix applicability and application
- Report generation in each format
- Severity threshold behavior
- Batch preflight correctness
- Custom rule creation and application

### 8.10 Grau de Confiança
- Medium-High (significant PreflightProfile.dll size indicates robust implementation)

### 8.11 Prioridade de Implementação
- Medium-High - Critical for quality assurance

### 8.10 Implementation Notes
- Original implementation, do not copy Esko code
- Support standard rule categories: color, geometry, marks, text
- Provide extensible rule framework
- Enable user-defined rule creation
- Ensure auto-fix undoability
- Support multiple output formats

## 9. Automation Specification

### 9.1 Behavior Observed/Compilation
- Action list execution (sequential, branching, looping)
- Command parsing and execution
- Variable data substitution
- Sequential flow control
- Error handling strategies
- Batch processing workflows

### 9.2 Data Structures Necessary
- **Action**: id, type, name, parameters list, result, error, undo_info
- **Parameter**: type (string/number/boolean/color/enum/object/action), value, source (literal/variable/calculated), variable_name
- **ActionList**: actions list, variables dictionary, execution state (pending/running/paused/completed/failed/stopped)
- **VariableData**: name, value, type (string/number/color/boolean/object), source (xml/csv/database/api/user)

### 9.3 Entradas
- Action list (XML/JSON/binary)
- Variable data source
- Execution control (pause, stop, resume)
- User prompts for interactive parameters

### 9.4 Saídas
- Execution result success/failure
- Variable substitutions applied
- Step-by-step execution log
- Error descriptions with positions
- Final document state

### 9.5 Regras de Negócio
- Linear sequence: action 1, then 2, then 3
- Branching: if condition then A else B
- Looping: repeat N times or while condition
- Error handling: stop on first, continue, or custom handler
- Variable substitution before command execution
- User prompts for undefined variables

### 9.5 Algoritmos Identificados
- Action list XML parsing (likely with element/attribute structure)
- Command registry lookup by type
- Parameter substitution from variable data
- Sequential flow with branching and looping
- Error state management and recovery
- Progress reporting during execution

### 9.7 Dependências
- Document Engine: for document commands
- Color Engine: for color commands
- Geometry Engine: for transform commands
- Interface: for user prompts and display

### 9.7 Casos Especiais
- Nested action lists
- Infinite loop detection
- Error recovery and resume
- Variable scope (local vs. global)
- Conditional action execution
- Batch processing of multiple documents
- External API integration points

### 9.8 Testes Necessários
- Action list parse and execute
- Variable substitution correctness
- Branch/loop control flow
- Error handling strategies
- Progress reporting
- Batch processing correctness
- Variable scope management

### 9.8 Grau de Confiança
- Medium (observed through VariableEngine.dll and automation module analysis)

### 9.9 Prioridade de Implementação
- Medium - Important for workflow automation

### 9.8 Implementation Notes
- Original implementation, do not copy Esko automation code
- Support XML and JSON action list formats
- Provide variable data source flexibility
- Include comprehensive error handling
- Document action list format specification
- Provide progress reporting UI

## 10. Interface Specification

### 10.1 Behavior Observed/Compromised
- Menus and toolbars with configurable items
- Dock panels (float, dock, tab, hide)
- Shortcut key management
- Property panel with categorized fields
- Command dispatch from UI to modules

### 10.3 Data Structures Necessary
- **MenuItem**: id, name, shortcut, enabled, checked, visible, type (normal/separator/submenu), handler, image
- **ToolbarButton**: id, name, image, enabled, checked, visible, style (push/toggle/drop-down), handler, shortcut
- **DockPanel**: id, title, widget, area (left/right/top/bottom/center), allowed_areas, floating, features, state
- **Shortcut**: id, key_combination, action, context, mode, enabled
- **PropertyRow**: category, fields list, live_preview, apply_button, cancel_button, search_capability

### 10.4 Entradas
- UI events (click, key press, drag)
- Parameter values from property panel
- Shortcut key presses
- Dock panel drag events

### 10.4 Saídas
- Command dispatch results
- Parameter updates
- Dock panel geometry updates
- Shortcut assignment confirmations
- Menu state updates (enabled/checked)

### 10.5 Regras de Negócio
- Shortcuts must not conflict with system/OS shortcuts
- Menu items enabled based on document state
- Dock panels remember last position/state
- Property panel updates trigger module operations
- Shortcut management: add/remove/modify/reset
- Toolbar customization: drag-and-drop reordering

### 10.6 Algoritmos Identificados
- Signal-slot (Qt) event handling pattern
- Command registry lookup by ID
- Shortcut key conflict detection
- Dock panel layout algorithm (left/right/top/bottom/center)
- Property row categorization and grouping
- Signal propagation through component hierarchy

### 10.8 Dependências
- Command Manager: for command registration and dispatch
- All Engines: for module-specific UI operations
- Localization.dll: for multilingual string management
- SmartNames.dll: for object naming references

### 10.8 Casos Especiais
- Right-to-left language support
- High DPI display scaling
- Multiple monitor setups
- Custom toolbar configurations
- Keyboard accessibility compliance
- Screen reader compatibility

### 10.9 Testes Necessários
- Menu item enable/disable state
- Toolbar customization and reset
- Shortcut add/remove/modify
- Dock panel float/dock/hide toggling
- Property panel field validation
- Shortcut conflict detection
- Multilingual string display

### 10.10 Grau de Confiança
- Medium (observed through Localization.dll and interface module analysis)

### 10.11 Prioridade de Implementação
- Medium - User experience critical

### 10.10 Implementation Notes
- Original UI implementation, do not copy Esko code
- Follow platform UI guidelines (Windows/macOS)
- Support accessibility (keyboard navigation, screen readers)
- Provide toolbar customization capabilities
- Maintain consistent shortcut scheme
- Ensure localization key management

## 11. Cross-Functional Implementation Notes

### 11.1 License and Legal
- All implementations must be original
- No copying of proprietary Esko code, resources, or components
- Use open licenses (MIT, BSD, Apache 2.0, or GPL as appropriate)
- Document all third-party library usages

### 11.2 Architectural Guidelines
- Modular design with well-defined interfaces
- Dependency injection for testability
- Event-driven architecture for module communication
- Plug-in architecture for extensibility
- Memory management with careful ownership tracking

### 11.3 Testing Guidelines
- Unit tests for individual functions
- Integration tests for module interactions
- End-to-end workflow tests
- Performance testing for large documents
- Regression testing on each release
- Cross-platform testing (Windows, macOS considered)

### 11.4 Performance Considerations
- O(n) algorithm awareness for large document operations
- Memory-efficient data structures
- Lazy loading for large resources
- Threading considerations for long operations
- Caching where appropriate (profiles, frequently used data)

### 11.5 Error Handling Guidelines
- Comprehensive error types and codes
- User-friendly error messages
- Recovery where possible
- Logging for debugging
- Graceful degradation on failures

### 11.6 Documentation Requirements
- API documentation for all public functions
- User guides for end-user features
- Developer guides for module extension
- Change logs for each release
- Architecture decision records

## Appendix A: Algorithm Confidence Scale
- **High**: Standard algorithm, well-documented, likely implementation matches common practice
- **Medium**: Algorithm identified but exact implementation details need verification
- **Low**: Speculative, based on naming patterns or partial analysis

## Appendix B: Confidence Notes on Naming
- Original Esko function names not always preserved
- Names descriptive of function purpose
- Prefixes indicate module: Art_, PDF_, Color_, Trapping_, etc.
- When original names unavailable, descriptive names used and marked as inferred

## Appendix C: Required Attribution
- Analysis based on ArtPro+ v250 examination
- Tools used: Ghidra 12.1.4, REA 6.8.0
- Binary analysis from C:\Esko\bg_prog_artpro+_v250\ArtPro+.app\
- Independent implementation specification for ArtPlus project