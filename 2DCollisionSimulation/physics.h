#pragma once
#include <vector>
#include "physicObject.h"
#include "collision.h"
#include "utils.h"
#include <algorithm>
#include "threadPool.h"

using cell = CollisionCell<5>;

struct Grid {
    const unsigned int size;
    const unsigned int sizeX;
    const unsigned int sizeY;

    std::vector<cell> Date;

    Grid(unsigned int sizeX,unsigned int sizeY)
    :size(sizeX*sizeY),sizeX(sizeX),sizeY(sizeY){
        Date.resize(size);
    }

    void Clear() {
        for (int i = 0; i < size; i++) {
            Date[i].clear();
        }
    }

    void Insert(const Vec2& position, unsigned int id,const Vec2& WorldSize,float radius) {
        unsigned int x = position.x * sizeX / WorldSize.x;
        unsigned int y = position.y * sizeY / WorldSize.y;

        Date[x + y * sizeX].push_back(id);
    }

    inline cell& Get(unsigned int x,unsigned int y) {
        return Date[x + y * sizeX];
    }
};

struct PhysicSolver
{
    std::vector<PhysicObject> objects;
    Vec2 world_size;
    Vec2 gravity = { 0.0f, 20.0f };
    Grid grid;

    // Simulation solving pass count
    unsigned int sub_steps;
    float radius;
    float diameter;
    float diameter2;
    float friction = 50.;
    float response_coef = 0.1f;

    PhysicSolver(Vec2 size,float radius = 5.f)
        : world_size{ to<float>(size.x), to<float>(size.y) }
        , sub_steps{ 8 }, radius(radius), diameter(radius * 2), diameter2(radius* radius * 4), grid(size.x / (radius * 2), size.y / (radius * 2))
    {
    }

    void setRadius(float radius) {
        this->radius = radius;
        diameter = radius * 2;
        diameter2 = radius * radius * 4;
    }

    void setGravity(Vec2 gravity) {
        this->gravity = gravity;
    }

    // Checks if two atoms are colliding and if so create a new contact
    void solveContact(unsigned int atom_1_idx, unsigned int atom_2_idx)
    {
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

    void checkCellCollisions(const cell& a, const cell& b)
    {
        for (int i = 0; i < a.objects_count; ++i) {
            for (int j = 0; j < b.objects_count; ++j) {
                solveContact(a.objects[i], b.objects[j]);
            }
        }
    }

    void solveCollision(unsigned int start, unsigned int end)
    {
        for (int i = start; i < end; ++i) {
            auto& the = grid.Date[i];
            checkCellCollisions(the, the);

            if (i % grid.sizeX) {
                checkCellCollisions(the, grid.Date[i - 1]);
                if (i + grid.sizeX - 1 < grid.size)
                    checkCellCollisions(the, grid.Date[i + grid.sizeX - 1]);
            }

            if ((i + 1) % grid.sizeX && i+1<grid.size) {
                checkCellCollisions(the, grid.Date[i + 1]);
                if (i + grid.sizeX + 1 < grid.size)
                    checkCellCollisions(the, grid.Date[i + grid.sizeX + 1]);
            }

            if (i + grid.sizeX < grid.size) {
                checkCellCollisions(the, grid.Date[i + grid.sizeX]);
            }
        }
    }

    void solveCollisions_Multi(tp::ThreadPool& tp)
    {
        for (int i = 0; i < grid.sizeY; i += 2) {
            tp.addTask([i,this] {
                solveCollision(i * grid.sizeX, i * grid.sizeX + grid.sizeX);
                });
        }


        tp.waitForCompletion();

        for (int i = 1; i < grid.sizeY; i += 2) {
            tp.addTask([i, this] {
                solveCollision(i * grid.sizeX, i * grid.sizeX + grid.sizeX);
                });
        }

    }

    // Add a new object to the solver
    uint64_t addObject(const PhysicObject& object)
    {
        objects.push_back(object);
        return objects.size() - 1;
    }

    // Add a new object to the solver
    uint64_t createObject(Vec2 pos)
    {
        objects.emplace_back(pos);
        return objects.size() - 1;
    }

    void addObjectsToGrid_Multi(tp::ThreadPool& tp) {
        grid.Clear();

        tp.dispatch(objects.size(), [this](uint32_t start, uint32_t end) {
            for (int i = start; i < end; i++) {
                grid.Insert(objects[i].position, i, world_size, radius);
            }
            });
    }

    void update(float dt,tp::ThreadPool& tp)
    {
        const float sub_dt = dt / to<float>(sub_steps);
        for (unsigned int i(sub_steps); i--;) {
            addObjectsToGrid_Multi(tp);
            solveCollisions_Multi(tp);
            updateObjects_Multi(sub_dt,tp);
        }
    }

    void updateObjects_Multi(float dt,tp::ThreadPool& tp)
    {
        tp.dispatch(to<unsigned int>(objects.size()), [this,dt](unsigned int start, unsigned int end) {
            for (unsigned int i = start; i < end; ++i) {

                PhysicObject& obj = objects[i];
                // Add gravity
                obj.acceleration += gravity;
                // Apply Verlet integration
                obj.update(dt,friction);
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
        });

    }
};

struct Emiter {
    Vec2 Speed = { 10.f,0.f };
    Vec2 Position = { 10.f,10.f };
    float time = 0.f;
    float intervel = 1.1f;
    unsigned int emitNum = 1;


    void Emit(PhysicSolver& solver,float dt) {
        time += dt;

        

        if (time*Math::length(Speed.x,Speed.y)>intervel*solver.diameter) {
            time = 0.f;

            float intv = solver.diameter*1.01f;
            auto pos = Position - Vec2(0, intv * emitNum / 2);

            for (int i = 0; i < emitNum; i++) {
                auto id = solver.createObject(Position + Vec2(0, intv * i));
                auto& obj = solver.objects[id];
                obj.last_position -= Speed * dt;
                obj.color = ColorUtils::getRainbow(id * 0.00001f);
            }
        }
    }
};