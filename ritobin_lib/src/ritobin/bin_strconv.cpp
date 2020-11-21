#include "bin_strconv.hpp"
#include "bin_numconv.hpp"
#include <optional>
#include <bit>
#include <span>

namespace ritobin::strconv_impl {
    // TODO: handle unicode shit properly

    using escape_pair = std::pair<std::string_view, std::string_view>;

    static void write_utf8(std::string& out, uint32_t value) noexcept {
        if (value < 0x80) {
            out += { (char)value };
        } else if (value < 0x800) {
            out += {
                (char)(((value >> 6) & 0b000011111) | 0b11000000),
                (char)(((value)&0b00111111) | 0b10000000),
            };
        } else if (value < 0x10000) {
            out += {
                (char)(((value >> 12) & 0b000001111) | 0b11100000),
                (char)(((value >> 6) & 0b00111111) | 0b10000000),
                (char)(((value)&0b00111111) | 0b10000000),
            };
        } else if (value < 0x10FFFF) {
            out += {
                (char)(((value >> 18) & 0b00000111) | 0b11110000),
                (char)(((value >> 12) & 0b00111111) | 0b10000000),
                (char)(((value >> 6) & 0b00111111) | 0b10000000),
                (char)(((value)&0b00111111) | 0b10000000),
            };
        } else {
            out += {(char)0xEF, (char)0xBF, (char)0xBD};
        }
    }

    struct StringIterator {
        std::string_view data_;

        char const* data() const noexcept {
            return data_.data();
        }

        size_t left() const noexcept {
            return data_.size();
        }

        std::optional<char> peek() noexcept {
            if (left()) {
                return data_.front();
            } else {
                return std::nullopt;
            }
        }

        std::optional<char> pop() noexcept {
            if (left()) {
                auto c = data_.front();
                data_.remove_prefix(1);
                return c;
            } else {
                return std::nullopt;
            }
        }

        std::optional<std::string_view> match_str(std::string_view what) noexcept {
            if (data_.starts_with(what)) {
                data_.remove_prefix(what.size());
                return what;
            } else {
                return std::nullopt;
            }
        }

        std::optional<std::string_view> match_escape(std::span<escape_pair const> escapes) {
            for (auto [from, to]: escapes) {
                if (match_str(from)) {
                    return to;
                }
            }
            return std::nullopt;
        }

        std::optional<uint32_t> match_hex(size_t size) noexcept {
            if (data_.size() < size) {
                return false;
            }
            uint32_t result = 0;
            auto str = data_.substr(0, size);
            if (to_num(str, result, 16)) {
                data_.remove_prefix(size);
                return result;
            } else {
                return std::nullopt;
            }
        }
    };

    struct StringUnquote {
        StringIterator iter;
        void process(std::string& out) noexcept {
            while (iter.left()) {
                if (!read_unicode_unescape(out)) {
                    break;
                }
                if (!read_simple_unescape(out)) {
                    break;
                }
            }
        }
    private:
        // reads utf16 escape, returns false on error
        // if you wonder why this looks so "complex":
        // https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF
        bool read_unicode_unescape(std::string& out) noexcept {
            uint32_t last = 0;
            while (iter.match_str("\\u")) {
                if (auto value = iter.match_hex(4)) {
                    if (*value >= 0xDC00u && value <= 0xDFFFu) {
                        if (last) {
                            write_utf8(out, (((last - 0xD800u) << 10) | (*value - 0xDC00u)) + 0x10000u);
                            last = 0;
                        } else {
                            write_utf8(out, *value);
                        }
                    } else {
                        if (last) {
                            write_utf8(out, last);
                            last = 0;
                        }
                        if (*value >= 0xD800u && *value <= 0xDBFFu) {
                            last = *value;
                        } else {
                            write_utf8(out, *value);
                        }
                    }
                } else {
                    return false;
                }
            }
            if (last) {
                write_utf8(out, last);
            }
            return true;
        }

        // read simple escapes, returns false on error
        bool read_simple_unescape(std::string& out) noexcept {
            constexpr escape_pair const escapes[] = {
                { "\\'", "\'" },
                { "\\\"", "\"" },
                { "\\\\", "\\" },
                { "\\a", "\a" },
                { "\\b", "\b" },
                { "\\f", "\f" },
                { "\\n", "\n" },
                { "\\r", "\r" },
                { "\\t", "\t" },
                { "\\\n", "\n" },
                { "\\\r\n", "\n" },
                { "\\\r", "\r" },
            };
            if (auto to = iter.match_escape(escapes)) {
                out += *to;
                return true;
            } else if (iter.match_str("\\x")) {
                if (auto hex = iter.match_hex(2)) {
                    write_utf8(out, *hex);
                    return true;
                } else {
                    return false;
                }
            } else if (iter.match_str("\\")) {
                return false;
            } else if (auto c = iter.pop()) {
                if ((uint8_t)*c < 0x20) {
                    return false;
                } else {
                    out += *c;
                    return true;
                }
            } else {
                return true;
            }
        }
    };

    struct StringQuote {
        StringIterator iter;

        void process(std::vector<char>& out) noexcept {
            out.push_back('"');
            while(iter.left()) {
                if (!write_simple_escape(out)) {
                    break;
                }
            }
            out.push_back('"');
        }
    private:
        // writes simple escape, returns false on error
        bool write_simple_escape(std::vector<char>& out) noexcept {
            constexpr escape_pair const escapes[] = {
                { "\t", "\\t" },
                { "\n", "\\n" },
                { "\r", "\\r" },
                { "\b", "\\b" },
                { "\f", "\\f" },
                { "\\", "\\\\" },
                { "\"", "\\\"" },
            };
            if (auto value = iter.match_escape(escapes)) {
                out.insert(out.end(), value->begin(), value->end());
                return true;
            } else if (auto c = iter.pop()) {
                if (c < (uint8_t)0x20) {
                    constexpr char digits[] = "0123456789abcdef";
                    char hex[] = {
                        '\\', 'x',
                        digits[(*c >> 4) & 0x0F],
                        digits[*c & 0x0F],
                    };
                    out.insert(out.end(), hex, hex + sizeof(hex));
                    return true;
                } else {
                    // TODO: unicode verification...
                    out.push_back(*c);
                    return true;
                }
            } else {
                return true;
            }
        }
    };
}

namespace ritobin {
    using namespace strconv_impl;

    char const* str_unquote_fetch_end(std::string_view data) noexcept {
        auto iter = StringIterator { data };
        for (auto quote = iter.pop(); iter.left() && iter.peek() != quote; iter.pop()) {
            iter.match_str("\\");
        }
        return iter.data();
    }

    char const* str_unquote(std::string_view data, std::string& out) noexcept {
        auto unquote = StringUnquote { { data } };
        unquote.process(out);
        return unquote.iter.data();
    }

    char const* str_quote(std::string_view data, std::vector<char>& out) noexcept {
        auto quote = StringQuote { { data } };
        quote.process(out);
        return quote.iter.data();
    }
}
