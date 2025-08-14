# libiff Examples

This directory contains example programs demonstrating various uses of the libiff library.

## Building the Examples

To build the examples, enable the `LIBIFF_BUILD_EXAMPLES` option when configuring CMake:

```bash
mkdir build
cd build
cmake -DLIBIFF_BUILD_EXAMPLES=ON ..
make
```

The example executables will be created in `build/bin/`.

## Examples Overview

### simple_example
A minimal example showing basic usage of libiff to parse any IFF/RIFF file and list its chunks.

```bash
./simple_example file.wav
```

### wav_parser
Parses WAV audio files and displays format information, sample rate, duration, and metadata.

```bash
./wav_parser audio.wav
```

Features:
- Reads format chunk (sample rate, channels, bit depth)
- Calculates duration from data chunk
- Handles compressed formats and extended format info
- Shows first bytes of audio data

### aiff_parser
Parses AIFF and AIFF-C audio files (big-endian IFF format).

```bash
./aiff_parser audio.aiff
```

Features:
- Reads Common chunk with 80-bit float sample rate
- Handles both AIFF and compressed AIFF-C
- Displays metadata (name, author, copyright, annotations)
- Shows markers if present

### structure_analyzer
Analyzes and displays the complete structure of any IFF/RIFF file.

```bash
./structure_analyzer file.riff
./structure_analyzer file.iff --verbose
```

Features:
- Shows complete chunk hierarchy
- Identifies file format (IFF, RIFF, RF64, etc.)
- Calculates statistics (chunk counts, size distribution)
- Reports most frequent chunk types
- Verbose mode shows file offsets and context

### chunk_extractor
Extracts specific chunks from files for analysis or processing.

```bash
# Extract all 'data' chunks
./chunk_extractor audio.wav data

# Extract and show hex dump
./chunk_extractor file.iff NAME --hex

# Extract and save to files
./chunk_extractor video.avi movi --save

# Extract all chunks (summary)
./chunk_extractor file.riff
```

Features:
- Extract specific chunk types
- Save chunks to separate files
- Display hex dumps
- Auto-detect text chunks
- Show chunk context (parent containers)

### secure_parser
Demonstrates security-hardened parsing of untrusted files.

```bash
# Parse with default security settings
./secure_parser untrusted.riff

# Custom limits: 500MB chunks, depth 20, 60 second timeout
./secure_parser large.wav 500 20 60
```

Security features:
- Configurable chunk size limits
- Maximum nesting depth enforcement
- Parse timeout protection
- Memory exhaustion prevention
- Truncation detection
- Detailed security violation reporting
- Safe handling of malformed data

## Common Use Cases

### Analyzing an Unknown File
```bash
# First, check the structure
./structure_analyzer mystery.file --verbose

# Then extract interesting chunks
./chunk_extractor mystery.file INFO --hex
```

### Validating Untrusted Files
```bash
# Run security analysis first
./secure_parser downloaded.riff

# If safe, proceed with parsing
./wav_parser downloaded.riff
```

### Extracting Audio Data
```bash
# Extract PCM data from WAV
./chunk_extractor audio.wav data --save

# Extract sound data from AIFF
./chunk_extractor audio.aiff SSND --save
```

### Debugging File Issues
```bash
# Check structure for corruption
./structure_analyzer corrupted.file

# Use secure parser for detailed analysis
./secure_parser corrupted.file 1000 50 120
```

## Error Handling

All examples include proper error handling and will report:
- File access errors
- Format violations
- Truncated data
- Invalid chunk sizes
- Memory allocation failures

## Tips

1. Always use binary mode when opening files
2. Start with `structure_analyzer` to understand file layout
3. Use `secure_parser` for untrusted files
4. Enable verbose mode for debugging
5. Check return codes in scripts (non-zero indicates errors)