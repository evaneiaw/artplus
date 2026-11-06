# ArtPro+ v250 - Geometry Engine Analysis

## Overview
The Geometry Engine in ArtPro+ v250 handles all geometric operations including lines, curves, polygons, boolean operations, transformations, and calculations of distance and area. This is a core module for any CAD/graphics application.

## Key DLLs
- **GeoMapper.dll** (731,176 bytes) - Geometric mapping
- **CurveApprox.dll** (98,856 bytes) - Curve approximation
- **Cornering.dll** (53,288 bytes) - Corner detection/handling
- **Offset.dll** (103,464 bytes) - Offset calculation
- **Pack3D.dll** (3,732,520 bytes) - 3D packing operations
- **LayoutEngine.dll** (528,936 bytes) - Layout operations
- **Mutator*.dll** families - Mutation operations
- **EPS (not in direct list but related)** - Encapsulated PostScript operations

## Geometric Primitives

### 1. Lines
#### Line Representation
- **2 Points**: Start point (x1, y1) and end point (x2, y2)
- **Point + Direction**: Start point and angle/direction vector
- **Slope-Intercept**: Slope and y-intercept (less common in CAD)

#### Line Operations
- **Construction**: Line(p1, p2), Line(angle, point)
- **Length**: Euclidean distance between endpoints
- **Midpoint**: ((x1+x2)/2, (y1+y2)/2)
- **Angle**: atan2(y2-y1, x2-x1)
- **Parallel Check**: Compare direction vectors
- **Perpendicular Check**: Dot product = 0
- **Intersection**: Line-line intersection algorithm
- **Offset**: Parallel line at distance d
- **Reflection**: Mirror across point or line

#### Likely Algorithm
- Standard Cartesian geometry
- Integer/float precision handling
- Special cases: vertical lines (infinite slope), coincident lines

### 2. Curves Bézier
#### Bézier Curve Representation
- **Quadratic Bézier**: P0, P1, P2 (3 control points)
- **Cubic Bézier**: P0, P1, P2, P3 (4 control points) - most common
- **Rational Bézier**: Weighted control points (for conic sections)

#### Bézier Operations
- **Evaluation**: Point at parameter t (de Casteljau algorithm)
- **Derivative**: Tangent at parameter t
- **Arc Length**: Length along curve (numerical integration)
- **Bounding Box**: Calculate bounding rectangle
- **Flattening**: Convert to line segments within tolerance
- **Intersection**: Curve-line or curve-curve intersection
- **Offset**: Parallel curve at distance d
- **Simplification**: Remove unnecessary control points

#### de Casteljau Algorithm (likely implemented)
```
Given cubic Bézier B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
Algorithm:
1. If t == 0: return P0
2. If t == 1: return P3
3. P01 = (1-t)*P0 + t*P1
   P12 = (1-t)*P1 + t*P2
   P23 = (1-t)*P2 + t*P3
4. P012 = (1-t)*P01 + t*P12
   P123 = (1-t)*P12 + t*P23
5. Return (1-t)*P012 + t*P123
```

#### Rational Bézier (for conics)
- Weighted control points w0, w1, w2
- Special case: circle arc representation

### 3. Polygons
#### Polygon Representation
- **Vertex List**: Ordered list of (x, y) points
- **Edge List**: List of edges (vi, vi+1)
- **Closed**: Last vertex connects to first
- **Convex/Concave**: Determined by cross product sign

#### Polygon Operations
- **Area Calculation**: Surveyor's formula (shoelace formula)
  ```
  Area = 0.5 * |Σ(xi * yi+1 - xi+1 * yi)|
  ```
- **Centroid**: Center of mass
  ```
  Cx = (1/(6A)) * Σ(xi + xi+1) * (xi * yi+1 - xi+1 * yi)
  Cy = (1/(6A)) * Σ(yi + yi+1) * (xi * yi+1 - xi+1 * yi)
  ```
- **Perimeter**: Sum of edge lengths
- **Point-in-Polygon**: Ray casting or winding number
- **Polygon Boolean Operations**: Union, Intersect, Subtract, XOR
- **Simplify**: Douglas-Peucker or similar
- **Convex Hull**: Gift wrapping or Monotone chain
- **Triangulation**: ear clip or sweepline

#### Likely Boolean Operation Algorithms
- **Weiler-Atherton**: Polygon clipping with holes
- **Sutherland-Hodgman**: Polygon clipping against convex window
- **Vatti**: General polygon clipping (likely for complex cases)
- **Greiner-Hormann**: Polygon intersection/clipping

### 4. Boolean Operations
#### Operation Types
- **Union (OR)**: All points in either polygon
- **Intersect (AND)**: Points in both polygons
- **Difference (SUBTRACT)**: Points in first but not second
- **XOR (EXCLUSIVE)**: Points in one but not both

#### Boolean Algorithm Classes
1. **Scanline-based**: Process scanlines through both polygons
2. **Constraint-based**: Build constraints from polygon edges
3. **Cell decomposition**: Decompose into simpler shapes
4. **Polygonal**: Keep as polygons, compute intersection

#### Implementation Evidence
- ArtPro+ likely uses Clipper Library (open-source polygon clipping)
- Or custom implementation optimized for prepress workflows
- Integer arithmetic for precision preservation
- Tolerance-based handling of nearly-coincident edges

### 5. Boolean Operations Details

#### Union (A ∪ B)
- All boundary points from both
- Holes from A remain holes unless overlapped by B
- Result is a (possibly) single polygon with holes

#### Intersection (A ∩ B)
- Only overlapping region
- Complex if polygons only partially overlap
- May result in multiple disjoint polygons

#### Difference (A - B)
- Points in A but not in B
- May create new holes from B's boundary
- Result may be multiple polygons

#### XOR (A ⊕ B)
- Points in A or B but not both
- Symmetric difference
- Often used for compound shape creation

### 6. Offset Operations
#### Offset (Parallel Curve)
- **Positive Offset**: Expand outward by distance d
- **Negative Offset**: Contract inward by distance d
- **Variable Offset**: Different distances at different points

#### Offset Algorithm (Likely)
- **Straight Skeleton**: Scale-invariant offset
- **Medial Axis**: Based on center of inscribed circles
- **Buffer Operation**: GIS-style buffer (common in CAD)
- **Clipper Library**: Likely used (Windsor/Even-odd rules)

#### Offset Rules
- **Square Ends**: Perpendicular at endpoints
- **Round Ends**: Quarter-circle at endpoints
- **Miter Limit**: Limit miter join ratio
- **Miter Join**: Sharp corner when offset lines meet
- **Bevel Join**: Flattened corner

#### Offset Calculation Steps
1. For each edge, compute offset line parallel at distance d
2. Intersect offset lines at vertices
3. Apply join style (miter, bevel, round)
4. Clip to miter limit if specified
5. Handle degenerate cases (zero-width regions)

### 7. Inset and Outset
#### Inset (Offset Inward)
- Similar to negative offset
- Used for creating inner boundaries
- Common in window/door openings

#### Outset (Offset Outward)
- Similar to positive offset
- Used for creating outer boundaries
- Common in matting/bleed areas

### 8. Bleed Generation
#### Bleed Operations
- **Uniform Bleed**: Extend all edges by same amount
- **Proportional Bleed**: Scale-based extension
- **Corner Handling**: Miter, bevel, or round at corners
- **Bleed Limit**: Maximum bleed distance

#### Bleed Algorithm
1. Identify edge objects
2. Extend each edge beyond document bounds
3. Apply corner styles
4. Generate bleed area geometry
5. Ensure no unwanted overlaps

### 9. Contours
#### Contour Representation
- **Outer Contour**: Boundary of shape
- **Inner Contours (Holes)**: Holes within shape
- **Compound Contour**: Outer + inner as single entity

#### Contour Operations
- **Simplify**: Reduce vertex count
- **Smooth**: Apply Bezier smoothing
- **Reverse**: Reverse orientation
- **Extract**: Isolate individual contours

#### Winding Number and Orientation
- **CW (Clockwise)**: Negative area, exterior boundary
- **CCW (Counter-Clockwise)**: Positive area, interior boundary (hole)
- **Consistency**: Ensure all contours have consistent orientation for boolean operations

### 10. Intersection Calculations
#### Line-Line Intersection
```
Given lines P0+t*P1 and Q0+u*Q1
Solve: P0 + t*P1 = Q0 + u*Q1
t = ((Q0-P0) x Q1) / (P1 x Q1)
u = ((Q0-P0) x P1) / (P1 x Q1)
where × is 2D cross product: (ax, ay) × (bx, by) = ax*by - ay*bx
```

#### Line-Circle Intersection
- Circle: (x - cx)^2 + (y - cy)^2 = r^2
- Line: P0 + t * (P1 - P0)
- Quadratic equation in t
- 0, 1, or 2 intersection points

#### Line-Rectangle Intersection
- Check if line intersects any of 4 edges
- Or use slab method (check x and y intervals)

#### Circle-Circle Intersection
- Distance between centers
- Based on sum/difference of radii

#### Polygon-Polygon Intersection
- Edge-edge intersection tests
- Point-in-polygon tests for vertices
- Use swept volume for moving polygons

### 11. Transformations Geométricas
#### Transformation Types
- **Translation**: T(dx, dy) - shift by (dx, dy)
- **Scaling**: S(sx, sy) - scale by factors
- **Rotation**: R(θ) - rotate by angle θ around origin
- **Reflection**: Flip across line
- **Shear**: Skew transformation
- **Combined**: Multiple transformations

#### Transformation Matrix
- 3x2 matrix for 2D affine:
  | a  c  tx |
  | b  d  ty |
  where x' = a*x + c*y + tx, y' = b*x + d*y + ty

#### Affine Transformation Properties
- Preserves straight lines
- Preserves parallelism
- Preserves ratio of lengths
- Preserves barycentric coordinates

#### Matrix Multiplication
- Combined transformation: M = T2 * T1
- Active vs. passive transformations
- Row vs. column vector convention

#### Specific Transformations

##### Rotation by Angle θ
```
| cos(θ)  -sin(θ)  0 |
| sin(θ)   cos(θ)  0 |
| 0         0       1 |
```

##### Scaling by (sx, sy)
```
| sx  0  0 |
| 0  sy  0 |
| 0  0  1 |
```

##### Translation by (tx, ty)
```
| 1  0  tx |
| 0  1  ty |
| 0  0  1 |
```

### 12. Cálculos de Distância e Área

#### Distance Formulas

##### Point-to-Point
```
d(P1, P2) = sqrt((x2-x1)^2 + (y2-y1)^2)
```

##### Point-to-Line
```
d(P, line AB) = |(B-A) x (P-A)| / |B-A|
```
Shortest distance from point to line defined by A and B.

##### Point-to-Segment
- Project point onto line
- If projection falls within segment, use point-to-line distance
- Otherwise, distance to closest endpoint

##### Line-to-Line
- Distance between skew lines (in 2D, parallel lines have distance, intersecting have 0)

##### Point-to-Polygon
- Minimum distance to any edge or vertex
- Could be point-to-segment for each edge

##### Polygon-to-Polygon
- Minimum distance between any pair of edges
- Or between features (vertex-vertex, vertex-edge, edge-edge)

#### Area Formulas

##### Triangle Area
```
Area = 0.5 * |(x2-x1)*(y3-y1) - (x3-x1)*(y2-y1)|
```
Or using Heron's formula with side lengths.

##### Rectangle Area
```
Area = width * height
```

##### Polygon Area (Shoelace)
```
Area = 0.5 * |Σ(xi * yi+1 - xi+1 * yi)|
```
for i = 0 to n-1, where (xn, yn) = (x0, y0)

##### Circle Area
```
Area = π * r^2
```

##### Circle Sector Area
```
Area = 0.5 * r^2 * θ (θ in radians)
```

##### Circle Segment Area
```
Area = 0.5 * r^2 * (θ - sin(θ))
```
where θ is the central angle.

### 13. Geometric Constraints
#### Common Constraints
- **Coincident**: Two points or point-to-line coincide
- **Parallel**: Two lines are parallel
- **Perpendicular**: Two lines are perpendicular
- **Tangent**: Line or circle tangent to curve
- **Concentric**: Two circles share center
- **Equal Length**: Two segments have same length
- **Equal Angle**: Two angles have same measure
- **Collinear**: Points or lines lie on same line

#### Constraint Solving
- Iterative relaxation
- Gaussian elimination for linear constraints
- Newton-Raphson for nonlinear constraints

## Integration with Other Engines

### Geometry <- Document Engine
- Object bounds calculation
- Transformation application
- Boolean operation operands
- Selection hit-testing

### Geometry <- Color Engine
- Color fills for shapes
- Gradient meshes
- Pattern tiling geometries

### Geometry <- Trapping Engine
- Edge detection for trap generation
- Overlap area calculation
- Gap detection between objects

### Geometry <- Preflight Engine
- Validation of geometric constraints
- Size/area checks
- Distance violations

### Geometry <- PDF Engine
- Path data extraction from PDF
- Transformation matrix preservation
- Boolean operation translation
- Vector-to-raster conversion parameters

## Algorithm Investigation Priority

1. **Boolean Operations**: How polygons are intersected/unioned/subtracted
2. **Offset/Inset Algorithms**: Parallel curve computation
3. **Curve Approximation**: Bézier fitting and simplification
4. **Area/Angle Calculations**: Numerical precision
5. **Intersection Robustness**: Edge cases and numerical stability
6. **Transformation Composing**: Matrix multiplication order
7. **Contour Orientation**: Winding number consistency

## Next Steps for Geometry Analysis

1. Use Ghidra to de CurveApprox.dll, GeoMapper.dll functions
2. Identify boolean operation implementation
3. Trace offset/inset algorithms
4. Document curve evaluation and flattening
5. Map intersection routines
6. Analyze transformation matrix handling
7. Investigate constraint solving if present