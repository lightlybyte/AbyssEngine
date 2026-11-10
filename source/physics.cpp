// ---------------------------------------------------------------------------
// physics.cpp — High-performance rigid body physics implementation.
// ---------------------------------------------------------------------------

#include "physics.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace Phys {

// ===========================================================================
// World
// ===========================================================================

World::World() = default;
World::~World() = default;

void World::setGravity(const Vec3& g) { gravity_ = g; }

uint32_t World::addBody(const Body& body) {
    bodies_.push_back(body);
    return static_cast<uint32_t>(bodies_.size() - 1);
}

void World::removeBody(uint32_t id) {
    if (id >= bodies_.size()) return;
    bodies_[id] = bodies_.back();
    bodies_.pop_back();
}

Body* World::getBody(uint32_t id) {
    return (id < bodies_.size()) ? &bodies_[id] : nullptr;
}

const Body* World::getBody(uint32_t id) const {
    return (id < bodies_.size()) ? &bodies_[id] : nullptr;
}

// ---------------------------------------------------------------------------
// Fixed-timestep step with sub-stepping
// ---------------------------------------------------------------------------
void World::step(Real dt) {
    dt = clamp(dt, 0.0f, 0.25f);

    Real remaining = dt;
    int   subSteps = 0;
    while (remaining > kEps && subSteps < maxSubSteps_) {
        Real h = std::min(remaining, dtFixed_);
        integrate(h);
        broadPhase();
        narrowPhase();
        resolveContacts(h, 8);
        updateSleeping(h);
        remaining -= h;
        ++subSteps;
    }
}
// ---------------------------------------------------------------------------
// Integration — symplectic Euler with damping
// ---------------------------------------------------------------------------
void World::integrate(Real dt) {
    for (auto& b : bodies_) {
        if (!b.active || b.asleep || b.kinematic) continue;

        b.linearVelocity += gravity_ * dt;
        b.linearVelocity += b.forceAccum * b.invMass * dt;
        b.angularVelocity += b.invInertia * b.torqueAccum * dt;

        b.linearVelocity  *= b.linearDamping;
        b.angularVelocity *= b.angularDamping;

        b.forceAccum  = {0, 0, 0};
        b.torqueAccum = {0, 0, 0};

        b.position += b.linearVelocity * dt;

        Quat dq = b.angularVelocity * dt;
        b.rotation = b.rotation * Quat(dq.x, dq.y, dq.z, 0.0f) * 0.5f + b.rotation;
        b.rotation = glm::normalize(b.rotation);

        Mat3 rotMat = glm::mat3_cast(b.rotation);
        Mat3 localInvInertia = glm::mat3(1.0f) / (b.mass * 0.166f);
        b.invInertia = rotMat * localInvInertia * glm::transpose(rotMat);

        b.sleepEnergy = glm::dot(b.linearVelocity, b.linearVelocity) +
                        glm::dot(b.angularVelocity, b.angularVelocity);
    }
}

// ---------------------------------------------------------------------------
// Broad phase — uniform spatial hash
// ---------------------------------------------------------------------------
void World::broadPhase() {
    for (auto& cell : grid_) cell.bodies.clear();

    for (size_t i = 0; i < bodies_.size(); ++i) {
        Body& b = bodies_[i];
        if (!b.active || b.asleep || b.kinematic) continue;

        Vec3 minB = b.position;
        Vec3 maxB = b.position;
        if (b.shape == Body::BOX) {
            minB -= b.halfExtents;
            maxB += b.halfExtents;
        } else if (b.shape == Body::SPHERE) {
            minB -= Vec3(b.radius);
            maxB += Vec3(b.radius);
        }

        int32_t cx = static_cast<int32_t>(std::floor(minB.x / cellSize_));
        int32_t cy = static_cast<int32_t>(std::floor(minB.y / cellSize_));
        int32_t cz = static_cast<int32_t>(std::floor(minB.z / cellSize_));

        uint32_t h = (static_cast<uint32_t>(cx) * 73856093u) ^
                     (static_cast<uint32_t>(cy) * 19349663u) ^
                     (static_cast<uint32_t>(cz) * 83492791u);
        uint32_t idx = h % grid_.size();
        grid_[idx].bodies.push_back(static_cast<uint32_t>(i));
    }
}

// ---------------------------------------------------------------------------
// Narrow phase — generate contacts between candidate pairs
// ---------------------------------------------------------------------------
static bool testSphereSphere(const Body& a, const Body& b, Contact& c) {
    Vec3 delta = b.position - a.position;
    Real distSq = glm::dot(delta, delta);
    Real radii = a.radius + b.radius;
    if (distSq >= radii * radii) return false;
    Real dist = std::sqrt(distSq);
    if (dist < kEps) dist = kEps;
    c.normal = delta / dist;
    c.point  = a.position + c.normal * a.radius;
    c.depth  = radii - dist;
    return true;
}

static bool testSpherePlane(const Body& sphere, const Body& plane, Contact& c) {
    Vec3 n = glm::normalize(plane.planeNormal);
    Real dist = glm::dot(sphere.position, n) - plane.planeOffset;
    if (dist >= sphere.radius) return false;
    c.normal = -n;
    c.depth  = sphere.radius - dist;
    c.point  = sphere.position - n * sphere.radius;
    return true;
}

static bool testBoxBox(const Body& a, const Body& b, Contact& c) {
    Mat3 ra = glm::mat3_cast(a.rotation);
    Mat3 rb = glm::mat3_cast(b.rotation);
    Vec3  ta = a.position;
    Vec3  tb = b.position;
    Vec3  ea = a.halfExtents;
    Vec3  eb = b.halfExtents;

    Vec3  R[3][3];
    Real  absR[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            R[i][j] = glm::dot(ra[i], rb[j]);
            absR[i][j] = std::abs(R[i][j]);
        }
    }
    Vec3 t = tb - ta;

    for (int i = 0; i < 3; ++i) {
        Real ra_ = ea[i];
        Real rb_ = eb[0]*absR[i][0] + eb[1]*absR[i][1] + eb[2]*absR[i][2];
        if (std::abs(glm::dot(t, ra[i])) > ra_ + rb_) return false;
    }
    for (int j = 0; j < 3; ++j) {
        Real ra_ = ea[0]*absR[0][j] + ea[1]*absR[1][j] + ea[2]*absR[2][j];
        if (std::abs(glm::dot(t, rb[j])) > ra_ + eb[j]) return false;
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vec3 axis = glm::cross(ra[i], rb[j]);
            if (glm::length(axis) < kEps) continue;
            Real ra_ = ea[(i+1)%3]*absR[(i+2)%3][j] + ea[(i+2)%3]*absR[(i+1)%3][j];
            Real rb_ = eb[(j+1)%3]*absR[i][(j+2)%3] + eb[(j+2)%3]*absR[i][(j+1)%3];
            if (std::abs(glm::dot(t, axis)) > ra_ + rb_) return false;
        }
    }

    c.normal = glm::normalize(t);
    if (glm::length(c.normal) < kEps) c.normal = {0,1,0};
    c.depth  = 0.01f;
    c.point  = (ta + tb) * 0.5f;
    return true;
}

void World::narrowPhase() {
    contacts_.clear();

    std::vector<std::pair<uint32_t,uint32_t>> pairs;
    pairs.reserve(256);

    for (auto& cell : grid_) {
        const auto& bs = cell.bodies;
        for (size_t i = 0; i < bs.size(); ++i) {
            for (size_t j = i + 1; j < bs.size(); ++j) {
                pairs.emplace_back(bs[i], bs[j]);
            }
        }
    }

    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());

    Contact c;
    c.bodyA = c.bodyB = 0;
    for (const auto& p : pairs) {
        Body* A = getBody(p.first);
        Body* B = getBody(p.second);
        if (!A || !B) continue;
        if (!A->active || !B->active) continue;
        if (A->asleep && B->asleep) continue;

        bool hit = false;
        if (A->shape == Body::SPHERE && B->shape == Body::SPHERE)
            hit = testSphereSphere(*A, *B, c);
        else if (A->shape == Body::SPHERE && B->shape == Body::PLANE)
            hit = testSpherePlane(*A, *B, c);
        else if (A->shape == Body::PLANE && B->shape == Body::SPHERE) {
            hit = testSpherePlane(*B, *A, c);
            if (hit) c.normal = -c.normal;
        }
        else if (A->shape == Body::BOX && B->shape == Body::BOX)
            hit = testBoxBox(*A, *B, c);
        else
            hit = testSphereSphere(*A, *B, c);

        if (hit) {
            c.bodyA = p.first;
            c.bodyB = p.second;
            c.friction = std::min(A->friction, B->friction);
            c.restitution = std::max(A->restitution, B->restitution);
            contacts_.push_back(c);
        }
    }
}

// ---------------------------------------------------------------------------
// Contact resolution — iterative impulse solver
// ---------------------------------------------------------------------------
void World::resolveContacts(Real dt, int iterations) {
    if (contacts_.empty()) return;

    for (int it = 0; it < iterations; ++it) {
        for (auto& contact : contacts_) {
            Body* A = getBody(contact.bodyA);
            Body* B = getBody(contact.bodyB);
            if (!A || !B) continue;
            if (!A->active && !B->active) continue;
            if (A->kinematic && B->kinematic) continue;

            Vec3 rA = contact.point - A->position;
            Vec3 rB = contact.point - B->position;

            Vec3 vA = A->linearVelocity + glm::cross(A->angularVelocity, rA);
            Vec3 vB = B->linearVelocity + glm::cross(B->angularVelocity, rB);
            Vec3  vRel = vA - vB;

            Real velNormal = glm::dot(vRel, contact.normal);
            if (velNormal > 0.0f) continue;

            Real invMassA = A->invMass;
            Real invMassB = B->invMass;

            Vec3 rA_cross_n = glm::cross(rA, contact.normal);
            Vec3 rB_cross_n = glm::cross(rB, contact.normal);

            Real invInertiaTermA = glm::dot(rA_cross_n, A->invInertia * rA_cross_n);
            Real invInertiaTermB = glm::dot(rB_cross_n, B->invInertia * rB_cross_n);

            Real invMassTotal = invMassA + invMassB + invInertiaTermA + invInertiaTermB;
            if (invMassTotal <= 0.0f) continue;

            Real e = contact.restitution;
            Real j = -(1.0f + e) * velNormal / invMassTotal;
            Real jClamped = std::max(j, 0.0f);

            Vec3 impulse = contact.normal * jClamped;

            if (!A->kinematic) {
                A->linearVelocity  -= impulse * A->invMass;
                A->angularVelocity -= A->invInertia * glm::cross(rA, impulse);
            }
            if (!B->kinematic) {
                B->linearVelocity  += impulse * B->invMass;
                B->angularVelocity += B->invInertia * glm::cross(rB, impulse);
            }

            Vec3 tangentVel = vRel - contact.normal * velNormal;
            Real tanSpeed = glm::length(tangentVel);
            if (tanSpeed > kEps) {
                Vec3 tangent = tangentVel / tanSpeed;
                Vec3 rA_cross_t = glm::cross(rA, tangent);
                Vec3 rB_cross_t = glm::cross(rB, tangent);
                Real invMassTan = invMassA + invMassB +
                                  glm::dot(rA_cross_t, A->invInertia * rA_cross_t) +
                                  glm::dot(rB_cross_t, B->invInertia * rB_cross_t);
                if (invMassTan > 0.0f) {
                    Real jt = -tanSpeed / invMassTan;
                    Real maxFriction = contact.friction * jClamped;
                    jt = clamp(jt, -maxFriction, maxFriction);
                    Vec3 frictionImpulse = tangent * jt;
                    if (!A->kinematic) {
                        A->linearVelocity  -= frictionImpulse * A->invMass;
                        A->angularVelocity -= A->invInertia * glm::cross(rA, frictionImpulse);
                    }
                    if (!B->kinematic) {
                        B->linearVelocity  += frictionImpulse * B->invMass;
                        B->angularVelocity += B->invInertia * glm::cross(rB, frictionImpulse);
                    }
                }
            }

            Real depth = std::max(contact.depth - slop_, 0.0f);
            Real bias = baumgarte_ * depth / dt;
            Vec3 correction = contact.normal * bias / invMassTotal;
            if (!A->kinematic) A->position -= correction * A->invMass;
            if (!B->kinematic) B->position += correction * B->invMass;
        }
    }
}

// ---------------------------------------------------------------------------
// Sleeping
// ---------------------------------------------------------------------------
void World::updateSleeping(Real dt) {
    for (auto& b : bodies_) {
        if (!b.active || b.kinematic) continue;

        if (b.sleepEnergy < kSleepThreshold) {
            b.sleepTimer += dt;
            if (b.sleepTimer > kSleepTime) b.asleep = true;
        } else {
            b.sleepTimer = 0.0f;
            b.asleep = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Raycast
// ---------------------------------------------------------------------------
std::optional<std::pair<uint32_t, Vec3>> World::raycast(
    const Vec3& origin, const Vec3& direction, Real maxDist) const {

    Vec3 dir = glm::normalize(direction);
    Real bestT = kInf;
    uint32_t bestId = UINT32_MAX;
    Vec3   bestNormal = {0,0,0};

    for (const auto& b : bodies_) {
        if (!b.active) continue;

        Real t = kInf;
        Vec3 normal = {0,0,0};

        if (b.shape == Body::SPHERE) {
            Vec3 oc = origin - b.position;
            Real a = glm::dot(dir, dir);
            Real bb = 2.0f * glm::dot(oc, dir);
            Real c = glm::dot(oc, oc) - b.radius * b.radius;
            Real disc = bb * bb - 4.0f * a * c;
            if (disc < 0.0f) continue;
            t = (-bb - std::sqrt(disc)) / (2.0f * a);
            if (t < 0.0f) t = (-bb + std::sqrt(disc)) / (2.0f * a);
            if (t < 0.0f || t > maxDist) continue;
            normal = glm::normalize(origin + dir * t - b.position);
        }
        else if (b.shape == Body::PLANE) {
            Real denom = glm::dot(dir, b.planeNormal);
            if (std::abs(denom) < kEps) continue;
            t = (b.planeOffset - glm::dot(origin, b.planeNormal)) / denom;
            if (t < 0.0f || t > maxDist) continue;
            normal = -b.planeNormal;
        }
        else {
            Real radius = glm::length(b.halfExtents);
            Vec3 oc = origin - b.position;
            Real bb = 2.0f * glm::dot(oc, dir);
            Real c = glm::dot(oc, oc) - radius * radius;
            Real disc = bb * bb - 4.0f * c;
            if (disc < 0.0f) continue;
            t = (-bb - std::sqrt(disc)) / 2.0f;
            if (t < 0.0f) t = (-bb + std::sqrt(disc)) / 2.0f;
            if (t < 0.0f || t > maxDist) continue;
            normal = glm::normalize(origin + dir * t - b.position);
        }

        if (t < bestT) {
            bestT   = t;
            bestId  = b.id;
            bestNormal = normal;
        }
    }

    if (bestId == UINT32_MAX) return std::nullopt;
    return std::make_pair(bestId, origin + dir * bestT);
}

} // namespace Phys
