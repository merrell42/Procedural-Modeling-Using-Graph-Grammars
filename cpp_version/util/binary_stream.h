#pragma once
#include <istream>
#include <ostream>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "../geometry/vec3.h"

// Primitive write
template<typename T>
inline void bsWrite(std::ostream& o, T v) {
    o.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

// Primitive read - throws on truncation
template<typename T>
inline T bsRead(std::istream& i) {
    T v{};
    i.read(reinterpret_cast<char*>(&v), sizeof(T));
    if (!i) throw std::runtime_error("binary cache read failed");
    return v;
}

inline void bsWriteStr(std::ostream& o, const std::string& s) {
    bsWrite<int32_t>(o, static_cast<int32_t>(s.size()));
    o.write(s.data(), static_cast<std::streamsize>(s.size()));
}

inline std::string bsReadStr(std::istream& i) {
    int32_t n = bsRead<int32_t>(i);
    if (n < 0 || n > (1 << 20)) throw std::runtime_error("bad string length in cache");
    std::string s(static_cast<size_t>(n), '\0');
    if (n > 0) i.read(s.data(), n);
    return s;
}

inline void bsWriteVec3(std::ostream& o, const Vec3& v) {
    bsWrite<double>(o, v.getX());
    bsWrite<double>(o, v.getY());
    bsWrite<double>(o, v.getZ());
}

inline Vec3 bsReadVec3(std::istream& i) {
    double x = bsRead<double>(i);
    double y = bsRead<double>(i);
    double z = bsRead<double>(i);
    return Vec3(x, y, z);
}
