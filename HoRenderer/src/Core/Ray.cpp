/*
	Created by Yinghao He on 2025-05-17
*/
#include "Ray.hpp"


Vector3f Ray::OffsetRayOrigin(const Vector3f &p, const Vector3f &pError, const Vector3f &N, const Vector3f &L)
{
    float d = glm::dot(glm::abs(N), pError);
	Vector3f offset = d * N;
	if (glm::dot(L, N) < 0.0f) {
		offset = -offset;
	}
	Vector3f po = p + offset;
	// Round offset point _po_ away from _p_
	for (int i = 0; i < 3; ++i) {
		if (offset[i] > 0) {
			po[i] = NextFloatUp(po[i]);
		}
		else if (offset[i] < 0) {
			po[i] = NextFloatDown(po[i]);
		}
	}

	return po;
}

Ray Ray::SpawnRay(const Vector3f &pos, const Vector3f &L, const Vector3f &Ng)
{
    // Make sure that L is a unit vector
    Vector3f normL = glm::normalize(L);
    return Ray(OffsetRayOrigin(pos, Vector3f(Epsilon), Ng, normL), normL);
}

