# ArtPro+ v250 - Document Engine Analysis

## Module: ArtDocument

### Overview
The ArtDocument module appears to be the central document management class in ArtPro+. Based on the DLL inventory and Ghidra import, this module handles:

- Document creation and destruction
- Page management
- Layer management
- Object hierarchy

### Key DLLs Involved
- **ArtDocument** (base module)
- **ArtPage** - Page representation
- **ArtLayer** - Layer management
- **ArtDB.dll** - Database backing store
- **Document.dll** - Document operations

### Functions likely associated with ArtDocument
Based on naming patterns observed in other Esko products and the DLL exports:

1. **Document Creation/Destruction**
   - ArtDocument_Create()
   - ArtDocument_Destroy()
   - ArtDocument_Open()
   - ArtDocument_Save()
   - ArtDocument_SaveAs()

2. **Page Management**
   - ArtPage_Create()
   - ArtPage_Destroy()
   - ArtPage_AddObject()
   - ArtPage_RemoveObject()
   - ArtPage_GetObjectAt()
   - ArtPage_GetSelectedObjects()

3. **Layer Management**
   - ArtLayer_Create()
   - ArtLayer_Destroy()
   - ArtLayer_MoveToTop()
   - ArtLayer_MoveToBottom()
   - ArtLayer_Select()
   - ArtLayer_Clone()

4. **Object Hierarchy**
   - ArtHierarchy_Create()
   - ArtHierarchy_Destroy()
   - ArtHierarchy_GetParent()
   - ArtHierarchy_GetChildren()
   - ArtHierarchy_FindObject()

### Data Structures (likely)

#### ArtDocument Structure
- Pointer to page list
- Pointer to layer list
- Selection state
- Undo/Redo history pointer
- Modification flag
- Reference count

#### ArtPage Structure
- List of GraphicObjects
- Page dimensions
- Current zoom/pan state
- Layer list
- Selection mask

#### ArtLayer Structure
- List of objects in layer
- Layer visibility
- Layer lock status
- Layer order (z-index)

### Algorithm Patterns (observed)

Based on common Esko ArtPro patterns:

1. **Object Cloning**
   - Deep copy of GraphicObject hierarchies
   - Reference counting for shared objects
   - Preservation of transformation matrices

2. **Selection Logic**
   - Ray casting for point-in-object testing
   - Z-order traversal for hit testing
   - Multi-select with modifier key handling

3. **Undo/Redo System**
   - Command pattern for operations
   - State snapshotting
   - Batch undo grouping

## Module: ArtPage

### Overview
ArtPage represents a single page within an ArtPro document. Based on the DLL naming convention and typical PDF/ layout engine patterns:

### Key Operations
1. **Page Initialization**
   - Set page size (A4, A3, custom sizes)
   - Set resolution (dpi)
   - Initialize color space

2. **Object Operations**
   - Add/remove GraphicObject
   - Move/transform objects
   - Rotate/scale objects
   - Boolean operations on objects

3. **Rendering**
   - Rasterization pipeline
   - Vector rendering
   - PDF export preparation

### Likely Data Structures
- **Page Rectangle**: Position and size in document units
- **Object List**: Linked list or array of GraphicObjects
- **Transformation Matrix**: 3x2 or 4x4 matrix for affine transforms
- **Clip Region**: Clipping boundaries for rendering

## Module: ArtLayer

### Overview
ArtLayer manages the stacking order of objects within a page.

### Key Operations
1. **Layer Creation**
   - ArtLayer_Create(name, visible, locked)
   
2. **Layer Operations**
   - MoveUp/MoveDown - change z-order
   - Show/Hide - toggle visibility
   - Lock/Unlock - prevent editing
   - Delete - remove layer and objects

3. **Selection within Layers**
   - SelectAllInLayer()
   - DeselectInLayer()
   - InvertSelectionInLayer()

### Data Structures
- **Layer Node**: Doubly-linked list node
  - Pointer to parent page
  - Pointer to first/last object
  - Layer properties (name, color, flags)
  - Prev/next pointers

## Module: GraphicObject

### Overview
The base class for all visual objects in ArtPro+. This is the foundation for all drawable elements.

### Object Types (inferred from DLL names)
- **GraphicObject** - Base class
- **ArtLine**, **ArtCurve**, **ArtBezier** - Line/curve objects
- **ArtRectangle**, **ArtEllipse**, **ArtPolygon** - Shape objects
- **ArtText** - Text objects
- **ArtImage** - Image objects
- **ArtGroup** - Group of objects
- **ArtArtboard** - Artboard container

### Common GraphicObject Operations
1. **Transformations**
   - Translate(x, y)
   - Scale(sx, sy)
   - Rotate(angle)
   - Skew(xAngle, yAngle)
   - Transform(matrix)

2. **Boolean Operations**
   - Union(object)
   - Intersect(object)
   - Subtract(object)
   - XOR(object)

3. **Properties**
   - GetBounds() -> rectangle
   - GetCenter() -> point
   - GetArea() -> float
   - GetPerimeter() -> float
   - IsVisible() -> bool
   - IsSelected() -> bool
   - SetVisible(bool)
   - SetSelected(bool)

4. **Serialization**
   - Serialize(writer) -> XML/binary
   - Deserialize(reader) -> object
   - GetSchema() -> type info

### Likely Data Structures
- **Object Header**: Type ID, reference count, flags
- **Transformation Matrix**: 3x2 floating-point matrix
- **Bounds Rectangle**: Floating-point rectangle
- **Style Properties**: Color, fill, stroke, transparency
- **Parent/Child Pointers**: For grouping hierarchy

## Integration with PDF Engine

The Document Engine interfaces with the PDF Engine through:

1. **Export Pipeline**
   - Document -> Internal representation -> PDF objects
   - GraphicObject -> PDF content stream elements
   - ArtPage -> PDF page object

2. **Preservation of Vector Data**
   - Maintaining object-based structure (not just rasterized)
   - Keeping transformation matrices editable
   - Preserving text as text (not outlines)

3. **PDF Object Mapping**
   - ArtPage -> /Page PDF object
   - GraphicObject -> PDF content stream operators
   - Color -> PDF color space/pattern
   - Transformation -> PDF transformation matrix

## Undo/Redo System

### Architecture
Based on typical Esko implementations:

1. **Command Pattern**
   - Each undoable operation creates a Command object
   - Command has Execute() and Unexecute() methods
   - Commands are stacked in an undo stack

2. **Command Types**
   - CreateObjectCommand
   - DeleteObjectCommand
   - MoveObjectCommand
   - TransformObjectCommand
   - ColorChangeCommand
   - LayerChangeCommand

3. **Macro Commands**
   - Group multiple atomic commands
   - Single undo/redo for complex operations
   - Support for batch operations

4. **State Management**
   - Current stack position tracker
   - Maximum undo steps configuration
   - Memory management for large documents

## Serialization Format

### Internal Format (likely XML-based)
- Document metadata (name, version, creation date)
- Page structure (size, objects, layers)
- Object definitions (type, geometry, style)
- Layer information (visibility, order, lock status)
- Selection state
- Undo history snapshot

### Export to PDF
- Vector objects preserved as PDF drawing commands
- Text preserved as PDF text operators
- Images exported at appropriate resolution
- Color management conversion to CMYK/RGB/Destination

## Cross-Module Dependencies

### Document Engine <- -> Geometry Engine
- Object bounds -> Geometry queries
- Transformations -> Matrix operations
- Boolean operations -> Intersection/union algorithms

### Document Engine <- -> Color Engine
- Object colors -> Color space conversions
- ICC profiles -> Color management
- Spot colors -> Device color mapping

### Document Engine <- -> Trapping Engine
- Object edges -> Trap generation
- Color interactions -> Overprint/knockout handling
- Layer boundaries -> Trap planning

### Document Engine <- -> Preflight Engine
- Document rules -> Validation checks
- Error/warning reporting -> User interface
- Automatic corrections -> Document modification

## Areas for Further Investigation

1. **Class Hierarchy**: Use Ghidra's class analysis to identify C++ classes, vtables, and RTTI data
2. **Function Signatures**: Extract function prototypes from exports and imports
3. **Pseudocode**: Decompile key functions to understand algorithm implementations
4. **Memory Layout**: Analyze heap/stack usage for object creation
5. **Inter-module Calls**: Trace calls between Document, Geometry, Color, and Trapping modules

## Next Steps
1. Launch Ghidra GUI and load the ArtPro+ project
2. Identify C++ classes and vtables
3. Decompile key ArtDocument, ArtPage, ArtLayer functions
4. Map function calls between modules
5. Document algorithm implementations