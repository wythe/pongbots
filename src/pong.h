#pragma once

#include <SFML/Graphics.hpp>

using V = sf::Vector2f; // 2d vector
using R = sf::FloatRect; // rectangle

struct pong_rect : public sf::RectangleShape {
    pong_rect(sf::Vector2f size) : sf::RectangleShape(size) {}
    sf::Vector2f direction; // a unit vector
    float speed = 1000.f;
    float dest_y;
};

sf::FloatRect to_rect(const sf::RectangleShape & shape) {
    return sf::FloatRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

V velocity(const pong_rect & a, float dt) {
    return a.direction * a.speed * dt;
}

