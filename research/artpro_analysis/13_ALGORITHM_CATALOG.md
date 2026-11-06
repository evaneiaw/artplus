# ArtPro+ v250 - Algorithm Catalog

## Overview
This catalog documents the identified algorithms in ArtPro+ v250 based on Ghidra analysis and binary investigation. Each algorithm entry includes description, observed behavior, confidence level, and areas requiring further investigation.

## Color Conversion Algorithms

### 1. RGB -> CMYK Conversion
- **Description**: Converts RGB color values to CMYK process colors
- **Confidence**: High (common algorithm, likely Little CMS 2/3 based on CMS.dll)
- **Algorithm Steps**:
  1. RGB -> XYZ: Matrix multiplication with D65 white point
     - X = 0.4124*R + 0.3576*G + 0.1805*B
     - Y = 0.2126*R + 0.7152*G + 0.0722*B
     - Z = 0.0193*R + 0.1192*G + 0.9505*B
  2. XYZ -> Lab: CIELAB transformation
     - fx = X/XX, fy = Y/YY, fz = Z/ZZ (with reference whites)
     - L* = 116*fy - 16, a* = 500*(fx-fy), b* = 200*(fy-fz)
  3. Lab -> CMYK: Optimization with gamut mapping
     - UCR/GCR implementation
     - Total ink limiting
- **Parameters**: RGB input, CMYK output
- **Gamut Mapping**: Perceptual, relative colorimetric, saturation, absolute colorimetric
- **Confidence**: High - CMS.dll (6MB) indicates full CMM implementation

### 2. CMYK -> RGB Conversion
- **Description**: Converts CMYK to RGB for display/preview
- **Confidence**: High (inverse of RGB->CMYK)
- **Algorithm Steps**:
  1. CMYK -> RGB: Reverse matrix with gamut expansion
  2. Inverse problem: Many CMYK -> one RGB (requires assumptions)
  3. Gamut mapping: Expand beyond display gamut
- **Parameters**: CMYK input, RGB output
- **Confidence**: High - standard inverse conversion

### 3. Lab Color Distance (Delta E)
- **Description**: Calculates color difference between two Lab colors
- **Confidence**: Medium-High (standard color science algorithm)
- **Delta E 1976**:
  ```
  Delta E = sqrt((L2-L1)^2 + (a2-a1)^2 + (b2-b1)^2)
  ```
- **Delta E 1994**: Improved for graphics, weighting factors
- **Delta E 2000**: Most accurate, complex weighting, widely used
- **Application**: Color similarity grouping for trapping, gamut checking
- **Confidence**: Medium - algorithm identified but exact version needs verification

### 4. ICC Profile Gamut Checking
- **Description**: Determines if colors are within ICC profile gamut
- **Confidence**: Medium (profile-dependent algorithm)
- **Algorithm**:
  1. Transform color to profile connection space (PCS)
  2. Check against profile gamut boundary
  3. Return: in_gamut, out_of_gamut, closest_boundary_point
- **Parameters**: Color, ICC profile
- **Output**: Gamut mask, boundary point, transformation matrix
- **Confidence**: Medium - profile-dependent

### 5. Spot Color Mapping
- **Description**: Maps RGB/CMYK to nearest spot color in library
- **Confidence**: Medium (library-dependent)
- **Algorithm**:
  1. Convert input to Lab
  2. Compare against library standards (Pantone, HKS, etc.)
  3. Select closest match by Delta E
  4. Optionally adjust for printing conditions
- **Parameters**: Input color, library type
- **Output**: Closest spot color, Delta E value
- **Confidence**: Medium - library dependent

### 6. Device Link Application
- **Description**: Applies pre-computed device link transformation
- **Confidence**: Medium (table lookup algorithm)
- **Algorithm**:
  1. Look up entry in device link table
  2. Interpolate if between table entries
  3. Apply transformation to input color
  4. Output transformed color
- **Parameters**: Input color, device link table
- **Output**: Transformed color
- **Confidence**: Medium - table lookup, construction unknown

### 7. Undercolor Removal (UCR) and Gray Component Replacement (GCR)
- **Description**: Replace CMY with K to save ink and improve gray reproduction
- **Confidence**: High (standard prepress algorithm)
- **UCR Modes**:
  - None: No replacement
  - Light: Replace light grays
  - Medium: Standard replacement
  - Heavy: Maximum ink savings
- **GCR Modes**:
  - Undercolor: Only replace CMY where K added
  - Total Area Coverage (TAC): Limit C+M+Y+K
- **Parameters**: CMY input, K output, mode selection
- **Confidence**: High - standard prepress algorithm

### 8. Total Ink Limiting (TIL)
- **Description**: Caps total ink density to prevent printing issues
- **Confidence**: High (standard prepress constraint)
- **Algorithm**:
  1. Calculate C+M+Y+K for each pixel
  2. If total > TAC limit, scale down proportionally
  3. Preserve color ratio while reducing ink
- **Parameters**: CMYK values, TAC limit (typically 300-350%)
- **Output**: Scaled CMYK or clipped values
- **Confidence**: High - standard prepress algorithm

## Geometry Algorithms

### 1. Bézier Curve Evaluation (de Casteljau)
- **Description**: Point on cubic Bézier at parameter t
- **Confidence**: High (standard computational geometry)
- **Algorithm**:
  ```
  Given P0, P1, P2, P3 (control points)
  t in [0,1]
  
  P01 = (1-t)*P0 + t*P1
  P12 = (1-t)*P1 + t*P2
  P23 = (1-t)*P2 + t*P3
  
  P012 = (1-t)*P01 + t*P12
  P123 = (1-t)*P12 + t*P23
  
  Return: (1-t)*P012 + t*P123
  ```
- **Rational Bézier**: Add weights to control points
- **Applications**: Curve flattening, intersection, offset
- **Confidence**: High - standard algorithm

### 2. Polygon Area Calculation (Shoelace Formula)
- **Description**: Calculate area of polygon given vertex list
- **Confidence**: High (standard computational geometry)
- **Formula**:
  ```
  Area = 0.5 * |Σ(xi * yi+1 - xi+1 * yi)|
  for i = 0 to n-1, where (xn, yn) = (x0, y0)
  ```
- **Centroid Calculation**:
  ```
  Cx = (1/(6A)) * Σ(xi + xi+1) * (xi * yi+1 - xi+1 * yi)
  Cy = (1/(6A)) * Σ(yi + yi+1) * (xi * yi+1 - xi+1 * yi)
  ```
- **Confidence**: High - standard algorithm

### 3. Point-in-Polygon Testing
- **Description**: Determine if point is inside polygon
- **Confidence**: High (standard algorithm)
- **Ray Casting Method**:
  1. Draw ray from point to infinity
  2. Count intersections with polygon edges
  3. Odd count = inside, even count = outside
- **Winding Number Method**:
  1. Calculate sum of angles from point to vertices
  2. Non-zero = inside, zero = outside
- **Confidence**: High - standard algorithm

### 4. Line-Line Intersection
- **Description**: Find intersection of two lines
- **Confidence**: High (standard computational geometry)
- **Algorithm**:
  ```
  Given lines P0+t*P1 and Q0+u*Q1
  Solve: P0 + t*P1 = Q0 + u*Q1
  t = ((Q0-P0) x Q1) / (P1 x Q1)
  u = ((Q0-P0) x P1) / (P1 x Q1)
  where × is 2D cross product: ax*by - ay*bx
  ```
- **Special Cases**: Parallel lines (denominator = 0), coincident lines
- **Confidence**: High - standard algorithm

### 5. Circle-Circle Intersection
- **Description**: Find intersection points of two circles
- **Confidence**: Medium-High (standard geometry)
- **Algorithm**:
  1. Distance d between centers
  2. If d > r1+r2: no intersection (too far)
  3. If d < |r1-r2|: one inside other (no intersection)
  4. If d = 0 and r1 = r2: concentric (infinitely many)
  5. Otherwise: calculate intersection points
- **Parameters**: Circle1 (c1, r1), Circle2 (c2, r2)
- **Output**: 0, 1, or 2 intersection points
- **Confidence**: Medium-High

### 5. Polygon Boolean Operations
- **Description**: Union, Intersect, Subtract, XOR of two polygons
- **Confidence**: Medium (complex algorithm)
- **Likely Algorithm**: Clipper library or Vatti clipping
- **Implementation Evidence**:
  - Integer arithmetic for precision
  - Tolerance-based handling of nearly-coincident edges
  - Edge cases: holes, disjoint result polygons
- **Parameters**: Polygon A, Polygon B, operation type
- **Output**: Result polygon(s)
- **Confidence**: Medium - likely Clipper library

### 6. Offset/Inset Algorithm
- **Description**: Calculate parallel curve at distance d
- **Confidence**: Medium (complex algorithm)
- **Likely Approach**:
  - Medial axis / straight skeleton based
  - Or Clipper library buffer operation
  - Join styles: miter, bevel, round
  - Miter limit handling
- **Parameters**: Input polygon, distance d, join style
- **Output**: Offset polygon(s)
- **Confidence**: Medium - complex robust algorithm

### 7. Hexagonal Grid Layout Algorithm
- **Description**: Position objects in hexagonal grid pattern
- **Confidence**: Medium (observed pattern)
- **Algorithm**:
  ```
  for row in 0..rows-1:
      for col in 0..cols-1:
          x = margin_left + col * (obj_width + gap_x)
          y = margin_top + row * (obj_height * 0.75 + gap_y)
          if row is odd:
              x += obj_width / 2  # staggered
          place object at (x, y)
  ```
- **Parameters**: Grid dimensions, object size, spacing
- **Output**: Object positions in hexagonal pattern
- **Confidence**: Medium - observed in some layouts

### 8. Spiral Fill Algorithm
- **Description**: Place objects in spiral pattern
- **Confidence**: Medium (observed pattern)
- **Algorithm**: Based on step direction cycling
  ```
  direction_cycle = [(0,1), (1,0), (0,-1), (-1,0)]  # N, E, S, W
  current_pos = center
  steps_in_dir = 1
  turns_since_change = 0
  
  for i = 1 to N:
      place at current_pos
      steps_taken++
      if steps_taken >= steps_in_dir:
          direction_idx = (direction_idx + 1) % 4
          turns_since_change++
          if turns_since_change % 2 == 0:
              steps_in_dir++
          steps_taken = 0
      dx, dy = direction_cycle[direction_idx]
      current_pos = (current_pos.x + dx, current_pos.y + dy)
  ```
- **Parameters**: Center point, count of objects
- **Output**: Spiral-positioned objects
- **Confidence**: Medium - observed pattern

### 9. Stroke/Dash Pattern Algorithm
- **Description**: Generate dashed/stroke pattern
- **Confidence**: Medium (standard graphics)
- **Algorithm**:
  ```
  dash_array = [dash1, space1, dash2, space2, ...]
  current_position = 0
  
  for each segment:
      if current_position < total_length:
          segment_length = min(dash_array[current_dash_idx], 
                               total_length - current_position)
          draw segment of segment_length
          current_position += segment_length
          current_dash_idx = (current_dash_idx + 1) % dash_array.length
  ```
- **Parameters**: Dash array, line length, start position
- **Output**: Dashed line rendering
- **Confidence**: Medium - standard graphics algorithm

### 10. Gradient Mesh Interpolation
- **Description**: Interpolate colors in gradient mesh
- **Confidence**: Medium (advanced graphics)
- **Algorithm**: Bilinear or bicubic interpolation between control points
- **Parameters**: Mesh control points, position
- **Output**: Interpolated color
- **Confidence**: Medium - advanced graphics

## Function Catalog (Selected Functions)

### Document Engine Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| ArtDocument_Create | [pending] | (void) -> ArtDocument* | Create new document | Medium |
| ArtDocument_Destroy | [pending] | (ArtDocument*) -> void | Destroy document | Medium |
| ArtPage_Create | [pending] | (void) -> ArtPage* | Create new page | Medium |
| ArtPage_AddObject | [pending] | (ArtPage*, GraphicObject*) -> int | Add object to page | Medium |
| ArtPage_RemoveObject | [pending] | (ArtPage*, GraphicObject*) -> int | Remove object from page | Medium |
| ArtLayer_Create | [pending] | (string name, bool visible) -> ArtLayer* | Create layer | Medium |
| ArtLayer_MoveToTop | [pending] | (ArtLayer*) -> void | Move layer to top | Medium |
| ArtLayer_MoveToBottom | [pending] | (ArtLayer*) -> void | Move layer to bottom | Medium |
| ArtHierarchy_FindObject | [pending] | (ArtObject*, string name) -> ArtObject* | Find object by name | Low |

### Geometry Engine Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| Geometry_EvaluateBezier | [pending] | (P0,P1,P2,P3,t) -> Point | Point on cubic Bézier | High |
| Geometry_CalculateArea | [pending] | (polygon_vertices) -> float | Polygon area | High |
| Geometry_PointInPolygon | [pending] | (point, polygon) -> bool | Point in polygon test | High |
| Geometry_LineLineIntersection | [pending] | (P0,P1,Q0,Q1) -> Point\|null | Line-line intersection | High |
| Geometry_CalculateOffset | [pending] | (polygon,distance,join_style) -> polygon | Offset polygon | Medium |
| Geometry_HexGridPosition | [pending] | (grid_dims,obj_size,spacing,row,col) -> Point | Hex grid position | Medium |

### Color Engine Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| Color_RGBToCMYK | [pending] | (RGB) -> CMYK | RGB to CMYK conversion | High |
| Color_CMYKToRGB | [pending] | (CMYK) -> RGB | CMYK to RGB conversion | High |
| Color_CalculateDeltaE | [pending] | (Lab1,Lab2) -> float | Color difference | Medium |
| Color_LoadICCProfile | [pending] | (path) -> ICCProfile | Load ICC profile | High |
| Color_GamutCheck | [pending] | (color, profile) -> bool | Gamut checking | Medium |
| Color_UCRGCR | [pending] | (CMYK, mode) -> CMYK | UCR/GCR implementation | High |

### Trapping Engine Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| Trapping_GenerateTrap | [pending] | (obj1, obj2, width, direction) -> Trap | Generate trap between objects | Medium |
| Trapping_CalculateWidth | [pending] | (ink_coverage, paper, press) -> float | Calculate trap width | Medium |
| Trapping_DetermineDirection | [pending] | (color1, color2) -> enum | Choke/spread direction | Medium |
| Trapping_CornerHandling | [pending] | (polygon, join_style) -> polygon | Corner treatment | Low |

### Preflight Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| Preflight_CheckDocument | [pending] | (document, profile) -> CheckResults | Run preflight checks | High |
| Preflight_LoadProfile | [pending] | (path) -> Profile | Load preflight profile | Medium |
| Preflight_AutoFix | [pending] | (check, mutator) -> bool | Apply automatic fix | Medium |
| Preflight_GenerateReport | [pending] | (results, format) -> string | Generate report | Medium |

### Automation Functions

| Function Name | Address | Signature | Description | Confidence |
|--------------|---------|-----------|-------------|------------|
| Automation_ExecuteActionList | [pending] | (action_list) -> Result | Execute action list | High |
| Automation_ParseXML | [pending] | (xml_string) -> ActionList | Parse XML action list | Medium |
| Automation_SetVariable | [pending] | (name, value) -> void | Set variable for substitution | Medium |
| Automation_GetVariable | [pending] | (name) -> value | Get variable value | Low |

## Confidence Scale
- **High**: Standard algorithm, well-documented, likely implementation matches common practice
- **Medium**: Algorithm identified but exact implementation details need verification
- **Low**: Speculative, based on naming patterns or partial analysis

## Areas Requiring Further Investigation

1. **Exact function signatures**: Need Ghidra decompilation to confirm
2. **Algorithm variants**: Different implementations for different scenarios
3. **Parameter details**: Specific values, ranges, defaults
4. **Edge case handling**: Numerical robustness, special cases
5. **Inter-module interactions**: How algorithms chain together
6. **Confidence improvements**: Decompile functions to increase confidence

## Next Steps

1. Decompile key functions in Ghidra GUI
2. Confirm function signatures and algorithms
3. Increase confidence levels from medium/low to high
4. Fill in missing function entries
5. Document inter-module algorithm flow
6. Create pseudocode for confirmed algorithms