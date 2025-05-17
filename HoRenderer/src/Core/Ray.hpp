/*
	Created by Yinghao He on 2025-05-17
*/
#pragma once

#include "Util.hpp"

class Ray{
public:
    Ray() {}
    Ray(const Vector3f &origin, const Vector3f &direction) : orig(origin), dir(direction) {}

    inline const Vector3f& origin() const { return orig; }
    inline const Vector3f& direction() const { return dir; }

    /*
    * @brief: Fine-tune the starting point of the light to slightly deviate from the surface to avoid erroneous self-intersection due to floating point precision
    *
    * @args: p: original intersection position (starting point to be offset)
    *        pError: floating point error estimate (indicating uncertainty of intersection coordinates)
    *        N: surface normal vector
    *        L: light direction
    * @ret: Return the adjusted point po as the new starting point of the light.
    */
    static Vector3f OffsetRayOrigin(const Vector3f &p, const Vector3f &pError, const Vector3f &N, const Vector3f &L);
   
    /*
    * @brief: Generate a new ray, making sure its starting point has been offset by OffsetRayOrigin to avoid self-intersection.
    *
    * @args: pos: starting point of the ray
    * L: ray direction vector
    * Ng: geometric normal of the surface (used to calculate the offset direction)
    * @ret: Return a new ray Ray, whose starting point has been offset and whose direction is L
    */
    static Ray SpawnRay(const Vector3f& pos, const Vector3f& L, const Vector3f& Ng);

    inline Vector3f at(float t) const { return orig + t * dir; }

public:
    Vector3f orig;
    Vector3f dir;
};