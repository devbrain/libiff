#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

// Platform-specific includes for memory mapping
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace iff {

// ==================== Core Types ====================

// FourCC implementation with compile-time and runtime support
class fourcc {
private:
std::uint32_t value_;

    static constexpr std::uint32_t make_value(char c0, char c1, char c2, char c3) noexcept {
        // Always store in native byte order for fast comparison
        return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c0)) << 0) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c1)) << 8) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c2)) << 16) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c3)) << 24);
    }

public:
constexpr fourcc() noexcept : value_(make_value(' ', ' ', ' ', ' ')) {}
    
    constexpr fourcc(char c0, char c1, char c2, char c3) noexcept 
        : value_(make_value(c0, c1, c2, c3)) {}
    
    // Compile-time constructor from string literal
    template<std::size_t N>
    constexpr fourcc(const char (&str)[N]) noexcept : value_(0) {
        static_assert(N >= 1 && N <= 5, "String literal must be 1-4 characters");
        char c0 = (N > 1) ? str[0] : ' ';
        char c1 = (N > 2) ? str[1] : ' ';
        char c2 = (N > 3) ? str[2] : ' ';
        char c3 = (N > 4) ? str[3] : ' ';
        value_ = make_value(c0, c1, c2, c3);
    }
    
    // Runtime constructor from string_view
    explicit fourcc(std::string_view str) : value_(0) {
        char chars[4] = {' ', ' ', ' ', ' '};
        std::copy_n(str.begin(), std::min(str.length(), size_t(4)), chars);
        value_ = make_value(chars[0], chars[1], chars[2], chars[3]);
    }
    
    // Construct from raw bytes (for reading from files)
    static fourcc from_bytes(const std::byte* data, std::endian file_endian) noexcept {
        fourcc result;
        std::memcpy(&result.value_, data, 4);
        if (file_endian != std::endian::native) {
            result.value_ = std::byteswap(result.value_);
        }
        return result;
    }
    
    // Get as bytes for writing
    void to_bytes(std::byte* data, std::endian file_endian) const noexcept {
        std::uint32_t val = value_;
        if (file_endian != std::endian::native) {
            val = std::byteswap(val);
        }
        std::memcpy(data, &val, 4);
    }
    
    std::array<char, 4> chars() const noexcept {
        return {{
            static_cast<char>(value_ & 0xFF),
            static_cast<char>((value_ >> 8) & 0xFF),
            static_cast<char>((value_ >> 16) & 0xFF),
            static_cast<char>((value_ >> 24) & 0xFF)
        }};
    }
    
    std::string to_string() const {
        auto ch = chars();
        return std::string(ch.begin(), ch.end());
    }
    
    constexpr bool operator==(const fourcc& other) const noexcept {
        return value_ == other.value_;
    }
    
    constexpr bool operator!=(const fourcc& other) const noexcept {
        return value_ != other.value_;
    }
    
    constexpr bool operator<(const fourcc& other) const noexcept {
        return value_ < other.value_;
    }
    
    friend struct std::hash<fourcc>;
};

// User-defined literal
constexpr fourcc operator""_4cc(const char* str, std::size_t len) {
return fourcc(
len > 0 ? str[0] : ' ',
len > 1 ? str[1] : ' ',
len > 2 ? str[2] : ' ',
len > 3 ? str[3] : ' '
);
}

// Common chunk IDs
namespace chunk_id {
inline constexpr fourcc FORM = "FORM"_4cc;
inline constexpr fourcc LIST = "LIST"_4cc;
inline constexpr fourcc CAT  = "CAT "_4cc;
inline constexpr fourcc PROP = "PROP"_4cc;
inline constexpr fourcc RIFF = "RIFF"_4cc;
inline constexpr fourcc RIFX = "RIFX"_4cc;
inline constexpr fourcc RF64 = "RF64"_4cc;
inline constexpr fourcc ds64 = "ds64"_4cc;
}

// Error types
enum class parse_error {
end_of_file,
invalid_magic,
chunk_too_large,
invalid_size,
unsupported_format,
io_error,
corrupt_data,
max_depth_exceeded,
bounds_exceeded
};

// Result type using C++23 expected (or backport)
template<typename T>
using parse_result = std::expected<T, parse_error>;

// Byte order for file formats
enum class byte_order {
little,  // RIFF, RF64
big      // IFF, RIFX
};

// ==================== I/O Abstraction ====================

// Abstract reader interface
class reader {
public:
virtual ~reader() = default;
virtual std::size_t read(void* dst, std::size_t size) = 0;
virtual bool seek(std::uint64_t offset) = 0;
virtual std::uint64_t tell() const = 0;
virtual std::uint64_t size() const = 0;

    // Convenience methods
    parse_result<std::vector<std::byte>> read_exact(std::size_t size) {
        std::vector<std::byte> buffer(size);
        if (read(buffer.data(), size) != size) {
            return std::unexpected(parse_error::end_of_file);
        }
        return buffer;
    }
    
    template<typename T>
    parse_result<T> read_pod(byte_order bo) {
        std::array<std::byte, sizeof(T)> buffer;
        if (read(buffer.data(), sizeof(T)) != sizeof(T)) {
            return std::unexpected(parse_error::end_of_file);
        }
        
        T value;
        std::memcpy(&value, buffer.data(), sizeof(T));
        
        if constexpr (sizeof(T) > 1) {
            if (bo != byte_order::native()) {
                value = std::byteswap(value);
            }
        }
        
        return value;
    }
    
    parse_result<fourcc> read_fourcc() {
        std::array<std::byte, 4> buffer;
        if (read(buffer.data(), 4) != 4) {
            return std::unexpected(parse_error::end_of_file);
        }
        // FourCC is always read as-is, no endian swap
        return fourcc::from_bytes(buffer.data(), std::endian::native);
    }

private:
static constexpr byte_order native() {
return (std::endian::native == std::endian::little)
? byte_order::little
: byte_order::big;
}
};

// Stream-based reader
class stream_reader final : public reader {
std::istream& stream_;
std::uint64_t size_;

public:
explicit stream_reader(std::istream& stream) : stream_(stream) {
auto pos = stream_.tellg();
stream_.seekg(0, std::ios::end);
size_ = static_cast<std::uint64_t>(stream_.tellg());
stream_.seekg(pos);
}

    std::size_t read(void* dst, std::size_t size) override {
        stream_.read(static_cast<char*>(dst), size);
        return static_cast<std::size_t>(stream_.gcount());
    }
    
    bool seek(std::uint64_t offset) override {
        stream_.clear();
        stream_.seekg(static_cast<std::streamoff>(offset));
        return stream_.good();
    }
    
    std::uint64_t tell() const override {
        return static_cast<std::uint64_t>(
            const_cast<std::istream&>(stream_).tellg()
        );
    }
    
    std::uint64_t size() const override { return size_; }
};

// Memory-based reader
class memory_reader final : public reader {
std::span<const std::byte> data_;
std::size_t pos_ = 0;

public:
    explicit memory_reader(std::span<const std::byte> data) 
        : data_(data) {}
    
    std::size_t read(void* dst, std::size_t size) override {
        size_t available = data_.size() - pos_;
        size_t to_read = std::min(size, available);
        std::memcpy(dst, data_.data() + pos_, to_read);
        pos_ += to_read;
        return to_read;
    }
    
    bool seek(std::uint64_t offset) override {
        if (offset > data_.size()) return false;
        pos_ = static_cast<std::size_t>(offset);
        return true;
    }
    
    std::uint64_t tell() const override { return pos_; }
    std::uint64_t size() const override { return data_.size(); }
};

// Memory-mapped file reader
class mmap_reader final : public reader {
struct impl;
std::unique_ptr<impl> pimpl_;

public:
explicit mmap_reader(const std::filesystem::path& path);
~mmap_reader();

    std::size_t read(void* dst, std::size_t size) override;
    bool seek(std::uint64_t offset) override;
    std::uint64_t tell() const override;
    std::uint64_t size() const override;
};

// ==================== Chunk Types ====================

// Source of chunk data
enum class chunk_source {
explicit_data,    // Normal chunk in file
prop_default      // Synthesized from PROP defaults
};

// Chunk header information
struct chunk_header {
fourcc id;
std::uint64_t size;           // Payload size (excluding padding)
std::uint64_t file_offset;    // Offset to chunk header in file
bool is_container;
std::optional<fourcc> type;   // For container chunks
chunk_source source = chunk_source::explicit_data;
};

// Bounded chunk reader
class chunk_reader {
reader* reader_;
std::uint64_t start_;
std::uint64_t size_;
std::uint64_t pos_ = 0;

public:
chunk_reader() = default;
    
    chunk_reader(reader* r, std::uint64_t start, std::uint64_t size)
        : reader_(r), start_(start), size_(size) {}
    
    std::size_t read(void* dst, std::size_t size) {
        size_t available = size_ - pos_;
        size_t to_read = std::min(size, available);
        if (to_read == 0) return 0;
        
        reader_->seek(start_ + pos_);
        size_t actual = reader_->read(dst, to_read);
        pos_ += actual;
        return actual;
    }
    
    bool skip(std::size_t size) {
        if (size > remaining()) return false;
        pos_ += size;
        return true;
    }
    
    std::uint64_t remaining() const { return size_ - pos_; }
    std::uint64_t offset() const { return start_; }
    std::uint64_t size() const { return size_; }
    
    // Convenience methods
    template<typename T>
    parse_result<T> read_pod(byte_order bo) {
        std::array<std::byte, sizeof(T)> buffer;
        if (read(buffer.data(), sizeof(T)) != sizeof(T)) {
            return std::unexpected(parse_error::end_of_file);
        }
        
        T value;
        std::memcpy(&value, buffer.data(), sizeof(T));
        
        if constexpr (sizeof(T) > 1) {
            if (bo != byte_order::native()) {
                value = std::byteswap(value);
            }
        }
        
        return value;
    }
    
    parse_result<std::string> read_string(std::size_t size) {
        std::string result(size, '\0');
        if (read(result.data(), size) != size) {
            return std::unexpected(parse_error::end_of_file);
        }
        return result;
    }

private:
static constexpr byte_order native() {
return (std::endian::native == std::endian::little)
? byte_order::little
: byte_order::big;
}
};

// ==================== Configuration ====================

struct parse_options {
bool strict = true;
std::uint64_t max_chunk_size = std::uint64_t(1) << 32;  // 4GB
bool allow_rf64 = true;
int max_depth = 64;

    // Warning callback
    using warning_handler = std::function<void(
        std::uint64_t offset,
        std::string_view category,
        std::string_view message
    )>;
    warning_handler on_warning;
};

struct structured_parse_options : parse_options {
bool inject_prop_defaults = true;
bool validate_list_types = true;
bool validate_cat_types = true;
};

// ==================== Handler System ====================

// Handler function type
using chunk_handler = std::function<void(
const chunk_header& header,
chunk_reader& reader,
byte_order order
)>;

// Handler registry with three-level precedence
class handler_registry {
public:
// Register handler for chunk within specific FORM type
void on_chunk_in_form(fourcc form_type, fourcc chunk_id, chunk_handler handler) {
form_handlers_[{form_type, chunk_id}] = std::move(handler);
}

    // Register handler for chunk within specific container type
    void on_chunk_in_container(fourcc container_type, fourcc chunk_id, chunk_handler handler) {
        container_handlers_[{container_type, chunk_id}] = std::move(handler);
    }
    
    // Register global handler for chunk ID
    void on_chunk(fourcc chunk_id, chunk_handler handler) {
        global_handlers_[chunk_id] = std::move(handler);
    }
    
    // Find handler with precedence: form > container > global
    const chunk_handler* find(
        const std::optional<fourcc>& current_form,
        const std::optional<fourcc>& current_container,
        fourcc chunk_id
    ) const {
        // Try form-specific handler first
        if (current_form) {
            auto it = form_handlers_.find({*current_form, chunk_id});
            if (it != form_handlers_.end()) {
                return &it->second;
            }
        }
        
        // Try container-specific handler
        if (current_container) {
            auto it = container_handlers_.find({*current_container, chunk_id});
            if (it != container_handlers_.end()) {
                return &it->second;
            }
        }
        
        // Try global handler
        auto it = global_handlers_.find(chunk_id);
        if (it != global_handlers_.end()) {
            return &it->second;
        }
        
        return nullptr;
    }
    
    // Builder pattern for fluent API
    class builder {
        handler_registry registry_;
        std::optional<fourcc> current_form_;
        std::optional<fourcc> current_container_;
        
    public:
        builder& form(fourcc type) {
            current_form_ = type;
            current_container_ = std::nullopt;
            return *this;
        }
        
        builder& container(fourcc type) {
            current_container_ = type;
            return *this;
        }
        
        builder& chunk(fourcc id, chunk_handler handler) {
            if (current_form_) {
                registry_.on_chunk_in_form(*current_form_, id, std::move(handler));
            } else if (current_container_) {
                registry_.on_chunk_in_container(*current_container_, id, std::move(handler));
            } else {
                registry_.on_chunk(id, std::move(handler));
            }
            return *this;
        }
        
        handler_registry build() {
            return std::move(registry_);
        }
    };
    
    static builder make() { return builder{}; }

private:
struct chunk_key {
fourcc scope;
fourcc id;

        bool operator==(const chunk_key& other) const {
            return scope == other.scope && id == other.id;
        }
    };
    
    struct chunk_key_hash {
        std::size_t operator()(const chunk_key& key) const noexcept {
            return std::hash<fourcc>{}(key.scope) ^ 
                   (std::hash<fourcc>{}(key.id) << 1);
        }
    };
    
    std::unordered_map<chunk_key, chunk_handler, chunk_key_hash> form_handlers_;
    std::unordered_map<chunk_key, chunk_handler, chunk_key_hash> container_handlers_;
    std::unordered_map<fourcc, chunk_handler, std::hash<fourcc>> global_handlers_;
};

// ==================== Selective Parsing ====================

// Plan for selective chunk parsing
struct parse_plan {
// Chunks to process: (optional scope, chunk_id)
std::vector<std::pair<std::optional<fourcc>, fourcc>> wanted_chunks;

    // Containers to descend into
    std::vector<fourcc> wanted_containers;
    
    // Maximum depth to descend
    int max_depth = 32;
    
    bool wants_container(fourcc type) const {
        if (wanted_containers.empty()) return true;
        return std::find(wanted_containers.begin(), 
                        wanted_containers.end(), type) != wanted_containers.end();
    }
    
    bool wants_chunk(const std::optional<fourcc>& scope, fourcc id) const {
        if (wanted_chunks.empty()) return true;
        
        for (const auto& [wanted_scope, wanted_id] : wanted_chunks) {
            bool scope_matches = !wanted_scope || 
                                (scope && *scope == *wanted_scope);
            if (scope_matches && wanted_id == id) {
                return true;
            }
        }
        return false;
    }
};

// ==================== RF64 Support ====================

struct rf64_state {
std::optional<std::uint64_t> riff_size;

    // Map from (chunk_id, absolute_offset) to size
    struct size_override {
        fourcc id;
        std::uint64_t offset;
        std::uint64_t size;
    };
    std::vector<size_override> overrides;
    
    std::optional<std::uint64_t> find_override(fourcc id, std::uint64_t offset) const {
        for (const auto& override : overrides) {
            if (override.id == id && override.offset == offset) {
                return override.size;
            }
        }
        return std::nullopt;
    }
};

// ==================== Parser Context ====================

struct parse_context {
std::optional<fourcc> form_type;      // Current FORM/RIFF type
std::optional<fourcc> container_type; // Immediate container type
};

// ==================== IFF-85 Specific ====================

// Property bank for PROP defaults
struct property_bank {
struct entry {
std::uint64_t offset;
std::uint64_t size;
};
std::unordered_map<fourcc, entry, std::hash<fourcc>> entries;
};

// Property view for accessing PROP defaults
class property_view {
reader* reader_;
const property_bank* bank_;

public:
property_view() : reader_(nullptr), bank_(nullptr) {}
    
    property_view(reader* r, const property_bank* bank)
        : reader_(r), bank_(bank) {}
    
    std::optional<chunk_reader> open(fourcc id) const {
        if (!reader_ || !bank_) return std::nullopt;
        
        auto it = bank_->entries.find(id);
        if (it == bank_->entries.end()) return std::nullopt;
        
        return chunk_reader(reader_, it->second.offset, it->second.size);
    }
    
    bool has(fourcc id) const {
        return bank_ && bank_->entries.contains(id);
    }
};

// FORM context for structured parsing
struct form_context {
fourcc form_type;
std::uint64_t payload_offset;
std::uint64_t payload_size;
std::size_t index_in_list;    // Position within LIST
property_view properties;      // PROP defaults
};

// Callbacks for structured parsing
struct structured_callbacks {
std::function<void(const form_context&)> on_form_begin;
std::function<void(const form_context&)> on_form_end;
};

// ==================== Main Parser Interface ====================

class chunk_parser {
public:
explicit chunk_parser(parse_options options = {})
: options_(std::move(options)) {}

    // Auto-detect format and parse
    parse_result<void> parse(
        reader& input,
        const handler_registry& handlers,
        const parse_plan* plan = nullptr
    );
    
    // Parse with explicit format
    parse_result<void> parse_iff(
        reader& input,
        const handler_registry& handlers,
        const parse_plan* plan = nullptr
    );
    
    parse_result<void> parse_riff(
        reader& input,
        const handler_registry& handlers,
        const parse_plan* plan = nullptr
    );
    
    // Structured IFF-85 parsing
    parse_result<void> parse_iff_structured(
        reader& input,
        const handler_registry& handlers,
        const structured_parse_options& options,
        const parse_plan* plan = nullptr,
        const structured_callbacks* callbacks = nullptr
    );

private:
parse_options options_;

    // Implementation details...
};

// ==================== Utility Functions ====================

// Format detection
enum class file_format { iff, riff, unknown };

inline file_format detect_format(reader& input) {
auto saved_pos = input.tell();
auto cleanup = [&] { input.seek(saved_pos); };

    std::array<std::byte, 4> magic;
    if (input.read(magic.data(), 4) != 4) {
        cleanup();
        return file_format::unknown;
    }
    
    cleanup();
    
    fourcc id = fourcc::from_bytes(magic.data(), std::endian::native);
    
    if (id == chunk_id::RIFF || id == chunk_id::RF64) {
        return file_format::riff;
    }
    
    if (id == chunk_id::FORM || id == chunk_id::LIST || 
        id == chunk_id::CAT || id == chunk_id::PROP || 
        id == chunk_id::RIFX) {
        return file_format::iff;
    }
    
    return file_format::unknown;
}

// Chunk visitor for traversal
class chunk_visitor {
public:
virtual ~chunk_visitor() = default;

    virtual void visit_chunk(
        const chunk_header& header,
        std::size_t depth
    ) = 0;
    
    virtual void enter_container(
        const chunk_header& header,
        std::size_t depth
    ) {}
    
    virtual void exit_container(
        const chunk_header& header,
        std::size_t depth
    ) {}
};

// Simple DOM builder for small files
struct chunk_node {
chunk_header header;
std::vector<chunk_node> children;
std::optional<std::vector<std::byte>> data;
};

parse_result<chunk_node> build_tree(
reader& input,
std::size_t inline_threshold = 1024,
std::size_t max_total_inline = 10 * 1024 * 1024
);

// ==================== Example Usage ====================

inline void example_usage() {
// Build handler registry
auto handlers = handler_registry::make()
.form("AIFF"_4cc)
.chunk("COMM"_4cc, [](const auto& h, auto& r, auto bo) {
auto channels = r.read_pod<std::uint16_t>(bo);
auto frames = r.read_pod<std::uint32_t>(bo);
// ...
})
.chunk("SSND"_4cc, [](const auto& h, auto& r, auto bo) {
// Handle sound data
})
.form("WAVE"_4cc)
.chunk("fmt "_4cc, [](const auto& h, auto& r, auto bo) {
auto format = r.read_pod<std::uint16_t>(bo);
// ...
})
.build();

    // Parse with plan
    parse_plan plan;
    plan.wanted_chunks = {
        {std::nullopt, "fmt "_4cc},
        {"AIFF"_4cc, "COMM"_4cc}
    };
    
    std::ifstream file("test.aiff", std::ios::binary);
    stream_reader reader(file);
    
    chunk_parser parser;
    auto result = parser.parse(reader, handlers, &plan);
}

} // namespace iff

// Specialization for std::hash
namespace std {
template<>
struct hash<iff::fourcc> {
std::size_t operator()(const iff::fourcc& f) const noexcept {
return std::hash<std::uint32_t>{}(f.value_);
}
};
}

// ==================== Implementation Notes ====================

/*
Implementation priorities:
1. Zero-copy streaming - no payload allocation unless explicitly requested
2. Proper RF64 support with ds64 chunk handling
3. Clean separation between raw and structured parsing
4. Type-safe handler registration with builder pattern
5. Memory-mapped file support for large files
6. Comprehensive error handling with expected<T, E>

Key design decisions:
- FourCC stores in native byte order for fast comparison
- Three-level handler precedence (form > container > global)
- Separate raw and structured walkers
- PROP defaults handled via two-pass in LIST
- Bounded chunk_reader prevents overreads
- Parse plan for selective processing
  */