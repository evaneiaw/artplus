# ArtPro+ v250 - Automation Analysis

## Overview
The Automation module in ArtPro+ v250 handles task automation through action lists, commands, and sequential execution. This enables workflow automation for repetitive tasks, batch processing, and integration with external systems.

## Key DLLs
- **Automation.dll** (not directly listed but implied by module I) - Core automation engine
- **ActionList related modules**: Various action list handling
- **Command processing modules**: Individual command implementation
- **VariableEngine.dll** (88,104 bytes) - Variable data engine
- **VariablesCoreLayer.dll** (343,592 bytes) - Variable core
- **VariablesViewLayer.dll** (681,000 bytes) - Variable visualization
- **VariablesXMLLayer.dll** (72,744 bytes) - XML variable handling
- **XMLReportLayer.dll** (183,336 bytes) - XML report generation

## Automation Operations

### 1. Action Lists
#### Action List Structure
- **Ordered Sequence**: Ordered list of actions to execute
- **Nested Groups**: Sub-action lists within main list
- **Conditional Branching**: If/then/else logic
- **Loops**: For/while repetition
- **Variables**: Runtime variable substitution

#### Action List Format (likely XML or binary)
```xml
<ActionList>
    <Action id="1" type="command">
        <name>Open Document</name>
        <parameters>
            <filepath>path/to/file</filepath>
        </parameters>
    </Action>
    <Action id="2" type="command">
        <name>Resize Object</name>
        <parameters>
            <width>100</width>
            <height>200</height>
        </parameters>
    </Action>
</ActionList>
```

#### Action List Operations
- **Load**: Read action list from file
- **Save**: Write action list to file
- **Execute**: Run through actions sequentially
- **Pause/Resume**: Control execution
- **Stop**: Halt execution

### 2. Commands
#### Command Structure
- **Command ID**: Unique identifier
- **Command Name**: Human-readable name
- **Parameters**: Input values for command
- **Result**: Output from command execution
- **Error Handling**: What on failure

#### Command Categories
1. **Document Commands**
   - Open document
   - Save document
   - Save As
   - Close document
   - New document

2. **Object Commands**
   - Create object (rectangle, text, image)
   - Delete object
   - Duplicate object
   - Move object
   - Resize object
   - Rotate object
   - Transform object

3. **Property Commands**
   - Set color (fill/stroke)
   - Set transparency
   - Set stroke width
   - Set font
   - Set text content

4. **View Commands**
   - Zoom to area
   - Pan to position
   - Toggle visibility
   - Toggle layers
   - Change measurement units

5. **Output Commands**
   - Export PDF
   - Print
   - Generate plates
   - Create marks
   - Preflight check

#### Command Execution
1. **Parse**: Interpret command and parameters
2. **Validate**: Check parameter validity
3. **Execute**: Perform the operation
4. **Return**: Return result/status
5. **Error**: Handle failures

#### Command Parameter Types
- **String**: Text values, file paths
- **Number**: Integer or floating point
- **Boolean**: True/false
- **Color**: CMYK/RGB/Lab values
- **Enum**: Named values from list
- **Object Reference**: Document object ID
- **Action List**: Nested action list

### 3. Parâmetros (Parameters)
#### Parameter Types
- **Literal**: Fixed value in action list
- **Variable**: Substituted at runtime from variable data
- **Calculated**: Result of previous command
- **Environment**: System values (path, user, etc.)

#### Parameter Source
- **Action List Definition**: Hard-coded in list
- **User Prompt**: Requested at execution time
- **Variable Data**: From data source (XML, CSV, database)
- **Document State**: Current document properties
- **System**: Operating system values

#### Common Parameters
- **File Paths**: Input/output file locations
- **Color Values**: CMYK, RGB, Lab specifications
- **Geometric Values**: Dimensions, positions, angles
- **Text Content**: String values for text objects
- **Boolean Flags**: True/false options
- **Enumerated Values**: Selected from list (e.g., "CMYK", "RGB")
- **Object References**: IDs of document objects

### 4. Execução Sequencial (Sequential Execution)
#### Execution Flow
- **Linear Sequence**: Action 1, then 2, then 3, etc.
- **Branching**: If condition then A else B
- **Looping**: Repeat N times or while condition
- **Error Handling**: Continue on error, stop on error

#### Execution Graph
```
Action List
  ├── Action 1 (Open)
  │     └── Result: Success
  ├── Action 2 (Resize)
  │     └── Result: Success
  ├── Action 3 (Conditional)
  │     └── Condition: true
  │     └── → Action 4 (Red)
  │     └── → Action 5 (Blue)
  ├── Action 6 (Loop)
  │     ├── Init: i = 0
  │     ├── Condition: i < 10
  │     ├── Body: Execute actions, i++
  │     └── → i >= 10 ? Stop : Continue
  └── Action 7 (End)
```

#### Execution States
- **Pending**: Yet to execute
- **Running**: Currently executing
- **Paused**: Temporarily stopped
- **Completed**: Finished successfully
- **Failed**: Terminated with error
- **Stopped**: User-interrupted

#### Error Handling Strategies
- **Stop on First Error**: Halt execution
- **Continue on Error**: Skip failed action
- **Error Handler**: Custom function on error
- **Retry**: Attempt action again
- **Skip**: Move to next action

### 5. Integração com Documentos (Document Integration)
#### Document Workflow Integration
- **Open Document**: First action in many lists
- **Save/Export**: Final actions for output
- **Intermediate Steps**: Processing in between
- **Error Recovery**: What if document changes

#### Document State Management
- **Track Changes**: Monitor document modifications
- **Save Points**: Checkpoints for undo
- **Lock Document**: Prevent concurrent modification
- **Version Tracking**: Track document versions

#### Integration Patterns
- **Batch Processing**: Multiple documents in sequence
- **Watch Folder**: Monitor directory for new files
- **Triggered Automation**: Start from external event
- **Scheduled Automation**: Run at specified times
- **API Integration**: External system control

### 5. Formatos de Automação (Automation Formats)
#### Supported Formats
- **XML**: Human-readable action lists
- **JSON**: Compact action list format
- **Binary**: Efficient internal format
- **Esko Script**: Esko-specific scripting language
- **Eclipse Script**: Alternative scripting

#### XML Action List Example
```xml
<Automation xmlns="http://www.artesoft.com/automation">
    <Actions>
        <Action type="open_document">
            <path>file:///C:/docs/file.apf</path>
        </Action>
        <Action type="set_color">
            <object_ref>obj_123</object_ref>
            <color_space>CMYK</color_space>
            <values>
                <c>100</c>
                <m>0</m>
                <y>0</y>
                <k>0</k>
            </values>
        </Action>
        <!-- More actions -->
    </Actions>
</Automation>
```

#### JSON Action List Example
```json
{
  "actions": [
    {"type": "open_document", "path": "file.apf"},
    {"type": "set_color", "object": "obj_123", "color": {"c": 100, "m": 0, "y": 0, "k": 0}}
  ]
}
```

### 6. Variáveis (Variables)
#### Variable Data Structure
- **Variable Name**: Identifier for substitution
- **Variable Value**: The actual data
- **Variable Type**: Text, number, color, object
- **Variable Source**: Where value comes from

#### Variable Sources
- **XML Data**: `<variables>` file
- **CSV/TSV**: Tab-separated values
- **Database**: SQL query results
- **API**: Web service responses
- **User Input**: Prompted at runtime

#### Variable Substitution
```
Action: Set text content
Parameter: "{{customer_name}}"
Variable Data: {"customer_name": "Acme Corp"}
Result: "Set text content to 'Acme Corp'"
```

#### Variable Types
- **String**: Text values
- **Number**: Numeric values (integer/float)
- **Color**: CMYK/RGB/Lab specifications
- **Boolean**: True/false values
- **Object**: Document object references
- **Action List**: Nested action lists

### 7. Automating Document Workflows
#### Common Workflow Automations

1. **Batch Preflight**
   - Process multiple documents
   - Apply profile, generate reports
   - Output: Log files, corrected documents

2. **Batch Export**
   - Export multiple documents to PDF
   - Vary settings per document
   - Output: PDF files in specified locations

3. **Color Conversion Batch**
   - Convert colors between spaces
   - Apply ICC profiles
   - Output: Updated documents

4. **Mark Addition**
   - Add registration marks to multiple documents
   - Vary position per page size
   - Output: Documents with marks

5. **Layout Automation**
   - Apply step & repeat to multiple
   - Vary counts/sizes per document
   - Output: Imposition-ready files

6. **Variable Data Printing**
   - Merge variable data with templates
   - Generate personalized documents
   - Output: Unique documents per data row

#### Workflow Engine
- **Trigger**: Start event (folder watch, API call, UI)
- **Sequence**: Action list execution
- **Conditional**: Branch based on results
- **Error Handling**: What on failure
- **Completion**: Final state and output

### Integration with Other Engines

#### Automation <- Document Engine
- Document open/save/close operations
- Object creation/modification
- Document state tracking

#### Automation <- Color Engine
- Color set/get operations
- Color conversion within actions
- Profile application

#### Automation <- Geometry Engine
- Transformations within actions
- Position/size calculations
- Boolean operations

#### Automation <- Preflight Engine
- Built-in preflight checks
- Auto-fix mutators
- Report generation

#### Automation <- Marks Engine
- Mark generation commands
- Mark parameter control

#### Automation <- Trapping Engine
- Trap generation commands
- Trap parameter control

### Algorithm Investigation Priority

1. **Action List Parsing**: How action lists are read and interpreted
2. **Command Execution Engine**: Core command processing
3. **Variable Substitution**: Data binding logic
4. **Sequential Flow Control**: Branching and looping
5. **Error Recovery**: Error handling strategies
6. **Format Parsing**: XML/JSON/binary format handling

### Next Steps for Automation Analysis

1. Use Ghidra to decompile Automation.dll functions
2. Identify action list parsing routines
3. Trace command execution flow
4. Document variable substitution logic
5. Analyze sequential flow control
6. Investigate error handling strategies
7. Map format parsers (XML/JSON)

## Observations from Import Log
- **VariableEngine.dll (88KB)**: Core variable management
- **VariablesCoreLayer.dll (344KB)**: Variable processing
- **VariablesViewLayer.dll (681KB)**: Variable visualization
- **VariablesXMLLayer.dll (73KB)**: XML variable handling
- **XMLReportLayer.dll (183KB)**: XML report integration
- Automation integrates with variable data for VDP (variable data printing)