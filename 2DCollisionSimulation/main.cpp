#include <SFML/Graphics.hpp>
#include <chrono>
#include "physics.h"
#include "render.h"
#include <string>
#include "threadPool.h"

const float radius = 1.2f;


int main() {
    const Vec2 worldSize{ 1600,1600 };
    // 创建一个窗口
    sf::RenderWindow window(sf::VideoMode(worldSize.x, worldSize.y), "Batch Render Colored Quads");

    auto time_now = std::chrono::high_resolution_clock::now();
    auto time_last = time_now;

    PhysicSolver solver(worldSize, radius);

    tp::ThreadPool threadPool(15);

    Renderer render(solver, radius);
    solver.gravity.y = 40.f;
    solver.friction = 40.f;
    solver.sub_steps = 1;
    solver.response_coef = 0.6f;

    float speed = 180.f;
    float interval = 1.2f;
    int num = 50;

    Emiter emiter;
    emiter.Position = Vec2(30.f,30.f);
    emiter.Speed.x = speed;
    emiter.emitNum = num;
    emiter.intervel = interval;

    Emiter emiter1;
    emiter1.Position = Vec2(worldSize.x - 30.f, 30.f);
    emiter1.Speed.x = -speed;
    emiter1.emitNum = num;
    emiter1.intervel = interval;

    double accum = 0.;

    float dt = 1 / 280.0f;

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

        if (solver.objects.size() < 600000) {
            emiter.Emit(solver, dt);
            //emiter1.Emit(solver, dt);
        }
        
        solver.update(dt,threadPool);

        render.text.setString("FPS: "+std::to_string(1.0/ elapsed)+"\nNum: "+std::to_string(solver.objects.size()));

        window.clear();
        //render.render(window);
        render.renderMultiThread(window, threadPool);
        window.display();
    }

    return 0;
}