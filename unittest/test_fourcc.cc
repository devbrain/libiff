#include <doctest/doctest.h>
#include <iff/fourcc.hh>

#include <sstream>
#include <unordered_map>
#include <set>
#include <cstring>

using namespace iff;

// Common fourcc identifiers
namespace chunk_id {
    // IFF container types
    inline constexpr fourcc FORM('F', 'O', 'R', 'M');
    inline constexpr fourcc LIST('L', 'I', 'S', 'T');
    inline constexpr fourcc CAT_('C', 'A', 'T', ' ');
    inline constexpr fourcc PROP('P', 'R', 'O', 'P');

    // RIFF container types
    inline constexpr fourcc RIFF('R', 'I', 'F', 'F');
    inline constexpr fourcc RIFX('R', 'I', 'F', 'X');
    inline constexpr fourcc RF64('R', 'F', '6', '4');

    // Format types
    inline constexpr fourcc WAVE('W', 'A', 'V', 'E');
    inline constexpr fourcc AVI_('A', 'V', 'I', ' ');
    inline constexpr fourcc AIFF('A', 'I', 'F', 'F');
    inline constexpr fourcc ILBM('I', 'L', 'B', 'M');

    // Common chunks
    inline constexpr fourcc fmt_('f', 'm', 't', ' ');
    inline constexpr fourcc data('d', 'a', 't', 'a');
    inline constexpr fourcc JUNK('J', 'U', 'N', 'K');
}

// Utility functions for fourcc
namespace fourcc_utils {
    inline bool is_container(const fourcc& f) {
        using namespace chunk_id;
        return f == FORM || f == LIST || f == CAT_ || f == PROP ||
               f == RIFF || f == RIFX || f == RF64;
    }

    inline bool is_riff_family(const fourcc& f) {
        using namespace chunk_id;
        return f == RIFF || f == RIFX || f == RF64;
    }

    inline bool is_iff_family(const fourcc& f) {
        using namespace chunk_id;
        return f == FORM || f == LIST || f == CAT_ || f == PROP;
    }

    // Endian conversion functions (fourcc is byte-order independent)
    inline fourcc from_be_bytes(const void* data) {
        return fourcc::from_bytes(data);
    }

    inline fourcc from_le_bytes(const void* data) {
        return fourcc::from_bytes(data);
    }

    inline void to_be_bytes(const fourcc& f, void* dest) {
        f.to_bytes(dest);
    }

    inline void to_le_bytes(const fourcc& f, void* dest) {
        f.to_bytes(dest);
    }
}

TEST_SUITE("FOURCC") {
    TEST_CASE("fourcc construction") {
        SUBCASE("default construction") {
            fourcc f;
            CHECK(f.to_string() == "    ");
            CHECK(f[0] == ' ');
            CHECK(f[1] == ' ');
            CHECK(f[2] == ' ');
            CHECK(f[3] == ' ');
        }

        SUBCASE("string literal construction") {
            fourcc form("FORM");
            CHECK(form.to_string() == "FORM");
            CHECK(form[0] == 'F');
            CHECK(form[1] == 'O');
            CHECK(form[2] == 'R');
            CHECK(form[3] == 'M');

            // String construction is runtime-only
            fourcc test("TEST");
            CHECK(test[0] == 'T');
            CHECK(test == fourcc('T', 'E', 'S', 'T'));
        }

        SUBCASE("individual char construction") {
            fourcc test('T', 'E', 'S', 'T');
            CHECK(test.to_string() == "TEST");

            fourcc mixed('A', ' ', 'B', ' ');
            CHECK(mixed.to_string() == "A B ");
        }

        SUBCASE("string_view construction with padding") {
            fourcc abc(std::string_view("ABC"));
            CHECK(abc.to_string() == "ABC ");
            CHECK(abc[3] == ' ');

            fourcc ab(std::string_view("AB"));
            CHECK(ab.to_string() == "AB  ");

            fourcc a(std::string_view("A"));
            CHECK(a.to_string() == "A   ");

            fourcc empty(std::string_view(""));
            CHECK(empty.to_string() == "    ");

            // Truncation
            fourcc toolong(std::string_view("TOOLONG"));
            CHECK(toolong.to_string() == "TOOL");
        }

        SUBCASE("C-string construction") {
            fourcc wave("WAVE");
            CHECK(wave.to_string() == "WAVE");

            fourcc fmt("fmt");
            CHECK(fmt.to_string() == "fmt ");

            const char* str = "DATA";
            fourcc data(str);
            CHECK(data.to_string() == "DATA");
        }

        SUBCASE("std::string construction") {
            std::string riff = "RIFF";
            fourcc f(riff);
            CHECK(f.to_string() == "RIFF");

            std::string xy = "XY";
            fourcc f2(xy);
            CHECK(f2.to_string() == "XY  ");
        }

        SUBCASE("from_bytes construction") {
            unsigned char bytes[4] = {'T', 'E', 'S', 'T'};
            fourcc test = fourcc::from_bytes(bytes);
            CHECK(test.to_string() == "TEST");

            // Non-printable bytes
            unsigned char binary[4] = {0x01, 0x02, 0x03, 0x04};
            fourcc bin = fourcc::from_bytes(binary);
            CHECK(bin[0] == 0x01);
            CHECK(bin[1] == 0x02);
            CHECK(bin[2] == 0x03);
            CHECK(bin[3] == 0x04);
        }

        SUBCASE("uint32_t construction") {
            // Note: result depends on system endianness
            std::uint32_t value = 0x54455354; // "TEST" in big-endian
            fourcc f(value);

            // Check that we can round-trip
            CHECK(f.to_uint32() == value);
        }
    }

    TEST_CASE("fourcc user-defined literals") {
        SUBCASE("4cc literal") {
            auto wave = "WAVE"_4cc;
            CHECK(wave.to_string() == "WAVE");

            auto fmt = "fmt"_4cc;
            CHECK(fmt.to_string() == "fmt ");

            auto ab = "AB"_4cc;
            CHECK(ab.to_string() == "AB  ");

            auto a = "A"_4cc;
            CHECK(a.to_string() == "A   ");

            auto empty = ""_4cc;
            CHECK(empty.to_string() == "    ");

            // Compile-time usage
            constexpr auto form = "FORM"_4cc;
            CHECK(form == fourcc("FORM"));
        }

        SUBCASE("FOURCC macro") {
            auto test = FOURCC("TEST");
            CHECK(test.to_string() == "TEST");

            auto xyz = FOURCC("XYZ");
            CHECK(xyz.to_string() == "XYZ ");

            // This should fail to compile:
            // auto bad = FOURCC("TOOLONG");
        }
    }

    TEST_CASE("fourcc conversions") {
        SUBCASE("to_string") {
            CHECK(fourcc("WAVE").to_string() == "WAVE");
            CHECK(fourcc("AB").to_string() == "AB  ");
            CHECK(fourcc().to_string() == "    ");
        }

        SUBCASE("to_string_view") {
            fourcc test("TEST");
            std::string_view sv = test.to_string_view();
            CHECK(sv == "TEST");
            CHECK(sv.size() == 4);
        }

        SUBCASE("to_string_trimmed") {
            CHECK(fourcc("WAVE").to_string_trimmed() == "WAVE");
            CHECK(fourcc("ABC").to_string_trimmed() == "ABC");
            CHECK(fourcc("AB").to_string_trimmed() == "AB");
            CHECK(fourcc("A").to_string_trimmed() == "A");
            CHECK(fourcc("").to_string_trimmed() == "");
            CHECK(fourcc("  X ").to_string_trimmed() == "  X");
        }

        SUBCASE("to_uint32 and from uint32") {
            fourcc test{"TEST"};
            std::uint32_t value = test.to_uint32();
            fourcc test2(value);
            CHECK(test == test2);
        }

        SUBCASE("to_bytes") {
            fourcc wave{"WAVE"};
            unsigned char bytes[4];
            wave.to_bytes(bytes);
            CHECK(bytes[0] == 'W');
            CHECK(bytes[1] == 'A');
            CHECK(bytes[2] == 'V');
            CHECK(bytes[3] == 'E');
        }
    }

    TEST_CASE("fourcc element access") {
        SUBCASE("operator[]") {
            fourcc test{"TEST"};
            CHECK(test[0] == 'T');
            CHECK(test[1] == 'E');
            CHECK(test[2] == 'S');
            CHECK(test[3] == 'T');

            // Mutable access
            fourcc mutable_test{"TEST"};
            mutable_test[0] = 'B';
            CHECK(mutable_test.to_string() == "BEST");
        }

        SUBCASE("iterators") {
            fourcc wave{"WAVE"};
            std::string result;
            for (char c : wave) {
                result += c;
            }
            CHECK(result == "WAVE");

            // Algorithms
            fourcc test{"TEST"};
            CHECK(std::all_of(test.begin(), test.end(), [](char c) {
                return c >= 'A' && c <= 'Z';
                }));

            // Reverse
            fourcc dcba{"ABCD"};
            std::reverse(dcba.begin(), dcba.end());
            CHECK(dcba.to_string() == "DCBA");
        }
    }

    TEST_CASE("fourcc comparison operators") {
        SUBCASE("equality") {
            CHECK(fourcc("TEST") == fourcc("TEST"));
            CHECK(fourcc("ABC") == fourcc("ABC "));
            CHECK(fourcc("AB") == fourcc("AB  "));
            CHECK(fourcc("") == fourcc("    "));

            CHECK(fourcc("TEST") != fourcc("BEST"));
            CHECK(fourcc("ABC") != fourcc("ABCD"));
        }

        SUBCASE("ordering") {
            CHECK(fourcc("AAAA") < fourcc("BBBB"));
            CHECK(fourcc("AAAA") <= fourcc("AAAA"));
            CHECK(fourcc("BBBB") > fourcc("AAAA"));
            CHECK(fourcc("BBBB") >= fourcc("BBBB"));

            // Lexicographic ordering
            CHECK(fourcc("ABC ") < fourcc("ABCD"));
            CHECK(fourcc("WAVE") > fourcc("FORM"));
        }

        SUBCASE("use in sorted containers") {
            std::set <fourcc> chunks;
            chunks.insert(fourcc("WAVE"));
            chunks.insert(fourcc("FORM"));
            chunks.insert(fourcc("RIFF"));
            chunks.insert(fourcc("fmt "));

            CHECK(chunks.size() == 4);
            CHECK(chunks.begin()->to_string() == "FORM");
            CHECK(std::prev(chunks.end())->to_string() == "fmt ");
        }
    }

    TEST_CASE("fourcc utility methods") {
        SUBCASE("is_printable") {
            CHECK(fourcc("TEST").is_printable());
            CHECK(fourcc("fmt ").is_printable());
            CHECK(fourcc("~!@#").is_printable());

            // Non-printable
            fourcc binary;
            binary[0] = 0x01;
            binary[1] = 0x02;
            binary[2] = 0x03;
            binary[3] = 0x04;
            CHECK_FALSE(binary.is_printable());

            fourcc mixed("AB\x01\x02");
            CHECK_FALSE(mixed.is_printable());
        }

        SUBCASE("has_padding") {
            CHECK_FALSE(fourcc("TEST").has_padding());
            CHECK(fourcc("ABC").has_padding());
            CHECK(fourcc("AB").has_padding());
            CHECK(fourcc("A").has_padding());
            CHECK(fourcc("").has_padding());
            CHECK(fourcc(" XYZ").has_padding());
            CHECK(fourcc("X Y ").has_padding());
        }
    }

    TEST_CASE("fourcc stream output") {
        SUBCASE("default format") {
            std::stringstream ss;
            ss << fourcc("WAVE");
            CHECK(ss.str() == "'WAVE'");

            ss.str("");
            ss << fourcc("fmt ");
            CHECK(ss.str() == "'fmt '");
        }

        SUBCASE("hex format") {
            std::stringstream ss;
            ss << std::hex << fourcc("WAVE");

            // The exact hex value depends on system endianness
            std::string hex_str = ss.str();
            CHECK(hex_str.substr(0, 2) == "0x");
            CHECK(hex_str.length() == 10); // "0x" + 8 hex digits

            // Round trip test
            std::uint32_t value = std::stoul(hex_str.substr(2), nullptr, 16);
            fourcc reconstructed(value);
            CHECK(reconstructed == fourcc("WAVE"));
        }

        SUBCASE("non-printable characters") {
            fourcc binary;
            binary[0] = 'A';
            binary[1] = 'B';
            binary[2] = '\x01';
            binary[3] = '\n';

            std::stringstream ss;
            ss << binary;
            CHECK(ss.str() == "'AB\\x01\\x0a'");
        }

        SUBCASE("preserving stream state") {
            std::stringstream ss;
            ss << std::hex << std::uppercase;
            auto flags = ss.flags();

            ss << fourcc("TEST");

            // Flags should be preserved
            CHECK(ss.flags() == flags);
        }
    }

    TEST_CASE("fourcc hashing") {
        SUBCASE("fourcc_hash") {
            fourcc_hash hasher;

            // Same fourcc should have same hash
            CHECK(hasher(fourcc("WAVE")) == hasher(fourcc("WAVE")));
            CHECK(hasher(fourcc("ABC")) == hasher(fourcc("ABC ")));

            // Different fourcc should (usually) have different hash
            CHECK(hasher(fourcc("WAVE")) != hasher(fourcc("FORM")));
        }

        SUBCASE("std::hash specialization") {
            std::hash <fourcc> hasher;

            CHECK(hasher(fourcc("TEST")) == hasher(fourcc("TEST")));
            CHECK(hasher(fourcc("XY")) == hasher(fourcc("XY  ")));
        }

        SUBCASE("use in unordered containers") {
            std::unordered_map <fourcc, int> chunk_sizes;

            chunk_sizes[fourcc("WAVE")] = 100;
            chunk_sizes[fourcc("fmt ")] = 16;
            chunk_sizes[fourcc("data")] = 44100;

            CHECK(chunk_sizes.size() == 3);
            CHECK(chunk_sizes[fourcc("WAVE")] == 100);
            CHECK(chunk_sizes[fourcc("fmt ")] == 16);

            // Check that padded versions work
            chunk_sizes[fourcc("ABC")] = 50;
            CHECK(chunk_sizes[fourcc("ABC ")] == 50);
        }
    }

    TEST_CASE("fourcc common constants") {
        using namespace chunk_id;

        SUBCASE("IFF constants") {
            CHECK(FORM.to_string() == "FORM");
            CHECK(LIST.to_string() == "LIST");
            CHECK(CAT_.to_string() == "CAT ");
            CHECK(PROP.to_string() == "PROP");
        }

        SUBCASE("RIFF constants") {
            CHECK(RIFF.to_string() == "RIFF");
            CHECK(RIFX.to_string() == "RIFX");
            CHECK(RF64.to_string() == "RF64");
        }

        SUBCASE("format types") {
            CHECK(WAVE.to_string() == "WAVE");
            CHECK(AVI_.to_string() == "AVI ");
            CHECK(AIFF.to_string() == "AIFF");
            CHECK(ILBM.to_string() == "ILBM");
        }

        SUBCASE("common chunks") {
            CHECK(fmt_.to_string() == "fmt ");
            CHECK(data.to_string() == "data");
            CHECK(JUNK.to_string() == "JUNK");
        }
    }

    TEST_CASE("fourcc_utils") {
        using namespace fourcc_utils;
        using namespace chunk_id;

        SUBCASE("is_container") {
            CHECK(is_container(FORM));
            CHECK(is_container(LIST));
            CHECK(is_container(CAT_));
            CHECK(is_container(PROP));
            CHECK(is_container(RIFF));
            CHECK(is_container(RIFX));
            CHECK(is_container(RF64));

            CHECK_FALSE(is_container(WAVE));
            CHECK_FALSE(is_container(fmt_));
            CHECK_FALSE(is_container(data));
        }

        SUBCASE("is_riff_family") {
            CHECK(is_riff_family(RIFF));
            CHECK(is_riff_family(RIFX));
            CHECK(is_riff_family(RF64));

            CHECK_FALSE(is_riff_family(FORM));
            CHECK_FALSE(is_riff_family(LIST));
            CHECK_FALSE(is_riff_family(WAVE));
        }

        SUBCASE("is_iff_family") {
            CHECK(is_iff_family(FORM));
            CHECK(is_iff_family(LIST));
            CHECK(is_iff_family(CAT_));
            CHECK(is_iff_family(PROP));

            CHECK_FALSE(is_iff_family(RIFF));
            CHECK_FALSE(is_iff_family(RIFX));
            CHECK_FALSE(is_iff_family(WAVE));
        }

        SUBCASE("endian conversions") {
            unsigned char bytes[4] = {'T', 'E', 'S', 'T'};

            // from_be_bytes and from_le_bytes should produce same result
            // since FourCC doesn't swap bytes
            fourcc be = from_be_bytes(bytes);
            fourcc le = from_le_bytes(bytes);
            CHECK(be == le);
            CHECK(be.to_string() == "TEST");

            // Round trip
            unsigned char out_be[4];
            unsigned char out_le[4];
            to_be_bytes(be, out_be);
            to_le_bytes(le, out_le);

            CHECK(std::memcmp(bytes, out_be, 4) == 0);
            CHECK(std::memcmp(bytes, out_le, 4) == 0);
        }
    }

    TEST_CASE("fourcc edge cases") {
        SUBCASE("null characters") {
            // Note: String literals with embedded nulls stop at the null when
            // passed as const char*. Use individual chars for nulls.
            fourcc null_test('A', 'B', '\0', 'D');
            CHECK(null_test.to_string() == std::string("AB\0D", 4));
            CHECK(null_test[2] == '\0');
            CHECK(null_test[3] == 'D');

            // String constructor stops at null terminator
            fourcc null_from_string("AB\0D");
            CHECK(null_from_string.to_string() == "AB  "); // Padded with spaces
            CHECK(null_from_string[2] == ' ');

            // to_string_trimmed should handle nulls
            fourcc null_padded('A', 'B', '\0', ' ');
            CHECK(null_padded.to_string_trimmed() == std::string("AB\0", 3));
        }

        SUBCASE("special characters") {
            fourcc special("!@#$");
            CHECK(special.to_string() == "!@#$");
            CHECK(special.is_printable());

            fourcc tab("A\tB\n");
            CHECK(tab[1] == '\t');
            CHECK(tab[3] == '\n');
            CHECK_FALSE(tab.is_printable());
        }

        SUBCASE("all same character") {
            fourcc aaaa("AAAA");
            CHECK(std::all_of(aaaa.begin(), aaaa.end(), [](char c) {
                return c == 'A';
                }));

            fourcc spaces("    ");
            CHECK(spaces.has_padding());
            CHECK(spaces.to_string_trimmed() == "");
        }

        SUBCASE("constexpr operations") {
            constexpr fourcc test('T', 'E', 'S', 'T');
            constexpr char first = test[0];

            // Note: comparison operators are not constexpr in C++17
            // due to std::array limitations
            bool equal = test == fourcc('T', 'E', 'S', 'T');
            bool not_equal = test != fourcc('B', 'E', 'S', 'T');

            static_assert(first == 'T');
            CHECK(equal);
            CHECK(not_equal);
        }
    }
}
