/*
    Created by Yinghao He on 2025-06-06
*/
#include "Transform.hpp"

Vector3f Transform::TransformPoint(const Vector3f& p) const {
    if (identity) {
        return p;
    }

    Vector4f ret = transformMatrix * Vector4f(p, 1.0f);
    if (std::abs(ret.w - 1.0f) < Epsilon) {
        return Vector3f(ret.x, ret.y, ret.z);
    } else {
        return Vector3f(ret.x, ret.y, ret.z) / ret.w;
    }
}

Vector3f Transform::TransformVector(const Vector3f& v) const {
    if (identity) {
        return v;
    }
    
    Vector4f ret = transformMatrix * Vector4f(v, 0.0f);
    return Vector3f(ret.x, ret.y, ret.z);
}

Vector3f Transform::TransformNormal(const Vector3f& n) const {
    if (identity) {
        return n;
    }
    
    Matrix4f invTranspose = glm::transpose(glm::inverse(transformMatrix));
    Vector4f ret = invTranspose * Vector4f(n, 0.0f);
    return glm::normalize(Vector3f(ret.x, ret.y, ret.z));
}

Transform Transform::Inverse() const {
    if (identity) {
        return Transform();
    }
    return Transform(glm::inverse(transformMatrix));
}

Transform Transform::operator*(const Transform& t) const {
    if (identity) return t;
    if (t.identity) return *this;
    return Transform(transformMatrix * t.transformMatrix);
}

Transform Transform::Translate(const Vector3f &offset) {
    Matrix4f m = glm::translate(Matrix4f(1.0f), Vector3f(offset.x, offset.y, offset.z));
    return Transform(m);
}

Transform Transform::Rotate(const Vector3f &rotation) {
    Matrix4f m = Matrix4f(1.0f);
    m = glm::rotate(m, degrees_to_radians(rotation.x), Vector3f(1.0f, 0.0f, 0.0f));
    m = glm::rotate(m, degrees_to_radians(-rotation.y), Vector3f(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, degrees_to_radians(rotation.z), Vector3f(0.0f, 0.0f, 1.0f));
    return Transform(m);
}

Transform Transform::Scale(const Vector3f &scale) {
    Matrix4f m = glm::scale(Matrix4f(1.0f), Vector3f(scale.x, scale.y, scale.z));
    return Transform(m);
}

Transform Transform::Scale(float scale) {
    Matrix4f m = glm::scale(Matrix4f(1.0f), Vector3f(scale, scale, scale));
    return Transform(m);
}