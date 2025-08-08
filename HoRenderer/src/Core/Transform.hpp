/*
    Created by Yinghao He on 2025-06-06
*/
#pragma once

#include "Util.hpp"

class Transform {
public:
    Transform() : identity(true), transformMatrix(Matrix4f(1.0f)) {}
    Transform(const Matrix4f& transformMatrix) : identity(false), transformMatrix(transformMatrix) {}

    Vector3f TransformPoint(const Vector3f& p) const;
    Vector3f TransformVector(const Vector3f& v) const;
    Vector3f TransformNormal(const Vector3f& n) const;

    Matrix4f Mat() const { return transformMatrix; }

    Transform Inverse() const;
    Transform operator*(const Transform& t) const;

    static Transform Translate(const Vector3f& offset);
    static Transform Rotate(const Vector3f& rotation);
    static Transform Scale(const Vector3f& scale);
    static Transform Scale(float scale);

    bool IsIdentity() const { return identity; }

private:
    bool identity;
    Matrix4f transformMatrix;
};