//
// Created by igor on 14/08/2025.
//
// Comprehensive tests for all generated test files

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "unittest_config.h"
#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>

using namespace iff;
using namespace std::string_literals;

static std::unique_ptr<std::ifstream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("test generated files - minimal formats") {
    SUBCASE("minimal_aiff.iff") {
        auto is = load_test("minimal_aiff.iff");
        REQUIRE(is->good());
        
        std::vector<std::string> chunk_ids;
        std::vector<std::uint64_t> chunk_sizes;
        
        CHECK_NOTHROW(
            for_each_chunk(*is, [&](const auto& chunk) {
                chunk_ids.push_back(chunk.header.id.to_string());
                chunk_sizes.push_back(chunk.header.size);
            })
        );
        
        // AIFF should have COMM and SSND chunks
        CHECK(chunk_ids.size() == 2);
        CHECK(chunk_ids[0] == "COMM");
        CHECK(chunk_ids[1] == "SSND");
    }
    
    SUBCASE("minimal_wave.riff") {
        auto is = load_test("minimal_wave.riff");
        REQUIRE(is->good());
        
        // RIFF format is now supported
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // Should have RIFF container
        CHECK(it->has_next());
        CHECK(it->current().header.id == "RIFF"_4cc);
        CHECK(it->current().header.is_container);
        if (it->current().header.type) {
            CHECK(it->current().header.type->to_string() == "WAVE");
        }
        
        // Iterate through all chunks
        int chunk_count = 0;
        while (it->has_next()) {
            chunk_count++;
            it->next();
        }
        CHECK(chunk_count >= 3);  // RIFF, fmt, data
    }
}

TEST_CASE("test generated files - nesting") {
    SUBCASE("deeply_nested.iff") {
        auto is = load_test("deeply_nested.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        int max_depth = 0;
        int container_count = 0;
        int data_chunk_count = 0;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            max_depth = std::max(max_depth, chunk.depth);
            
            if (chunk.header.is_container) {
                container_count++;
            } else if (chunk.header.id == "DATA"_4cc) {
                data_chunk_count++;
            }
            
            it->next();
        }
        
        // Should have nested containers
        CHECK(container_count >= 3);
        CHECK(data_chunk_count == 1);
        CHECK(max_depth >= 3);
    }
    
    SUBCASE("deeply_nested_correct.iff") {
        auto is = load_test("deeply_nested_correct.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // Track the nesting structure
        std::vector<std::string> container_types;
        int max_depth = 0;
        int data_chunks = 0;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            max_depth = std::max(max_depth, chunk.depth);
            
            if (chunk.header.is_container && chunk.header.type) {
                container_types.push_back(chunk.header.type->to_string());
            } else if (chunk.header.id == "DATA"_4cc) {
                data_chunks++;
            }
            
            it->next();
        }
        
        // Just verify we have deep nesting and some containers
        CHECK(max_depth >= 3);  // Should have at least some nesting
        CHECK(container_types.size() >= 3);  // Should have multiple containers
        CHECK(data_chunks >= 1);  // Should have at least one DATA chunk
    }
    
    SUBCASE("form_in_form.iff") {
        auto is = load_test("form_in_form.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        std::vector<std::string> structure;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.is_container) {
                structure.push_back(chunk.header.id.to_string() + ":" + 
                    (chunk.header.type ? chunk.header.type->to_string() : ""));
            } else {
                structure.push_back(chunk.header.id.to_string());
            }
            
            it->next();
        }
        
        // Should have nested FORM structures
        CHECK(structure.size() >= 7);  // FORM:COMP, NAME, FORM:PIC1, SIZE, DATA, FORM:SND1, RATE, DATA, DESC
        CHECK(structure[0] == "FORM:COMP");  // Root composite FORM
        CHECK(structure[1] == "NAME");       // First chunk in root
        CHECK(structure[2] == "FORM:PIC1");  // First nested FORM
        // Check for second nested FORM
        bool found_snd1 = false;
        for (const auto& s : structure) {
            if (s == "FORM:SND1") {
                found_snd1 = true;
                break;
            }
        }
        CHECK(found_snd1);
    }
}

TEST_CASE("test generated files - odd sized chunks") {
    SUBCASE("odd_sized_chunks.iff") {
        auto is = load_test("odd_sized_chunks.iff");
        REQUIRE(is->good());
        
        std::vector<std::string> chunk_ids;
        std::vector<std::uint64_t> chunk_sizes;
        std::vector<std::vector<std::byte>> chunk_data;
        
        CHECK_NOTHROW(
            for_each_chunk(*is, [&](const auto& chunk) {
                chunk_ids.push_back(chunk.header.id.to_string());
                chunk_sizes.push_back(chunk.header.size);
                
                if (chunk.reader) {
                    chunk_data.push_back(chunk.reader->read_all());
                }
            })
        );
        
        CHECK(chunk_ids.size() == 5);
        
        // Check odd sizes
        CHECK(chunk_ids[0] == "ODD1");
        CHECK(chunk_sizes[0] == 1);
        CHECK(chunk_data[0].size() == 1);
        
        CHECK(chunk_ids[1] == "EVN2");
        CHECK(chunk_sizes[1] == 2);
        CHECK(chunk_data[1].size() == 2);
        
        CHECK(chunk_ids[2] == "ODD3");
        CHECK(chunk_sizes[2] == 3);
        CHECK(chunk_data[2].size() == 3);
        
        CHECK(chunk_ids[3] == "ODD5");
        CHECK(chunk_sizes[3] == 5);
        CHECK(chunk_data[3].size() == 5);
        
        CHECK(chunk_ids[4] == "EVN4");
        CHECK(chunk_sizes[4] == 4);
        CHECK(chunk_data[4].size() == 4);
    }
}

TEST_CASE("test generated files - PROP defaults") {
    SUBCASE("prop_defaults.iff") {
        auto is = load_test("prop_defaults.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        bool found_list = false;
        bool found_prop = false;
        bool found_form1 = false;
        bool found_form2 = false;
        std::vector<std::string> prop_chunks;
        std::vector<std::string> form1_chunks;
        std::vector<std::string> form2_chunks;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == "LIST"_4cc) {
                found_list = true;
                CHECK(chunk.header.is_container);
                CHECK(chunk.header.type);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "ILBM");
                }
            } else if (chunk.header.id == "PROP"_4cc) {
                found_prop = true;
                CHECK(chunk.header.is_container);
                CHECK(chunk.header.type);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "ILBM");
                }
            } else if (chunk.header.id == "FORM"_4cc) {
                if (!found_form1) {
                    found_form1 = true;
                } else {
                    found_form2 = true;
                }
                CHECK(chunk.header.is_container);
                CHECK(chunk.header.type);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "ILBM");
                }
            } else if (!chunk.header.is_container) {
                // Track which container this chunk belongs to
                if (chunk.current_container && *chunk.current_container == "PROP"_4cc) {
                    prop_chunks.push_back(chunk.header.id.to_string());
                } else if (chunk.current_form && *chunk.current_form == "ILBM"_4cc) {
                    if (!found_form2) {
                        form1_chunks.push_back(chunk.header.id.to_string());
                    } else {
                        form2_chunks.push_back(chunk.header.id.to_string());
                    }
                }
            }
            
            it->next();
        }
        
        CHECK(found_list);
        CHECK(found_prop);
        CHECK(found_form1);
        CHECK(found_form2);
        
        // PROP should contain BMHD and CMAP
        CHECK(prop_chunks.size() == 2);
        if (prop_chunks.size() >= 2) {
            CHECK(prop_chunks[0] == "BMHD");
            CHECK(prop_chunks[1] == "CMAP");
        }
        
        // Forms should have their own chunks
        CHECK(form1_chunks.size() >= 1);
        CHECK(form2_chunks.size() >= 1);
    }
}

TEST_CASE("test generated files - truncated files") {
    SUBCASE("truncated_header.iff") {
        auto is = load_test("truncated_header.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        int chunk_count = 0;
        bool got_exception = false;
        
        try {
            while (it->has_next()) {
                chunk_count++;
                it->next();
            }
        } catch (const std::exception& e) {
            got_exception = true;
            // Should get an exception due to truncation
        }
        
        // The file is truncated, but the iterator might still successfully
        // read the container header before hitting the truncation
        // Just verify we don't read too many chunks
        CHECK((got_exception || chunk_count <= 2));  // May read FORM header and possibly one chunk
    }
}

TEST_CASE("test generated files - RF64 format") {
    SUBCASE("rf64_basic.rf64") {
        auto is = load_test("rf64_basic.rf64");
        REQUIRE(is->good());
        
        // RF64 is now supported
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // Should have RF64 container
        CHECK(it->has_next());
        CHECK(it->current().header.id == "RF64"_4cc);
        CHECK(it->current().header.is_container);
    }
    
    SUBCASE("rf64_with_table.rf64") {
        auto is = load_test("rf64_with_table.rf64");
        REQUIRE(is->good());
        
        // RF64 is now supported
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // Should have RF64 container
        CHECK(it->has_next());
        CHECK(it->current().header.id == "RF64"_4cc);
        CHECK(it->current().header.is_container);
    }
}

TEST_CASE("test generated files - container validation") {
    SUBCASE("verify container types") {
        auto is = load_test("form_in_form.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            // Containers should have type tags
            if (chunk.header.is_container) {
                if (chunk.header.id == "FORM"_4cc || 
                    chunk.header.id == "LIST"_4cc) {
                    CHECK(chunk.header.type.has_value());
                } else if (chunk.header.id == "CAT "_4cc) {
                    // CAT containers don't have type tags
                    CHECK(!chunk.header.type.has_value());
                }
            }
            
            it->next();
        }
    }
}

TEST_CASE("test generated files - chunk reader functionality") {
    SUBCASE("read chunk data") {
        auto is = load_test("odd_sized_chunks.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            if (!chunk.header.is_container && chunk.reader) {
                // Test various reader methods
                auto initial_remaining = chunk.reader->remaining();
                auto initial_offset = chunk.reader->offset();
                
                CHECK(initial_offset == 0);
                CHECK(initial_remaining == chunk.header.size);
                CHECK(chunk.reader->size() == chunk.header.size);
                
                // Read one byte
                if (chunk.header.size > 0) {
                    std::byte buffer[1];
                    auto read_count = chunk.reader->read(buffer, 1);
                    CHECK(read_count == 1);
                    CHECK(chunk.reader->offset() == 1);
                    CHECK(chunk.reader->remaining() == chunk.header.size - 1);
                    
                    // Read the rest
                    auto rest = chunk.reader->read_all();
                    CHECK(rest.size() == chunk.header.size - 1);
                    CHECK(chunk.reader->remaining() == 0);
                }
            }
            
            it->next();
        }
    }
    
    SUBCASE("skip functionality") {
        auto is = load_test("minimal_aiff.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            if (!chunk.header.is_container && chunk.reader) {
                if (chunk.header.size >= 4) {
                    // Skip first 4 bytes
                    CHECK(chunk.reader->skip(4));
                    CHECK(chunk.reader->offset() == 4);
                    CHECK(chunk.reader->remaining() == chunk.header.size - 4);
                }
            }
            
            it->next();
        }
    }
}

TEST_CASE("test generated files - handler registry") {
    SUBCASE("handler invocation") {
        auto is = load_test("odd_sized_chunks.iff");
        REQUIRE(is->good());
        
        handler_registry handlers;
        std::vector<std::string> begin_events;
        std::vector<std::string> end_events;
        
        // Register handlers for specific chunks
        handlers.on_chunk("ODD1"_4cc, [&](const chunk_event& e) {
            if (e.type == chunk_event_type::begin) {
                begin_events.push_back("ODD1");
                CHECK(e.reader != nullptr);
            } else {
                end_events.push_back("ODD1");
                CHECK(e.reader == nullptr);
            }
        });
        
        handlers.on_chunk("EVN2"_4cc, [&](const chunk_event& e) {
            if (e.type == chunk_event_type::begin) {
                begin_events.push_back("EVN2");
            } else {
                end_events.push_back("EVN2");
            }
        });
        
        parse(*is, handlers);
        
        // Should have both begin and end events
        CHECK(begin_events.size() == 2);
        CHECK(end_events.size() == 2);
        CHECK(begin_events[0] == "ODD1");
        CHECK(begin_events[1] == "EVN2");
        CHECK(end_events[0] == "ODD1");
        CHECK(end_events[1] == "EVN2");
    }
    
    SUBCASE("form-specific handlers") {
        auto is = load_test("prop_defaults.iff");
        REQUIRE(is->good());
        
        handler_registry handlers;
        std::vector<std::string> ilbm_chunks;
        
        // Register handler for BODY chunks in ILBM forms
        handlers.on_chunk_in_form("ILBM"_4cc, "BODY"_4cc, [&](const chunk_event& e) {
            if (e.type == chunk_event_type::begin) {
                ilbm_chunks.push_back("BODY in ILBM");
                CHECK(e.current_form.has_value());
                CHECK(*e.current_form == "ILBM"_4cc);
            }
        });
        
        parse(*is, handlers);
        
        // Should be called for BODY chunks within ILBM forms (there are 2)
        CHECK(ilbm_chunks.size() == 2);
    }
}

TEST_CASE("test generated files - error handling") {
    SUBCASE("graceful EOF handling") {
        auto is = load_test("truncated_header.iff");
        REQUIRE(is->good());
        
        // Should handle truncated files gracefully
        std::vector<std::string> chunks_found;
        bool had_error = false;
        
        try {
            for_each_chunk(*is, [&](const auto& chunk) {
                chunks_found.push_back(chunk.header.id.to_string());
            });
        } catch (const io_error&) {
            had_error = true;
        }
        
        // File is truncated, so we should either get an error
        // or process only the chunks before truncation
        CHECK((had_error || chunks_found.size() <= 1));
    }
}

TEST_CASE("test generated files - file format detection") {
    SUBCASE("IFF format detection") {
        auto is = load_test("minimal_aiff.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // First chunk should be FORM
        CHECK(it->has_next());
        const auto& chunk = it->current();
        CHECK(chunk.header.id == "FORM"_4cc);
        CHECK(chunk.header.is_container);
    }
    
    // RIFF format detection is tested in "minimal_wave.riff" test above
}