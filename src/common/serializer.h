#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <cassert>

namespace serializer {

inline void write_uint8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

inline void write_int32_be(std::vector<uint8_t>& out, int32_t v) {
    uint32_t u = static_cast<uint32_t>(v);
    out.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((u >> 0) & 0xFF));
}

inline void write_uint64_be(std::vector<uint8_t>& out, uint64_t v) {
    out.push_back(static_cast<uint8_t>((v >> 56) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 48) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 40) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 32) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 0) & 0xFF));
}

inline void write_double_be(std::vector<uint8_t>& out, double d) {
    static_assert(sizeof(double) == sizeof(uint64_t), "double must be 8 bytes");
    uint64_t v;
    std::memcpy(&v, &d, sizeof(v)); // copy bit pattern
    write_uint64_be(out, v);
}

// read helpers from raw buffer (big-endian)
inline int32_t read_int32_be(const uint8_t* buf) {
    uint32_t u = (static_cast<uint32_t>(buf[0]) << 24) |
                 (static_cast<uint32_t>(buf[1]) << 16) |
                 (static_cast<uint32_t>(buf[2]) << 8) |
                 (static_cast<uint32_t>(buf[3]) << 0);
    return static_cast<int32_t>(u);
}

inline uint64_t read_uint64_be(const uint8_t* buf) {
    uint64_t v = (static_cast<uint64_t>(buf[0]) << 56) |
                 (static_cast<uint64_t>(buf[1]) << 48) |
                 (static_cast<uint64_t>(buf[2]) << 40) |
                 (static_cast<uint64_t>(buf[3]) << 32) |
                 (static_cast<uint64_t>(buf[4]) << 24) |
                 (static_cast<uint64_t>(buf[5]) << 16) |
                 (static_cast<uint64_t>(buf[6]) << 8) |
                 (static_cast<uint64_t>(buf[7]) << 0);
    return v;
}

inline double read_double_be(const uint8_t* buf) {
    uint64_t v = read_uint64_be(buf);
    double d;
    std::memcpy(&d, &v, sizeof(d));
    return d;
}

} 
