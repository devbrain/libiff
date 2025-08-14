//
// Created by igor on 13/08/2025.
//

#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>
#include "iff85_chunk_iterator.hh"
#include "riff_chunk_iterator.hh"

namespace iff {
    
    std::unique_ptr<chunk_iterator> chunk_iterator::get_iterator(std::istream& stream) {
        // Use default options
        parse_options default_opts;
        return get_iterator(stream, default_opts);
    }
    
    std::unique_ptr<chunk_iterator> chunk_iterator::get_iterator(std::istream& stream, const parse_options& options) {
        // Save current position
        auto start_pos = stream.tellg();
        if (start_pos == std::streampos(-1)) {
            THROW_PARSE("Failed to get stream position");
        }
        
        // Read first 4 bytes to detect format
        char magic[4];
        stream.read(magic, 4);
        if (!stream.good()) {
            THROW_PARSE("Failed to read file magic");
        }
        
        // Restore position
        stream.seekg(start_pos);
        
        // Create fourcc from magic bytes
        fourcc id(magic[0], magic[1], magic[2], magic[3]);
        
        // Detect format and create appropriate iterator
        if (id == "FORM"_4cc || id == "LIST"_4cc || id == "CAT "_4cc) {
            // IFF-85 format
            return std::make_unique<iff85_chunk_iterator>(stream, options);
        } else if (id == "RIFF"_4cc || id == "RF64"_4cc || id == "RIFX"_4cc) {
            // RIFF format
            return std::make_unique<riff_chunk_iterator>(stream, options);
        } else {
            THROW_PARSE("Unknown file format: " + id.to_string());
        }
    }
    
} // namespace iff