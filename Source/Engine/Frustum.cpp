#include "Frustum.h"

void Frustum::Extract(const glm::mat4& m) {
    auto row = [&](int r) -> glm::vec4 {
        return { m[0][r], m[1][r], m[2][r], m[3][r] };
    };

    glm::vec4 r0 = row(0);
    glm::vec4 r1 = row(1);
    glm::vec4 r2 = row(2);
    glm::vec4 r3 = row(3);

    glm::vec4 p[6] = {
        r3 + r0, r3 - r0,
        r3 + r1, r3 - r1,
        r3 + r2, r3 - r2
    };

    for (int i = 0; i < 6; ++i) {
        glm::vec4 v = p[i];
        float len = glm::length(glm::vec3(v));
        if (len > 1e-6f) v /= len;
        planes[i].normal = { v.x, v.y, v.z };
        planes[i].d = v.w;
    }
}

bool Frustum::AABBInside(const AABB& box) const {
    for (int i = 0; i < 6; ++i) {
        const glm::vec3& n = planes[i].normal;
        float d = planes[i].d;

        glm::vec3 pos;
        pos.x = (n.x >= 0.0f) ? box.max.x : box.min.x;
        pos.y = (n.y >= 0.0f) ? box.max.y : box.min.y;
        pos.z = (n.z >= 0.0f) ? box.max.z : box.min.z;

        if (glm::dot(n, pos) + d < 0.0f) return false;
    }
    return true;
}