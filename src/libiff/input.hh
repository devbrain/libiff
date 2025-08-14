//
// Created by igor on 12/08/2025.
//

#pragma once

#include <iosfwd>
#include <cstdint>
#include <array>
#include <memory>
#include <vector>
#include <cstring>

#include <iff/exceptions.hh>
#include <iff/byte_order.hh>
#include <iff/fourcc.hh>

namespace iff {
    class subreader;

    // Base reader interface
    class reader_base {
        public:
            enum whence_t {
                set,
                cur,
                end
            };

        public:
            virtual ~reader_base() = default;

            // Simple interface - throws on error
            virtual std::size_t read(void* dst, std::size_t size) = 0;
            virtual void seek(std::uint64_t offset, whence_t whence) = 0;
            virtual std::uint64_t tell() const = 0;
            virtual std::uint64_t size() const = 0;
            
            // Get underlying stream (may throw if not available)
            virtual std::istream& get_stream() = 0;

            // Create a subreader from current position
            [[nodiscard]] std::unique_ptr <subreader> create_subreader(std::size_t size);

            // Convenience methods
            std::vector<std::byte> read_exact(std::size_t size) {
                std::vector<std::byte> buffer(size);
                std::size_t actual = read(buffer.data(), size);
                THROW_IO_IF(actual != size, "Unexpected EOF: requested", size, "got", actual);
                return buffer;
            }

            template<typename T>
            T read(byte_order bo) {
                std::array<std::byte, sizeof(T)> buff;
                std::size_t actual = read(buff.data(), sizeof(T));
                THROW_IO_IF(actual != sizeof(T), "Failed to read", sizeof(T), "bytes");
                
                T value;
                std::memcpy(&value, buff.data(), sizeof(T));
                if constexpr (sizeof(T) > 1) {
                    if (!byte_order_native(bo)) {
                        value = swap_byte_order(value);
                    }
                }
                return value;
            }

            fourcc read_fourcc();
    };

    // Main reader class - reads from stream without limits
    class reader : public reader_base {
        public:
            explicit reader(std::istream& is);
            ~reader() override = default;

            std::size_t read(void* dst, std::size_t size) override;
            void seek(std::uint64_t offset, whence_t whence) override;
            std::uint64_t tell() const override;
            std::uint64_t size() const override;

            std::istream& get_stream() override { return m_stream; }

        private:
            std::istream& m_stream;
    };

    // Subreader class - reads from a limited region
    class subreader : public reader_base {
        public:
            subreader(reader_base* parent, std::uint64_t start, std::size_t size);
            ~subreader() override;

            subreader(const subreader&) = delete;
            subreader& operator = (const subreader&) = delete;

            subreader(subreader&&) = default;
            subreader& operator = (subreader&&) = default;

            std::size_t read(void* dst, std::size_t size) override;
            void seek(std::uint64_t offset, whence_t whence) override;
            std::uint64_t tell() const override;
            std::uint64_t size() const override;
            std::istream& get_stream() override;

            [[nodiscard]] std::size_t remaining() const;
            [[nodiscard]] std::uint64_t start_offset() const { return m_start; }

        private:
            reader_base* m_parent;
            std::uint64_t m_start; // Absolute start position in parent
            std::size_t m_size; // Size of this region
            std::uint64_t m_position; // Current position relative to m_start
    };
}
