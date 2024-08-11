#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>
#include "physics.h"
#include "threadPool.h"


struct Renderer
{
    PhysicSolver& solver;

    sf::VertexArray world_va;
    sf::VertexArray objects_va;
    sf::Texture     object_texture;
    sf::Font font;
    sf::Text text;
    float radius;

    explicit
        Renderer(PhysicSolver& solver,float radius = 10.f)
        : solver(solver),radius(radius)
    {
        initializeWorldVA();

        object_texture.loadFromFile("../res/circle.png");
        object_texture.generateMipmap();
        object_texture.setSmooth(true);

        font.loadFromFile("C:/Windows/Fonts/simkai.ttf");

        text.setFont(font); // 设置字体
        text.setCharacterSize(15); // 设置字符大小（以像素为单位）
        text.setFillColor(sf::Color::White); // 设置文本颜色
        //text.setStyle(sf::Text::Bold | sf::Text::Underlined); // 设置文本样式
    }

    void render(sf::RenderWindow& window) {
        window.draw(&world_va[0], 4, sf::Quads);

        sf::RenderStates states;
        states.texture = &object_texture;

        // Particles
        updateParticlesVA();
        if (objects_va.getVertexCount() > 0)
            window.draw(&objects_va[0], objects_va.getVertexCount(), sf::Quads, states);

        renderHUD(window);
    }

    void renderMultiThread(sf::RenderWindow& window,tp::ThreadPool& tp) {
        window.draw(&world_va[0], 4, sf::Quads);

        sf::RenderStates states;
        states.texture = &object_texture;

        // Particles
        updateParticlesVAMultiThread(tp);
        //tp.waitForCompletion();
        if (objects_va.getVertexCount() > 0)
            window.draw(&objects_va[0], objects_va.getVertexCount(), sf::Quads, states);

        renderHUD(window);
    }

    void initializeWorldVA() {
        world_va.resize(4);
        world_va[0].position = { 0.0f               , 0.0f };
        world_va[1].position = { solver.world_size.x, 0.0f };
        world_va[2].position = { solver.world_size.x, solver.world_size.y };
        world_va[3].position = { 0.0f               , solver.world_size.y };

        const uint8_t level = 120;
        const sf::Color background_color{ level, level, level };
        world_va[0].color = background_color;
        world_va[1].color = background_color;
        world_va[2].color = background_color;
        world_va[3].color = background_color;
    }

    void updateParticlesVA() {
        objects_va.resize(solver.objects.size() * 4);

        const float texture_size = 1024.0f;

        for (uint32_t i = 0; i < solver.objects.size(); ++i) {
            const PhysicObject& object = solver.objects[i];
            const uint32_t idx = i << 2;
            objects_va[idx + 0].position = object.position + Vec2{ -radius, -radius };
            objects_va[idx + 1].position = object.position + Vec2{ radius, -radius };
            objects_va[idx + 2].position = object.position + Vec2{ radius,  radius };
            objects_va[idx + 3].position = object.position + Vec2{ -radius,  radius };
            objects_va[idx + 0].texCoords = { 0.0f        , 0.0f };
            objects_va[idx + 1].texCoords = { texture_size, 0.0f };
            objects_va[idx + 2].texCoords = { texture_size, texture_size };
            objects_va[idx + 3].texCoords = { 0.0f        , texture_size };

            const sf::Color color = object.color;
            objects_va[idx + 0].color = color;
            objects_va[idx + 1].color = color;
            objects_va[idx + 2].color = color;
            objects_va[idx + 3].color = color;
        }
    }

    void updateParticlesVAMultiThread(tp::ThreadPool& tp) {
        objects_va.resize(solver.objects.size() * 4);

        const float texture_size = 1024.0f;

        tp.dispatch(to<uint32_t>(solver.objects.size()), [this, texture_size](uint32_t start, uint32_t end) {
            for (uint32_t i = start; i < end; ++i) {
                const PhysicObject& object = solver.objects[i];
                const uint32_t idx = i << 2;
                objects_va[idx + 0].position = object.position + Vec2{ -radius, -radius };
                objects_va[idx + 1].position = object.position + Vec2{ radius, -radius };
                objects_va[idx + 2].position = object.position + Vec2{ radius,  radius };
                objects_va[idx + 3].position = object.position + Vec2{ -radius,  radius };
                objects_va[idx + 0].texCoords = { 0.0f        , 0.0f };
                objects_va[idx + 1].texCoords = { texture_size, 0.0f };
                objects_va[idx + 2].texCoords = { texture_size, texture_size };
                objects_va[idx + 3].texCoords = { 0.0f        , texture_size };

                const sf::Color color = object.color;
                objects_va[idx + 0].color = color;
                objects_va[idx + 1].color = color;
                objects_va[idx + 2].color = color;
                objects_va[idx + 3].color = color;
            }
        });
    }

    void renderHUD(sf::RenderWindow& window) {
        window.draw(text); // 绘制文本
    }
};