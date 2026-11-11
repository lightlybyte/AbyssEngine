#include <vector>
#include <optional>
#include "physics_math.h"
#include <glm/glm.hpp>

namespace Phys {
struct Body;
struct Contact;
class World {
public:
    World();
    ~World();
    void step(Real dt);
    uint32_t addBody(const Body& body);
    void removeBody(uint32_t id);
    Body* getBody(uint32_t id);
    const Body* getBody(uint32_t id) const;
    void setGravity(const Vec3& g);
    Vec3 gravity() const { return gravity_; }
    const std::vector<Contact>& contacts() const { return contacts_; }
    std::optional<std::pair<uint32_t, Vec3>> raycast(const Vec3& origin, const Vec3& direction, Real maxDist) const;
private:
    std::vector<Body> bodies_;
    std::vector<Contact> contacts_;
    std::vector<std::vector<uint32_t>> grid_;
    Real cellSize_ = 4.0f;
    uint32_t gridDim_ = 32;
    Vec3 gravity_ = {0.0f, kGravity, 0.0f};
    Real dtFixed_ = 1.0f / 60.0f;
    int maxSubSteps_ = 4;
    Real baumgarte_ = 0.2f;
    Real slop_ = 0.01f;
    void integrate(Real dt);
    void broadPhase();
    void narrowPhase();
    void resolveContacts(Real dt, int iterations);
    void updateSleeping(Real dt);
};
}
