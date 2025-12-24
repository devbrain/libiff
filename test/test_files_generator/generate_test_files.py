#!/usr/bin/env python3
"""
IFF/RIFF Test File Generator

Generates binary IFF/RIFF files from JSON descriptions.
"""

import json
import struct
import os
import sys
import argparse
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple
import random


class IFFGenerator:
    """Generate IFF/RIFF files from JSON descriptions."""
    
    def __init__(self, definition: Dict[str, Any]):
        self.definition = definition
        self.format = definition['format']
        self.endian = self._determine_endian()
        self.output = bytearray()
        
    def _determine_endian(self) -> str:
        """Determine byte order based on format."""
        endian = self.definition.get('endian', 'auto')
        if endian == 'auto':
            if self.format in ['RIFF', 'RF64']:
                return 'little'
            else:  # IFF, RIFX
                return 'big'
        return endian
    
    def _pack_u16(self, value: int) -> bytes:
        """Pack 16-bit unsigned integer."""
        fmt = '<H' if self.endian == 'little' else '>H'
        return struct.pack(fmt, value)
    
    def _pack_u32(self, value: int) -> bytes:
        """Pack 32-bit unsigned integer."""
        fmt = '<I' if self.endian == 'little' else '>I'
        return struct.pack(fmt, value)
    
    def _pack_u64(self, value: int) -> bytes:
        """Pack 64-bit unsigned integer."""
        fmt = '<Q' if self.endian == 'little' else '>Q'
        return struct.pack(fmt, value)
    
    def _pack_fourcc(self, fourcc: str) -> bytes:
        """Pack FourCC identifier."""
        # Pad with spaces if needed
        fourcc = fourcc.ljust(4)[:4]
        return fourcc.encode('ascii')
    
    def _generate_data(self, data_spec: Any) -> bytes:
        """Generate chunk data from specification."""
        if data_spec is None:
            return b''
        
        if isinstance(data_spec, str):
            # Simple hex string or text
            if all(c in '0123456789ABCDEFabcdef ' for c in data_spec):
                # Hex string
                hex_str = data_spec.replace(' ', '')
                return bytes.fromhex(hex_str)
            else:
                # Text string
                return data_spec.encode('utf-8')
        
        elif isinstance(data_spec, dict):
            data_type = data_spec.get('type', 'hex')
            
            if data_type == 'hex':
                hex_str = data_spec['value'].replace(' ', '')
                return bytes.fromhex(hex_str)
            
            elif data_type == 'text':
                return data_spec['value'].encode('utf-8')
            
            elif data_type == 'zeros':
                size = data_spec.get('size', 0)
                return b'\x00' * size
            
            elif data_type == 'pattern':
                pattern = bytes.fromhex(data_spec['value'].replace(' ', ''))
                repeat = data_spec.get('repeat', 1)
                return pattern * repeat
            
            elif data_type == 'random':
                size = data_spec.get('size', 0)
                return bytes(random.randint(0, 255) for _ in range(size))
            
            elif data_type == 'file':
                file_path = data_spec['value']
                with open(file_path, 'rb') as f:
                    return f.read()
            
            elif data_type == 'ds64':
                # Special handling for ds64 chunk
                return self._generate_ds64(data_spec)
        
        return b''
    
    def _generate_ds64(self, spec: Dict[str, Any]) -> bytes:
        """Generate ds64 chunk data for RF64."""
        data = bytearray()
        
        # RIFF size (64-bit)
        riff_size = spec.get('riff_size', 0)
        data.extend(self._pack_u64(riff_size))
        
        # Data chunk size (64-bit)
        data_size = spec.get('data_size', 0)
        data.extend(self._pack_u64(data_size))
        
        # Sample count (64-bit)
        sample_count = spec.get('sample_count', 0)
        data.extend(self._pack_u64(sample_count))
        
        # Table entry count (32-bit)
        table = spec.get('table', [])
        data.extend(self._pack_u32(len(table)))
        
        # Table entries
        for entry in table:
            chunk_id = self._pack_fourcc(entry['chunk_id'])
            chunk_size = self._pack_u64(entry['chunk_size'])
            data.extend(chunk_id)
            data.extend(chunk_size)
        
        return bytes(data)
    
    def _generate_chunk(self, chunk: Dict[str, Any]) -> Tuple[bytes, int]:
        """
        Generate a chunk and return (data, actual_size).
        actual_size is the size to write in the header (may differ from data length).
        """
        chunk_id = chunk['id']
        chunk_type = chunk.get('type')
        children = chunk.get('children', [])
        
        # Start with chunk ID
        output = bytearray()
        output.extend(self._pack_fourcc(chunk_id))
        
        # Reserve space for size
        size_offset = len(output)
        output.extend(b'\x00\x00\x00\x00')
        
        # Determine if this is a container
        is_container = chunk_id in ['FORM', 'LIST', 'CAT ', 'PROP', 'RIFF', 'RIFX', 'RF64']
        
        # Start tracking payload
        payload_start = len(output)
        
        if is_container:
            # Add type tag
            if chunk_type:
                output.extend(self._pack_fourcc(chunk_type))
            
            # Add children
            for child in children:
                child_data, _ = self._generate_chunk(child)
                output.extend(child_data)
        else:
            # Add data
            data = self._generate_data(chunk.get('data'))
            output.extend(data)
        
        # Calculate size
        payload_size = len(output) - payload_start
        
        # Handle special size markers
        if chunk.get('size_marker') == 'MAX_U32':
            size_value = 0xFFFFFFFF
        elif chunk.get('size_marker') == 'ZERO':
            size_value = 0
        elif 'size_override' in chunk:
            size_value = chunk['size_override']
        else:
            size_value = payload_size
        
        # Write size at reserved position
        size_bytes = self._pack_u32(size_value)
        output[size_offset:size_offset+4] = size_bytes
        
        # Add padding if needed
        if chunk.get('align', True) and (len(output) - 8) % 2 == 1:
            output.append(0)
        
        return bytes(output), size_value
    
    def generate(self) -> bytes:
        """Generate the complete file."""
        root = self.definition['root']
        file_data, _ = self._generate_chunk(root)
        
        # Apply any corruptions
        if 'corruptions' in self.definition:
            file_data = self._apply_corruptions(file_data, self.definition['corruptions'])
        
        return file_data
    
    def _apply_corruptions(self, data: bytes, corruptions: List[Dict[str, Any]]) -> bytes:
        """Apply intentional corruptions for testing."""
        data = bytearray(data)
        
        for corruption in corruptions:
            corr_type = corruption['type']
            
            if corr_type == 'truncate_at':
                offset = corruption['offset']
                if offset < 0:
                    offset = len(data) + offset
                data = data[:offset]
            
            elif corr_type == 'corrupt_bytes':
                offset = corruption['offset']
                length = corruption.get('length', 1)
                value = corruption.get('value', 'FF')
                corrupt_data = bytes.fromhex(value)
                for i in range(length):
                    if offset + i < len(data):
                        data[offset + i] = corrupt_data[i % len(corrupt_data)]
            
            elif corr_type == 'invalid_size':
                offset = corruption['offset']
                value = corruption['value']
                size_bytes = self._pack_u32(int(value, 0))
                data[offset:offset+4] = size_bytes
            
            elif corr_type == 'exceed_parent':
                # This would be implemented by modifying chunk sizes
                pass
        
        return bytes(data)


def generate_test_file(json_path: Path, output_dir: Path) -> None:
    """Generate a test file from JSON definition."""
    with open(json_path, 'r') as f:
        definition = json.load(f)
    
    generator = IFFGenerator(definition)
    file_data = generator.generate()
    
    # Determine output filename
    name = definition['name']
    format_ext = {
        'IFF': '.iff',
        'RIFF': '.riff',
        'RIFX': '.rifx',
        'RF64': '.rf64'
    }
    ext = format_ext.get(definition['format'], '.bin')
    output_path = output_dir / f"{name}{ext}"
    
    # Write file
    with open(output_path, 'wb') as f:
        f.write(file_data)
    
    print(f"Generated: {output_path} ({len(file_data)} bytes)")
    
    # Also generate a hex dump for debugging
    hex_path = output_dir / f"{name}.hex"
    with open(hex_path, 'w') as f:
        for i in range(0, len(file_data), 16):
            hex_part = ' '.join(f'{b:02x}' for b in file_data[i:i+16])
            ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in file_data[i:i+16])
            f.write(f"{i:08x}  {hex_part:<48}  |{ascii_part}|\n")
    
    print(f"  Hex dump: {hex_path}")


def main():
    parser = argparse.ArgumentParser(description='Generate IFF/RIFF test files from JSON')
    parser.add_argument('input', nargs='*', help='JSON definition files (or directory)')
    parser.add_argument('-o', '--output', default='generated', help='Output directory')
    parser.add_argument('-a', '--all', action='store_true', help='Generate all files in definitions/')
    
    args = parser.parse_args()
    
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if args.all:
        # Find all JSON files in definitions directory
        definitions_dir = Path('test/test-files/definitions')
        if not definitions_dir.exists():
            definitions_dir = Path('definitions')
        if not definitions_dir.exists():
            print(f"Error: Cannot find definitions directory")
            sys.exit(1)
        
        json_files = list(definitions_dir.glob('*.json'))
    else:
        json_files = []
        for input_path in args.input:
            path = Path(input_path)
            if path.is_dir():
                json_files.extend(path.glob('*.json'))
            else:
                json_files.append(path)
    
    if not json_files:
        print("No JSON files to process")
        sys.exit(1)
    
    print(f"Generating {len(json_files)} test files...")
    for json_path in json_files:
        try:
            generate_test_file(json_path, output_dir)
        except Exception as e:
            print(f"Error processing {json_path}: {e}")
            import traceback
            traceback.print_exc()
    
    print(f"\nGenerated files in: {output_dir}")


if __name__ == '__main__':
    main()