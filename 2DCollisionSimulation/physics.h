#pragma once
#include <vector>
#include "physicObject.h"
#include "collision.h"
#include "utils.h"
#include <algorithm>
#include "threadPool.h"

//template<typename T>
//class MultiVector{
//    std::vector<T> Date;
//    std::mutex Write;
//public:
//
//    inline void push_back(T& value){
//        //std::lock_guard<std::mutex> lock(Write);
//        Date.push_back(value);
//    }
//
//    inline void Set(T& value, uint32_t i){
//        //std::lock_guard<std::mutex> lock(Write);
//        Date[i] = value;
//    }
//
//    inline const T& operator[] (int i) {
//        return Date[i];
//    }
//
//    inline int size() {
//        return Date.size();
//    }
//
//    inline void clear() {
//        Date.clear();
//    }
//};

template<typename T,uint32_t max>
class MultiVector{
    std::mutex Write;
    T Date[max];
    uint32_t num = 0;
public:

    MultiVector() = default;

    MultiVector(const MultiVector<T, max>& a) {
        for (int i = 0; i < a.size();i++)
            Date[i] = a[i];
    }

    inline void push_back(T& value){
        std::lock_guard<std::mutex> lock(Write);
        if (num >= max)
            return;
        Date[num] = value;
        num += num < max;
    }

    inline void Set(T& value, uint32_t i){
        std::lock_guard<std::mutex> lock(Write);
        Date[i] = value;
    }

    inline const T& operator[] (int i) const {
        if (i >= max)
            return 0;
        return Date[i];
    }

    inline uint32_t size() const {
        return num;
    }

    inline void clear() {
        num = 0;
    }
};

struct Grid {
    const int32_t size;
    const int32_t sizeX;
    const int32_t sizeY;

    std::vector<MultiVector<uint32_t, 10>> Date;

    Grid(uint32_t sizeX,uint32_t sizeY)
    :size(sizeX*sizeY),sizeX(sizeX),sizeY(sizeY){
        Date.resize(size);
    }

    void Clear() {
        for (int i = 0; i < size; i++) {
            Date[i].clear();
        }
    }

    void Insert(const Vec2& position, uint32_t id,const Vec2& WorldSize,float radius) {
        Vec2 size = Vec2(WorldSize.x / sizeX,WorldSize.y / sizeY);
        uint32_t lx = (position.x - radius) / size.x;
        uint32_t ly = (position.y - radius) / size.y;
        uint32_t rx = (position.x + radius) / size.x;
        uint32_t ry = (position.y + radius) / size.y;

        Date[ly*sizeY + lx].push_back(id);

        if (rx > lx) {
            Date[rx + ly * sizeY].push_back(id);
            if (ry > ly) {
                Date[lx + ry * sizeY].push_back(id);
                Date[rx + ry * sizeY].push_back(id);
            }
                
        }else if (ry > ly)
            Date[lx + ry * sizeY].push_back(id);
    }
};

struct PhysicSolver
{
    std::vector<PhysicObject> objects;
    Vec2 world_size;
    Vec2 gravity = { 0.0f, 20.0f };
    Grid grid;

    // Simulation solving pass count
    uint32_t sub_steps;
    float radius;
    float diameter;
    float diameter2;
    float friction = 50.;

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

    void checkAtomCellCollisions(uint32_t atom_idx, const CollisionCell<16>& c)
    {
        for (uint32_t i{ 0 }; i < c.objects_count; ++i) {
            solveContact(atom_idx, c.objects[i]);
        }
    }

    void processCell(const CollisionCell<16>& c, uint32_t index)
    {
    }

    void solveCollision(uint32_t start, uint32_t end)
    {
        grid.Clear();


        for (int i = 0; i < objects.size();i++) {
            grid.Insert(objects[i].position, i, world_size, radius);
        }

        for (int i = 0; i < grid.size; i++) {
            auto& cell = grid.Date[i];
            for (int m = 0; m < cell.size(); m++) {
                for (int n = 0; n < cell.size(); n++) {
                    solveContact(cell[m], cell[n]);
                }
            }
        }
    }

    void solveCollisionMulti(uint32_t start, uint32_t end)
    {
        for (int i = start; i < end; i++) {
            auto& cell = grid.Date[i];
            for (int m = 0; m < cell.size(); m++) {
                for (int n = 0; n < cell.size(); n++) {
                    solveContact(cell[m], cell[n]);
                }
            }
        }
    }

    // Find colliding atoms
    void solveCollisions()
    {
        /*for (uint32_t i = 0; i < objects.size(); i++) {
            for (uint32_t j = i+1; j < objects.size(); j++) {
                solveContact(i, j);
            }
        }*/

        solveCollision(0, objects.size() - 1);
    }

    void solveCollisionsMulti(tp::ThreadPool& tp)
    {
        /*for (uint32_t i = 0; i < objects.size(); i++) {
            for (uint32_t j = i+1; j < objects.size(); j++) {
                solveContact(i, j);
            }
        }*/

        grid.Clear();

        tp.dispatch(to<uint32_t>(objects.size()), [this](uint32_t start, uint32_t end) {
            for (int i = start; i < end; i++) {
                grid.Insert(objects[i].position, i, world_size, radius);
            }
         });

        tp.waitForCompletion();

        tp.dispatch(to<uint32_t>(grid.size), [this](uint32_t start, uint32_t end) {
                solveCollisionMulti(start, end);
        });
        
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

    void update(float dt)
    {
        const float sub_dt = dt / to<float>(sub_steps);
        for (uint32_t i(sub_steps); i--;) {
            solveCollisions();
            updateObjects(sub_dt);
        }
    }

    void updateMultiThread(float dt,tp::ThreadPool& tp)
    {
        const float sub_dt = dt / to<float>(sub_steps);
        for (uint32_t i(sub_steps); i--;) {
            solveCollisionsMulti(tp);
            updateObjectsMultiThread(sub_dt,tp);
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

    }

    void updateObjectsMultiThread(float dt,tp::ThreadPool& tp)
    {
        tp.dispatch(to<uint32_t>(objects.size()), [this,dt](uint32_t start, uint32_t end) {
            for (uint32_t i = start; i < end; ++i) {

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
    float intervel = 0.25f;
    uint32_t emitNum = 1;


    void Emit(PhysicSolver& solver,float dt) {
        time += dt;

        if (intervel < time) {
            time = 0.f;

            float intv = solver.diameter*1.01f;
            auto pos = Position - Vec2(0, intv * emitNum / 2);

            for (int i = 0; i < emitNum; i++) {
                auto id = solver.createObject(Position + Vec2(0, intv * i));
                auto& obj = solver.objects[id];
                obj.last_position -= Speed * dt;
                obj.color = ColorUtils::getRainbow(id * 0.000005f);
            }
        }
    }
};