//
// Test warning callback functionality across different scenarios
//

#include <doctest/doctest.h>
#include <iff/chunk_iterator.hh>
#include <iff/parse_options.hh>
#include <iff/fourcc.hh>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include "unittest_config.h"

using namespace iff;
using namespace std::string_literals;

// Helper to track warnings
struct warning_tracker {
    struct warning_info {
        std::uint64_t offset;
        std::string category;
        std::string message;
    };
    
    std::vector<warning_info> warnings;
    
    void operator()(std::uint64_t offset, std::string_view category, std::string_view message) {
        warnings.push_back({offset, std::string(category), std::string(message)});
    }
    
    bool has_warning(std::string_view category) const {
        return std::any_of(warnings.begin(), warnings.end(),
            [category](const warning_info& w) { return w.category == category; });
    }
    
    std::size_t count_category(std::string_view category) const {
        return std::count_if(warnings.begin(), warnings.end(),
            [category](const warning_info& w) { return w.category == category; });
    }
};

TEST_CASE("Warning callbacks - size_limit") {
    SUBCASE("Using generated large_chunks file") {
        std::string path = std::string(UNITTEST_PATH_TO_GENERATED_FILES) + "/large_chunks.iff";
        std::ifstream file(path, std::ios::binary);
        
        if (!file.good()) {
            // Generated file not available - skip test
            return;
        }
        
        parse_options opts;
        opts.strict = false;
        opts.max_chunk_size = 5000;  // 5KB limit
        
        warning_tracker tracker;
        opts.on_warning = std::ref(tracker);
        
        auto it = chunk_iterator::get_iterator(file, opts);
        
        while (it->has_next()) {
            it->next();
        }
        
        // Should have received size_limit warnings for large chunks
        CHECK(tracker.has_warning("size_limit"));
        CHECK(tracker.count_category("size_limit") >= 1);
    }
}

TEST_CASE("Warning callbacks - depth_limit") {
    SUBCASE("Using generated deeply_nested_riff file") {
        std::string path = std::string(UNITTEST_PATH_TO_GENERATED_FILES) + "/deeply_nested_riff.riff";
        std::ifstream file(path, std::ios::binary);
        
        if (!file.good()) {
            // Generated file not available - skip test
            return;
        }
        
        parse_options opts;
        opts.strict = false;
        opts.max_depth = 2;  // Allow only 2 levels
        
        warning_tracker tracker;
        opts.on_warning = std::ref(tracker);
        
        auto it = chunk_iterator::get_iterator(file, opts);
        
        while (it->has_next()) {
            it->next();
        }
        
        // Should have depth warnings since file has 6 levels of nesting
        CHECK(tracker.has_warning("depth_limit"));
        CHECK(tracker.count_category("depth_limit") >= 1);
    }
    
    SUBCASE("Using generated deeply_nested IFF file") {
        std::string path = std::string(UNITTEST_PATH_TO_GENERATED_FILES) + "/deeply_nested.iff";
        std::ifstream file(path, std::ios::binary);
        
        if (!file.good()) {
            // Generated file not available - skip test
            return;
        }
        
        parse_options opts;
        opts.strict = false;
        opts.max_depth = 3;  // Allow only 3 levels
        
        warning_tracker tracker;
        opts.on_warning = std::ref(tracker);
        
        auto it = chunk_iterator::get_iterator(file, opts);
        
        int max_depth_seen = 0;
        while (it->has_next()) {
            const auto& chunk = it->current();
            max_depth_seen = std::max(max_depth_seen, static_cast<int>(chunk.depth));
            it->next();
        }
        
        // Should not go deeper than max_depth
        CHECK(max_depth_seen <= 3);
        
        // Should have depth_limit warning
        CHECK(tracker.has_warning("depth_limit"));
    }
}

TEST_CASE("Warning callbacks - handler variations") {
    SUBCASE("No warning handler set") {
        std::string path = std::string(UNITTEST_PATH_TO_GENERATED_FILES) + "/large_chunks.iff";
        std::ifstream file(path, std::ios::binary);
        
        if (!file.good()) {
            return;
        }
        
        parse_options opts;
        opts.strict = false;
        opts.max_chunk_size = 100;
        // No on_warning set
        
        auto it = chunk_iterator::get_iterator(file, opts);
        
        // Should not crash
        CHECK_NOTHROW([&]() {
            while (it->has_next()) {
                it->next();
            }
        }());
    }
    
    SUBCASE("Warning handler with category filtering") {
        std::string path = std::string(UNITTEST_PATH_TO_GENERATED_FILES) + "/deeply_nested.iff";
        std::ifstream file(path, std::ios::binary);
        
        if (!file.good()) {
            return;
        }
        
        parse_options opts;
        opts.strict = false;
        opts.max_chunk_size = 100;
        opts.max_depth = 2;
        
        std::map<std::string, int> category_counts;
        opts.on_warning = [&category_counts](std::uint64_t, std::string_view category, std::string_view) {
            category_counts[std::string(category)]++;
        };
        
        auto it = chunk_iterator::get_iterator(file, opts);
        
        while (it->has_next()) {
            it->next();
        }
        
        // Check we got the expected categories
        bool has_size_limit = category_counts.count("size_limit") > 0;
        bool has_depth_limit = category_counts.count("depth_limit") > 0;
        CHECK((has_size_limit || has_depth_limit));
    }
}