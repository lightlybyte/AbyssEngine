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
