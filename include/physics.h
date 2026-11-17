#pragma once
#include <vector>
#include <optional>
#include "physics_math.h"
#include <glm/glm.hpp>

namespace Phys {

// ===========================================================================
// Collision Shapes
// ===========================================================================
enum class ShapeType : uint8_t {
    SPHERE,
    BOX,
    PLANE
};

// ===========================================================================
// Contact Structure
// ===========================================================================
struct Contact {
    uint32_t bodyA = 0;
    uint32_t bodyB = 0;
    Vec3 normal = {0, 1, 0};
    Vec3 point = {0, 0, 0};
    Real depth = 0.0f;
    Real friction = 0.5f;       // Coulomb friction coefficient
    Real restitution = 0.0f;    // Bounciness
};

// ===========================================================================
// Rigid Body
// ===========================================================================
struct Body {
    uint32_t id = 0;
    
    // State
    Vec3 position = {0, 0, 0};
    Quat rotation = {0, 0, 0, 1};
    Vec3 linearVelocity = {0, 0, 0};
    Vec3 angularVelocity = {0, 0, 0};
    
    // Forces/torques (accumulated per step)
    Vec3 forceAccum = {0, 0, 0};
    Vec3 torqueAccum = {0, 0, 0};
    
    // Mass properties
    Real mass = 1.0f;
    Real invMass = 1.0f;
    Mat3 invInertia = Mat3(1.0f);  // World-space inverse inertia
    Mat3 localInvInertia = Mat3(1.0f);  // Local-space inverse inertia (for recompute)
    
    // Damping
    Real linearDamping = 0.01f;
    Real angularDamping = 0.01f;
    
    // Material properties
    Real friction = 0.5f;      // Coulomb friction coefficient
    Real restitution = 0.0f;   // Bounciness (0 = no bounce, 1 = perfect elastic)
    
    // Simulation flags
    bool active = true;
    bool kinematic = false;   // Infinite mass, not affected by forces
    bool asleep = false;
    
    // Sleeping state
    Real sleepEnergy = 0.0f;
    Real sleepTimer = 0.0f;
    
    // Shape type and parameters
    ShapeType shape = ShapeType::SPHERE;
    
    // Shape-specific data (use based on shape type)
    Real radius = 1.0f;
    Vec3 halfExtents = {1, 1, 1};
    Vec3 planeNormal = {0, 1, 0};
    Real planeOffset = 0.0f;
    
    // AABB for broad phase
    Vec3 aabbCenter = {0, 0, 0};
    Real aabbRadius = 1.0f;
    
    void updateAABB() {
        aabbCenter = position;
        if (shape == ShapeType::BOX) {
            aabbRadius = glm::length(halfExtents);
        } else {
            aabbRadius = radius;
        }
    }
    
    static constexpr uint32_t INVALID_ID = UINT32_MAX;
};

// ===========================================================================
// Physics World
// ===========================================================================
class World {
public:
    World();
    ~World();
    
    // Core operations
    void step(Real dt);
    uint32_t addBody(const Body& body);
    void removeBody(uint32_t id);
    Body* getBody(uint32_t id);
    const Body* getBody(uint32_t id) const;
    void setGravity(const Vec3& g);
    Vec3 gravity() const { return gravity_; }
    
    // Accessors
    const std::vector<Contact>& contacts() const { return contacts_; }
    size_t bodyCount() const { return bodies_.size(); }
    
    // Raycasting
    std::optional<std::pair<uint32_t, Vec3>> raycast(
        const Vec3& origin, const Vec3& direction, Real maxDist) const;
    
private:
    std::vector<Body> bodies_;
    std::vector<Contact> contacts_;
    
    // Spatial hash grid for broad phase
    struct Cell {
        std::vector<uint32_t> bodies;
        void clear() { bodies.clear(); }
    };
    std::vector<Cell> grid_;
    Real cellSize_ = 4.0f;
    uint32_t gridDim_ = 32;
    Vec3 origin_ = {-100, -100, -100};
    
    // Integration parameters
    Vec3 gravity_ = {0.0f, kGravity, 0.0f};
    Real dtFixed_ = 1.0f / 60.0f;
    int maxSubSteps_ = 4;
    Real baumgarte_ = 0.2f;
    Real slop_ = 0.01f;
    
    // Internal methods
    void integrate(Real dt);
    void broadPhase();
    void narrowPhase();
    void resolveContacts(Real dt, int iterations);
    void updateSleeping(Real dt);
    
    // Sleeping thresholds
    static constexpr Real kSleepThreshold = 0.01f;
    static constexpr Real kSleepTime = 0.5f;
};

} // namespace Phys
