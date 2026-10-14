#pragma once

#include <glm/glm.hpp>

struct Plane {
    glm::vec3 normal;
    float d;
};

struct AABB {
    glm::vec3 min{0};
    glm::vec3 max{0};
};

struct Frustum {
    Plane planes[6];

    void Extract(const glm::mat4& m);
    bool AABBInside(const AABB& box) const;
};