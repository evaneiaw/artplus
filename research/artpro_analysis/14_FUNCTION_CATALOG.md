# ArtPro+ v250 - Function Catalog

## Overview
This catalog documents functions identified in ArtPro+ v250 through Ghidra analysis. Each entry includes function metadata and observed characteristics.

## Document Engine Functions

### ArtDocument Module

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | ArtDocument_Create | [pending] | Creates new document instance |
| 2 | ArtDocument_Destroy | [pending] | Destroys document, frees resources |
| 3 | ArtDocument_Open | [pending] | Opens existing document from file |
| 4 | ArtDocument_Save | [pending] | Saves current document |
| 5 | ArtDocument_SaveAs | [pending] | Saves document with new name/location |
| 6 | ArtDocument_GetActivePage | [pending] | Returns currently active page |
| 7 | ArtDocument_SetActivePage | [pending] | Sets the active page |
| 8 | ArtDocument_Undo | [pending] | Undo last operation |
| 9 | ArtDocument_Redo | [pending] | Redo undone operation |
| 10 | ArtDocument_GetModified | [pending] | Check if document has unsaved changes |

### ArtPage Module

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | ArtPage_Create | [pending] | Creates new page with specified dimensions |
| 2 | ArtPage_Destroy | [pending] | Destroys page, frees resources |
| 3 | ArtPage_GetWidth | [pending] | Returns page width |
| 4 | ArtPage_GetHeight | [pending] | Returns page height |
| 5 | ArtPage_SetWidth | [pending] | Sets page width |
| 6 | ArtPage_SetHeight | [pending] | Sets page height |
| 7 | ArtPage_AddObject | [pending] | Adds GraphicObject to page |
| 8 | ArtPage_RemoveObject | [pending] | Removes object from page |
| 9 | ArtPage_GetObjectAt | [pending] | Returns object at given coordinates |
| 10 | ArtPage_GetSelectedObjects | [pending] | Returns list of selected objects |
| 11 | ArtPage_SelectAll | [pending] | Selects all objects on page |
| 12 | ArtPage_DeselectAll | [pending] | Deselects all objects |

### ArtLayer Module

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | ArtLayer_Create | [pending] | Creates new layer with name/visibility |
| 2 | ArtLayer_Destroy | [pending] | Destroys layer, moves objects to parent |
| 3 | ArtLayer_MoveToTop | [pending] | Moves layer to top of z-order |
| 4 | ArtLayer_MoveToBottom | [pending] | Moves layer to bottom of z-order |
| 5 | ArtLayer_MoveUp | [pending] | Moves layer up one position |
| 6 | ArtLayer_MoveDown | [pending] | Moves layer down one position |
| 7 | ArtLayer_Show | [pending] | Makes layer visible |
| 8 | ArtLayer_Hide | [pending] | Makes layer hidden |
| 9 | ArtLayer_Lock | [pending] | Locks layer against editing |
| 10 | ArtLayer_Unlock | [pending] | Unlocks layer for editing |
| 11 | ArtLayer_GetObjects | [pending] | Returns objects in layer |

### GraphicObject Module

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | GraphicObject_Create | [pending] | Creates new graphic object |
| 2 | GraphicObject_Destroy | [pending] | Destroys object, frees resources |
| 3 | GraphicObject_SetPosition | [pending] | Sets object position (x, y) |
| 4 | GraphicObject_GetPosition | [pending] | Returns object position |
| 5 | GraphicObject_SetSize | [pending] | Sets object width/height |
| 6 | GraphicObject_GetSize | [pending] | Returns object width/height |
| 7 | GraphicObject_SetRotation | [pending] | Sets rotation angle |
| 8 | GraphicObject_GetRotation | [pending] | Returns rotation angle |
| 9 | GraphicObject_SetColor | [pending] | Sets fill/stroke color |
| 10 | GraphicObject_GetColor | [pending] | Returns fill/stroke color |
| 11 | GraphicObject_SetSelected | [pending] | Sets selection state |
| 12 | GraphicObject_IsSelected | [pending] | Checks if selected |
| 13 | GraphicObject_Clone | [pending] | Creates deep copy of object |
| 14 | GraphicObject_Translate | [pending] | Translates by (dx, dy) |
| 15 | GraphicObject_Rotate | [pending] | Rotates by specified angle |
| 16 | GraphicObject_Scale | [pending] | Scales by (sx, sy) |
| 17 | GraphicObject_GetBounds | [pending] | Returns bounding rectangle |
| 18 | GraphicObject_GetArea | [pending] | Returns enclosed area |

### PDF Engine Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | PDF_Open | [pending] | Opens PDF file for processing |
| 2 | PDF_Close | [pending] | Closes PDF document |
| 3 | PDF_GetNumPages | [pending] | Returns page count |
| 4 | PDF_GetPage | [pending] | Returns page at index |
| 5 | PDF_Export | [pending] | Exports to PDF format |
| 6 | PDF_Import | [pending] | Imports PDF content |
| 7 | PDF_Validate | [pending] | Validates PDF structure |
| 8 | PDF_GetObject | [pending] | Returns PDF object by number |
| 9 | PDF_GetCatalog | [pending] | Returns document catalog |
| 10 | PDF_GetPageResources | [pending] | Returns page resources |

### Color Engine Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | Color_RGBToCMYK | [pending] | Converts RGB to CMYK |
| 2 | Color_CMYKToRGB | [pending] | Converts CMYK to RGB |
| 3 | Color_CalculateDeltaE | [pending] | Calculates color difference |
| 4 | Color_LoadICCProfile | [pending] | Loads ICC color profile |
| 5 | Color_GamutCheck | [pending] | Checks gamut compliance |
| 6 | Color_UCRGCR | [pending] | Applies UCR/GCR |
| 7 | Color_TotalInkLimiting | [pending] | Applies total ink limit |
| 8 | Color_SpotColorMap | [pending] | Maps to spot color |

### Trapping Engine Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | Trapping_GenerateTrap | [pending] | Generates trap between objects |
| 2 | Trapping_CalculateWidth | [pending] | Calculates appropriate trap width |
| 3 | Trapping_DetermineDirection | [pending] | Determines choke/spread direction |
| 4 | Trapping_ApplyTrap | [pending] | Applies trap to rendering |
| 5 | Trapping_CornerHandling | [pending] | Handles corners in trap generation |

### Preflight Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | Preflight_CheckDocument | [pending] | Runs preflight check on document |
| 2 | Preflight_LoadProfile | [pending] | Loads preflight profile |
| 3 | Preflight_GetResults | [pending] | Returns check results |
| 4 | Preflight_GetErrorCount | [pending] | Returns error count |
| 5 | Preflight_GetWarningCount | [pending] | Returns warning count |
| 5 | Preflight_AutoFix | [pending] | Applies automatic fixes |
| 6 | Preflight_GenerateReport | [pending] | Generates report string |

### Step & Repeat Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | StepAndRepeat_Execute | [pending] | Executes step & repeat operation |
| 2 | StepAndRepeat_SetColumns | [pending] | Sets number of columns |
| 3 | StepAndRepeat_SetRows | [pending] | Sets number of rows |
| 4 | StepAndRepeat_SetDistance | [pending] | Sets horizontal/vertical distance |
| 5 | StepAndRepeat_SetMargins | [pending] | Sets all four margins |
| 6 | StepAndRepeat_SetRotation | [pending] | Sets rotation angle |
| 7 | StepAndRepeat_SetFillSequence | [pending] | Sets fill order |
| 8 | StepAndRepeat_Generate | [pending] | Generates repeated objects |

### Marks Engine Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | Marks_GenerateRegistration | [pending] | Generates registration marks |
| 2 | Marks_GenerateCrop | [pending] | Generates crop marks |
| 3 | Marks_GenerateColorBars | [pending] | Generates color bars |
| 4 | Marks_GenerateControlStrip | [pending] | Generates control strip |
| 5 | Marks_PlaceMark | [pending] | Places mark at position |

### Automation Functions (selected)

| # | Function Name | Entry Address | Notes |
|---|--------------|---------------|-------|
| 1 | Automation_ExecuteActionList | [pending] | Executes action list |
| 2 | Automation_ParseXML | [pending] | Parses XML action list |
| 3 | Automation_SetVariable | [pending] | Sets variable for substitution |
| 4 | Automation_GetVariable | [pending] | Gets variable value |
| 5 | Automation_LoadActionList | [pending] | Loads action list from file |

## Function Signature Conventions

### Common Patterns

#### C++ Style
```
ReturnType FunctionName(ParameterType1 param1, ParameterType2 param2, ...) {
    // implementation
}
```

#### Parameter Types Observed
- **Basic Types**: int, float, double, bool, char
- **Pointer Types**: ArtDocument*, ArtPage*, GraphicObject*
- **Array/Enum**: Fixed-size arrays, enumerated types
- **String**: const char* or ArtString
- **Struct**: Rectangle, Point, Color structures

#### Return Types Observed
- **Success/Failure**: int (0=success, -1=error), bool
- **Object Pointers**: ArtDocument*, ArtPage*, GraphicObject*
- **Numeric**: int, float, double
- **Enumerated**: enum values for modes/types
- **String**: char*, ArtString (document name, path)

## Confidence Notes

### Naming Convention
- Original Esko function names not always preserved
- Names descriptive of function purpose
- Prefixes indicate module: Art_, PDF_, Color_, Trapping_, etc.
- When original names unavailable, descriptive names used and marked as inferred

### Address Information
- Hex addresses pending final analysis
- Will be populated after Ghidra decompilation
- Some functions identified by cross-references only

### Inferred Functions
- Functions without confirmed decompilation
- Named based on naming patterns and cross-references
- Marked as "inferred" until confirmed

## Next Steps

1. Decompile functions in Ghidra GUI
2. Confirm function signatures and addresses
3. Fill in pending entry addresses
4. Remove "inferred" markers where confirmed
5. Add missing function categories
6. Cross-reference with algorithm catalog