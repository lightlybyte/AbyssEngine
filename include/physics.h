#pragma once
// ---------------------------------------------------------------------------
// physics.h — High-performance rigid body physics.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <glm/glm.hpp>
#include "physics_math.h"

namespace Phys {

// ---------------------------------------------------------------------------
// Rigid body
// ---------------------------------------------------------------------------
struct Body {
    uint32_t id      = 0;
    bool   active    = true;
    bool   kinematic = false;

    Real   mass      = 1.0f;
    Real   invMass   = 1.0f;
    Mat3   invInertia;          // world-space inverse inertia tensor

    Vec3   position;
    Quat   rotation;
    Vec3   linearVelocity;
    Vec3   angularVelocity;

    Vec3   forceAccum;
    Vec3   torqueAccum;

    Real   linearDamping  = 0.98f;
    Real   angularDamping = 0.98f;

    Real   friction       = 0.6f;
    Real   restitution    = 0.0f;

    // Sleeping
    Real   sleepEnergy    = 0.0f;
    bool   asleep         = false;
    static constexpr Real kSleepThreshold = 0.01f;
    static constexpr Real kSleepTime      = 0.5f;
    Real   sleepTimer     = 0.0f;

    // Collision shape
    enum ShapeType { SPHERE, BOX, PLANE, CAPSULE };
    ShapeType shape = BOX;
    Vec3      halfExtents = {0.5f, 0.5f, 0.5f};  // BOX
    Real      radius      = 0.5f;                 // SPHERE/CAPSULE
    Real      height        = 1.0f;               // CAPSULE
    Vec3      planeNormal   = {0, 1, 0};          // PLANE
    Real      planeOffset   = 0.0f;               // PLANE
};

// ---------------------------------------------------------------------------
// Contact manifold
// ---------------------------------------------------------------------------
struct Contact {
    Vec3  point;       // world contact point
    Vec3  normal;      // world contact normal (from A to B)
    Real  depth;       // penetration depth (positive = overlapping)
    Real  friction;
    Real  restitution;
    uint32_t bodyA;
    uint32_t bodyB;
};

// ---------------------------------------------------------------------------
// Physics world — SoA layout for cache efficiency
// ---------------------------------------------------------------------------
class World {
public:
    World();
    ~World();

    // Step the simulation with fixed timestep + sub-stepping.
    void step(Real dt);

    // Body management
    uint32_t addBody(const Body& body);
    void     removeBody(uint32_t id);
    Body*    getBody(uint32_t id);
    const Body* getBody(uint32_t id) const;

    // Gravity
    void setGravity(const Vec3& g);
    Vec3  gravity() const { return gravity_; }

    // Contact query
    const std::vector<Contact>& contacts() const { return contacts_; }

    // Raycast (simplified sphere vs all bodies)
    std::optional<std::pair<uint32_t, Vec3>> raycast(
        const Vec3& origin, const Vec3& direction, Real maxDist) const;

private:
    // Integration (explicit symplectic Euler).
    void integrate(Real dt);
    // Broad-phase: spatial hash.
    void broadPhase();
    // Narrow-phase: generate contacts.
    void narrowPhase();
    // Resolve contacts with iterative impulse solver.
    void resolveContacts(Real dt, int iterations);
    // Sleeping logic.
    void updateSleeping(Real dt);

    // Spatial hash cell
    struct Cell {
        std::vector<uint32_t> bodies;
    };

    // ---- Data (SoA-ish but stored as vector of structs for simplicity) ----
    std::vector<Body>   bodies_;
    std::vector<Contact> contacts_;

    // Spatial hash
    std::vector<Cell>   grid_;
    Real                cellSize_   = 4.0f;
    uint32_t            gridDim_    = 32;

    Vec3                gravity_    = {0.0f, kGravity, 0.0f};
    Real                dtFixed_    = 1.0f / 60.0f;
    int                 maxSubSteps_ = 4;

    // Iterative solver parameters
    Real                baumgarte_  = 0.2f;
    Real                slop_       = 0.01f;
};

} // namespace Phys
