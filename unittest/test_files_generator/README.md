# IFF/RIFF Test File Generation System

This directory contains a JSON-based system for generating IFF/RIFF test files for comprehensive parser testing.

## Structure

```
test_files_generator/
├── definitions/           # JSON test file definitions
├── generate_test_files.py # Test file generator script
└── README.md             # This file
```

Generated files are created in the build directory at:
```
build/unittest/generated/
```

## Usage

### Automatic Generation with CMake

Test files are automatically generated during the build process. When you build the unittest target, CMake will:
1. Create the `generated/` directory in the build tree
2. Run `generate_test_files.py` on all JSON definitions
3. Place generated binary files in `CMAKE_CURRENT_BINARY_DIR/generated/`

```bash
# Build the project (automatically generates test files)
cmake -B build
cmake --build build
```

### Manual Generation

You can also run the generator manually:

```bash
# Generate all test files
python3 unittest/test_files_generator/generate_test_files.py \
    unittest/test_files_generator/definitions/*.json \
    -o build/unittest/generated

# Generate a single file
python3 unittest/test_files_generator/generate_test_files.py \
    unittest/test_files_generator/definitions/minimal_aiff.json \
    -o output/
```

## JSON Definition Format

Each test file is described by a JSON file that specifies:

- **name**: Test file name (without extension)
- **description**: What the test validates
- **format**: IFF, RIFF, RIFX, or RF64
- **root**: The root chunk structure
- **corruptions**: Optional intentional corruptions for error testing

### Chunk Structure

Each chunk can have:
- **id**: FourCC identifier (required)
- **type**: Container type tag (for FORM, LIST, etc.)
- **children**: Array of child chunks for containers
- **data**: Chunk payload data
- **size_override**: Override calculated size
- **size_marker**: Special values like MAX_U32
- **align**: Whether to add padding for odd sizes (default: true)
- **comment**: Documentation

### Data Specifications

Data can be specified as:

1. **Hex string**: `"0A0B0C0D"`
2. **Text**: `"Hello World"`
3. **Structured data**:
   - `{"type": "hex", "value": "AABBCCDD"}`
   - `{"type": "text", "value": "Sample text"}`
   - `{"type": "zeros", "size": 100}`
   - `{"type": "pattern", "value": "AA55", "repeat": 50}`
   - `{"type": "random", "size": 256}`
   - `{"type": "file", "value": "path/to/file"}`

### Example Definition

```json
{
  "name": "simple_form",
  "description": "Simple FORM chunk with data",
  "format": "IFF",
  "root": {
    "id": "FORM",
    "type": "TEST",
    "children": [
      {
        "id": "DATA",
        "data": {
          "type": "text",
          "value": "Hello, IFF!"
        }
      }
    ]
  }
}
```

## Test Categories

### 1. Basic Format Tests
- `minimal_aiff.json` - Minimal valid AIFF file
- `minimal_wave.json` - Minimal valid WAVE file
- `prop_defaults.json` - PROP default handling

### 2. Error Handling Tests
- `truncated.json` - Truncated file tests
- `odd_sized.json` - Odd-sized chunk padding
- `deeply_nested.json` - Maximum nesting depth

### 3. RF64 Tests
- `rf64_large.json` - RF64 with ds64 chunk

### 4. Corruption Tests
Files can include corruption specifications:
```json
"corruptions": [
  {
    "type": "truncate_at",
    "offset": -6,
    "comment": "Truncate in middle of header"
  }
]
```

## Generated Output

For each JSON definition, the generator creates:
1. Binary test file (`.iff`, `.riff`, `.rifx`, or `.rf64`)
2. Hex dump file (`.hex`) for debugging

## Adding New Test Files

1. Create a JSON file in `definitions/`
2. Follow the schema in `docs/test-file-schema.json`
3. Run the generator
4. Add corresponding test cases in the parser tests

## Validation

The generated files should be validated against the parser to ensure:
- Correct format detection
- Proper chunk parsing
- Expected error handling
- Correct PROP default application
- Handler invocation with proper context

## Tips

- Keep test files minimal and focused
- Document the purpose of each test clearly
- Use comments in JSON to explain complex structures
- Generate hex dumps for debugging parser issues
- Test both valid and invalid file structures