/**
  \file G3D-base.lib/include/G3D-base/Vector2int32.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_Vector2int32_h
#define G3D_Vector2int32_h

#include "G3D-base/platform.h"
#include "G3D-base/G3DString.h"
#include "G3D-base/g3dmath.h"
#include "G3D-base/format.h"
#include "G3D-base/HashTrait.h"

namespace G3D {

/**
 \class Vector2int32 
 A Vector2 that packs its fields into int32s.
 */
G3D_BEGIN_PACKED_CLASS(2)
Vector2int32 {
private:
    // Hidden operators
    bool operator<(const Vector2int32&) const;
    bool operator>(const Vector2int32&) const;
    bool operator<=(const Vector2int32&) const;
    bool operator>=(const Vector2int32&) const;

public:
    G3D::int32              x;
    G3D::int32              y;

    Vector2int32() : x(0), y(0) {}
    Vector2int32(G3D::int32 _x, G3D::int32 _y) : x(_x), y(_y){}
    explicit Vector2int32(const class Vector2& v);
    explicit Vector2int32(class BinaryInput& bi);
    Vector2int32(const class Vector2int16& v);
    Vector2int32(const class Vector2uint16& v);
    Vector2int32(const class Any& a);

    inline G3D::int32& operator[] (int i) {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::int32*)this)[i];
    }

    inline const G3D::int32& operator[] (int i) const {
        debugAssert(((unsigned int)i) <= 1);
        return ((G3D::int32*)this)[i];
    }

    inline Vector2int32 operator+(const Vector2int32& other) const {
        return Vector2int32(x + other.x, y + other.y);
    }

    inline Vector2int32 operator-(const Vector2int32& other) const {
        return Vector2int32(x - other.x, y - other.y);
    }

    inline Vector2int32 operator-() const {
        return Vector2int32(-x, -y);
    }

    inline Vector2int32 operator*(const Vector2int32& other) const {
        return Vector2int32(x * other.x, y * other.y);
    }

    inline Vector2int32 operator*(const int s) const {
        return Vector2int32(x * s, y * s);
    }

    inline Vector2int32& operator+=(const Vector2int32& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** Shifts both x and y */
    inline Vector2int32 operator>>(const int s) const {
        return Vector2int32(x >> s, y >> s);
    }

    /** Shifts both x and y */
    inline Vector2int32 operator<<(const int s) const {
        return Vector2int32(x << s, y << s);
    }

    inline Vector2int32& operator-=(const Vector2int32& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline Vector2int32& operator*=(const Vector2int32& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vector2int32 clamp(const Vector2int32& lo, const Vector2int32& hi);

    inline bool operator==(const Vector2int32& other) const {
        return (x == other.x) && (y == other.y);
    }

    inline bool operator!= (const Vector2int32& other) const {
        return !(*this == other);
    }

    Vector2int32 max(const Vector2int32& v) const {
        return Vector2int32(G3D::max(x, v.x), G3D::max(y, v.y));
    }

    Vector2int32 min(const Vector2int32& v) const {
        return Vector2int32(G3D::min(x, v.x), G3D::min(y, v.y));
    }

    String toString() const;

    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);

    /** Parses a string of the form "number x number" */
    static Vector2int32 parseResolution(const String& s);
}
G3D_END_PACKED_CLASS(2)

typedef Vector2int32 Point2int32;

} // namespace G3D

template<> struct HashTrait<G3D::Vector2int32> {
    static size_t hashCode(const G3D::Vector2int32& key) { return static_cast<size_t>(key.x ^ ((int)key.y << 1)); }
};

#endif
