#pragma once
#include <vector>
#include "physicObject.h"
#include "collision.h"
#include "utils.h"

struct PhysicSolver
{
    std::vector<PhysicObject> objects;
    Vec2 world_size;
    Vec2 gravity = { 0.0f, 20.0f };

    // Simulation solving pass count
    uint32_t sub_steps;
    float diameter;
    float diameter2;

    PhysicSolver(Vec2 size,float radius = 5.f)
        : world_size{ to<float>(size.x), to<float>(size.y) }
        , sub_steps{ 4 }, diameter(radius*2), diameter2(radius* radius*4)
    {
    }

    void setRadius(float radius) {
        diameter = radius * 2;
        diameter2 = radius * radius * 4;
    }

    void setGravity(Vec2 gravity) {
        this->gravity = gravity;
    }

    // Checks if two atoms are colliding and if so create a new contact
    void solveContact(uint32_t atom_1_idx, uint32_t atom_2_idx)
    {
        constexpr float response_coef = 1.0f;
        constexpr float eps = 0.0001f;
        PhysicObject& obj_1 = objects[atom_1_idx];
        PhysicObject& obj_2 = objects[atom_2_idx];
        const Vec2 o2_o1 = obj_1.position - obj_2.position;
        const float dist2 = o2_o1.x * o2_o1.x + o2_o1.y * o2_o1.y;
        if (dist2 < diameter2 && dist2 > eps) {
            const float dist = sqrt(dist2);
            const float delta = response_coef * 0.5f * (diameter - dist);
            const Vec2 col_vec = (o2_o1 / dist) * delta;
            obj_1.position += col_vec;
            obj_2.position -= col_vec;
        }
    }

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell& c)
    {
        for (uint32_t i{ 0 }; i < c.objects_count; ++i) {
            solveContact(atom_idx, c.objects[i]);
        }
    }

    void processCell(const CollisionCell& c, uint32_t index)
    {
    }

    void solveCollisionThreaded(uint32_t start, uint32_t end)
    {

    }

    // Find colliding atoms
    void solveCollisions()
    {
        for (uint32_t i = 0; i < objects.size(); i++) {
            for (uint32_t j = i+1; j < objects.size(); j++) {
                solveContact(i, j);
            }
        }
    }

    // Add a new object to the solver
    uint64_t addObject(const PhysicObject& object)
    {
        objects.push_back(object);
        return objects.size()-1;
    }

    // Add a new object to the solver
    uint64_t createObject(Vec2 pos)
    {
        objects.emplace_back(pos);
        return objects.size() - 1;
    }

    void update(float dt)
    {
        const float sub_dt = dt / to<float>(sub_steps);
        for (uint32_t i(sub_steps); i--;) {
            solveCollisions();
            updateObjects(sub_dt);
        }
    }

    void updateObjects(float dt)
    {
        uint32_t end = objects.size();

        for (uint32_t i=0; i < end; ++i) {
            PhysicObject& obj = objects[i];
            // Add gravity
            obj.acceleration += gravity;
            // Apply Verlet integration
            obj.update(dt);
            // Apply map borders collisions
            const float margin = diameter;
            if (obj.position.x > world_size.x - margin) {
                obj.position.x = world_size.x - margin;
            }
            else if (obj.position.x < margin) {
                obj.position.x = margin;
            }
            if (obj.position.y > world_size.y - margin) {
                obj.position.y = world_size.y - margin;
            }
            else if (obj.position.y < margin) {
                obj.position.y = margin;
            }
        }

    }
};

struct Emiter {
    Vec2 Speed = { 10.f,0.f };
    Vec2 Position = { 10.f,10.f };
    float time = 0.f;
    float intervel = 0.25f;
    uint32_t emitNum = 1;


    void Emit(PhysicSolver& solver,float dt) {
        time += dt;

        if (intervel < time) {
            time = 0.f;

            float intv = solver.diameter + 0.1f;
            auto pos = Position - Vec2(0, intv * emitNum / 2);

            for (int i = 0; i < emitNum; i++) {
                auto id = solver.createObject(Position + Vec2(0, intv * i));
                auto& obj = solver.objects[id];
                obj.last_position -= Speed * dt;
                obj.color = ColorUtils::getRainbow(id * 0.001f);
            }
        }
    }
};