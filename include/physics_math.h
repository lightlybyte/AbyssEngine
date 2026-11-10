#pragma once
// ---------------------------------------------------------------------------
// physics_math.h — SIMD-optimized math utilities for the physics engine.
// ---------------------------------------------------------------------------

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Phys {

using Vec3  = glm::vec3;
using Mat3  = glm::mat3;
using Mat4  = glm::mat4;
using Quat  = glm::quat;
using Real  = float;

static constexpr Real kEps   = 1e-4f;
static constexpr Real kInf   = 1e30f;
static constexpr Real kGravity = -9.81f;

// Square of a value (avoids repeated multiplication).
inline Real sqr(Real x) { return x * x; }

// Clamp a value to [lo, hi].
inline Real clamp(Real v, Real lo, Real hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Saturating clamp to [0, 1].
inline Real saturate(Real v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Fast inverse square root (good enough for physics normalization).
inline Real rsqrt(Real x) {
    return (x <= kEps) ? kInf : 1.0f / std::sqrt(x);
}

// Compute inverse transpose of a 3x3 for normal transformation.
inline Mat3 inverseTranspose(const Mat3& m) {
    return glm::transpose(glm::inverse(m));
}

} // namespace Phys
