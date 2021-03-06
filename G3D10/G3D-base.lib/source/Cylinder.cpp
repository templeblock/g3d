/**
  \file G3D-base.lib/source/Cylinder.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/platform.h"
#include "G3D-base/Cylinder.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/LineSegment.h"
#include "G3D-base/CoordinateFrame.h"
#include "G3D-base/Line.h"
#include "G3D-base/AABox.h"
#include "G3D-base/Any.h"

namespace G3D {

Cylinder::Cylinder(class BinaryInput& b) {
    deserialize(b);
}


Cylinder::Cylinder() {
}


Cylinder::Cylinder(const Vector3& _p1, const Vector3& _p2, float _r) 
    : p1(_p1), p2(_p2), mRadius(_r) {
}

Cylinder::Cylinder(const Any & a) {
    AnyTableReader r(a);
    r.getIfPresent("radius", mRadius);
    r.getIfPresent("p1", p1);
    r.getIfPresent("p2", p2);
    r.verifyDone();
}

Any Cylinder::toAny() const {
    Any a;
    a["radius"] = mRadius;
    a["p1"] = p1;
    a["p2"] = p2;
    return Any();
}


void Cylinder::serialize(class BinaryOutput& b) const {
    p1.serialize(b);
    p2.serialize(b);
    b.writeFloat64(mRadius);
}


void Cylinder::deserialize(class BinaryInput& b) {
    p1.deserialize(b);
    p2.deserialize(b);
    mRadius = (float)b.readFloat64();
}

size_t Cylinder::hashCode() const {
    return (size_t)(mRadius*100.0f) ^ p1.hashCode() ^ p2.hashCode();
}

bool Cylinder::operator==(const Cylinder & other) const {
    return (mRadius == other.mRadius) && (p1 == other.p1) && (p2 == other.p2);
}


Line Cylinder::axis() const {
    return Line::fromTwoPoints(p1, p2);
}



float Cylinder::radius() const {
    return mRadius;
}


float Cylinder::volume() const {
    return
        (float)pi() * square(mRadius) * (p1 - p2).magnitude();
}


float Cylinder::area() const {
    return
        // Sides
        ((float)twoPi() * mRadius) * height() +

         // Caps
         (float)twoPi() * square(mRadius);
}

void Cylinder::getBounds(AABox& out) const {
    Vector3 min = p1.min(p2) - (Vector3(1, 1, 1) * mRadius);
    Vector3 max = p1.max(p2) + (Vector3(1, 1, 1) * mRadius);
    out = AABox(min, max);
}

bool Cylinder::contains(const Vector3& p) const { 
    return LineSegment::fromTwoPoints(p1, p2).distanceSquared(p) <= square(mRadius);
}


void Cylinder::getReferenceFrame(CoordinateFrame& cframe) const {
    cframe.translation = center();

    Vector3 Y = (p1 - p2).direction();
    Vector3 X = (abs(Y.dot(Vector3::unitX())) > 0.9) ? Vector3::unitY() : Vector3::unitX();
    Vector3 Z = X.cross(Y).direction();
    X = Y.cross(Z);        
    cframe.rotation.setColumn(0, X);
    cframe.rotation.setColumn(1, Y);
    cframe.rotation.setColumn(2, Z);
}


void Cylinder::getRandomSurfacePoint(Vector3& p, Vector3& N, Random& rnd) const {
    float h = height();
    float r = radius();

    // Create a random point on a standard cylinder and then rotate to the global frame.

    // Relative areas (factor of 2PI already taken out)
    float capRelArea  = square(r) / 2.0f;
    float sideRelArea = r * h;

    float r1 = uniformRandom(0, capRelArea * 2 + sideRelArea);

    if (r1 < capRelArea * 2) {

        // Select a point uniformly at random on a disk
        // @cite http://mathworld.wolfram.com/DiskPointPicking.html
        float a = rnd.uniform(0, (float)twoPi());
        float r2 = sqrt(rnd.uniform(0, 1)) * r;
        p.x = cos(a) * r2;
        p.z = sin(a) * r2;

        N.x = 0;
        N.z = 0;
        if (r1 < capRelArea) {
            // Top
            p.y = h / 2.0f;
            N.y = 1;
        } else {
            // Bottom
            p.y = -h / 2.0f;
            N.y = -1;
        }
    } else {
        // Side
        float a = rnd.uniform(0, (float)twoPi());
        N.x = cos(a);
        N.y = 0;
        N.z = sin(a);
        p.x = N.x * r;
        p.z = N.y * r;
        p.y = rnd.uniform(-h / 2.0f, h / 2.0f);
    }

    // Transform to world space
    CoordinateFrame cframe;
    getReferenceFrame(cframe);
    
    p = cframe.pointToWorldSpace(p);
    N = cframe.normalToWorldSpace(N);
}


Vector3 Cylinder::randomInteriorPoint(Random& rnd) const {
    float h = height();
    float r = radius();

    // Create a random point in a standard cylinder and then rotate to the global frame.

    // Select a point uniformly at random on a disk
    // @cite http://mathworld.wolfram.com/DiskPointPicking.html
    float a = rnd.uniform(0, (float)twoPi());
    float r2 = sqrt(rnd.uniform(0, 1)) * r;

    const Vector3 p(  cos(a) * r2,
                rnd.uniform(-h / 2.0f, h / 2.0f),
                sin(a) * r2);

    // Transform to world space
    CoordinateFrame cframe;
    getReferenceFrame(cframe);
    
    return cframe.pointToWorldSpace(p);
}

} // namespace
