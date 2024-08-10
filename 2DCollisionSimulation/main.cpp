#include <SFML/Graphics.hpp>
#include <chrono>
#include "physics.h"
#include "render.h"
#include <string>
#include "threadPool.h"

const float radius = 1.325f;


int main() {
    const Vec2 worldSize{ 1200,1200 };
    // 创建一个窗口
    sf::RenderWindow window(sf::VideoMode(worldSize.x, worldSize.y), "Batch Render Colored Quads");

    auto time_now = std::chrono::high_resolution_clock::now();
    auto time_last = time_now;

    PhysicSolver solver(worldSize, radius);

    tp::ThreadPool threadPool(10);

    Renderer render(solver, radius);
    solver.gravity.y = 500.f;
    solver.friction = 80.f;
    solver.sub_steps = 1;

    Emiter emiter;
    emiter.Position = Vec2(25.f,25.f);
    emiter.Speed.x = 400.;
    emiter.emitNum = 50;
    emiter.intervel = 0.0075f;

    Emiter emiter1;
    emiter1.Position = Vec2(worldSize.x - 25.5f, 25.f);
    emiter1.Speed.x = -400.;
    emiter1.emitNum = 50;
    emiter1.intervel = 0.0075f;

    double accum = 0.;

    float dt = 1 / 1000.f;

    // 主循环
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        time_now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(time_now - time_last).count();
        time_last = time_now;

        if (elapsed < 1. / 75.) {
            emiter.Emit(solver, dt);
            emiter1.Emit(solver, dt);
        }
        
        solver.updateMultiThread(dt,threadPool);

        render.text.setString("FPS: "+std::to_string(1.0/ elapsed)+"\nNum: "+std::to_string(solver.objects.size()));

        window.clear();
        //render.render(window);
        render.renderMultiThread(window, threadPool);
        window.display();
    }

    return 0;
}