#include <SFML/Graphics.hpp>
#include <chrono>
#include "physics.h"
#include "render.h"
#include <string>

const float radius = 7.f;


int main() {
    const Vec2 worldSize{ 900,900 };
    // 创建一个窗口
    sf::RenderWindow window(sf::VideoMode(worldSize.x, worldSize.y), "Batch Render Colored Quads");
    window.setFramerateLimit(120);

    auto time_now = std::chrono::high_resolution_clock::now();
    auto time_last = time_now;

    PhysicSolver solver(worldSize, radius);

    Renderer render(solver, radius);
    solver.gravity.y = 500.f;
    Emiter emiter;
    emiter.Position = Vec2(25.f,25.f);
    emiter.Speed.x = 180.f;
    emiter.emitNum = 2;
    emiter.intervel = 0.02f;

    double accum = 0.;

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
            emiter.Emit(solver, elapsed);
        }
        
        solver.update(elapsed);

        render.text.setString("FPS: "+std::to_string(1.0/ elapsed)+"\nNum: "+std::to_string(solver.objects.size()));

        window.clear();
        render.render(window);
        window.display();
    }

    return 0;
}