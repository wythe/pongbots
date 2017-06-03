#pragma once

#include "pong.h"

using V = sf::Vector2f; // 2d vector
using C = sf::Vector2f; // 2d coord
using Rect = pong_rect; // 2d coord plus width and height

float top_side(const R & r) { return r.top; }
float bottom_side(const R & r) { return r.top + r.height; }
float left_side(const R & r) { return r.left; }
float right_side(const R & r) { return r.left + r.width; }

// get the near and far distance between a and b
void distance(const R & a, const R & b, V & near, V & far) {
    if (a.left < b.left) {
        near.x = left_side(b) - right_side(a);
        far.x = right_side(b) - left_side(a);
    } else {
        near.x = left_side(a) - right_side(b);
        far.x = right_side(a) - left_side(b);
    }

    if (a.top < b.top) {
        near.y = top_side(b) - bottom_side(a);
        far.y = bottom_side(b) - top_side(a);
    } else {
        near.y = top_side(a) - bottom_side(b);
        far.y = bottom_side(a) - top_side(b);
    }
}

void collision_time(const Rect & a, const V & entry, const V & exit, V & t_entry, V & t_exit, float dt) {
    t_entry.x = t_entry.y = -std::numeric_limits<float>::infinity();
    t_exit.x = t_exit.y = std::numeric_limits<float>::infinity();

    auto v = velocity(a, dt);
    if (a.direction.x != 0.f) {
        t_entry.x = entry.x  / v.x;
        t_exit.x = exit.x  / v.x;
    }
    if (a.direction.y != 0.f) {
        t_entry.y = entry.y  / v.y;
        t_exit.y = exit.y  / v.y;
    }
}

bool detect_collision(const Rect & a, const Rect & b, float dt) {
    V entry, exit;
    distance(to_rect(a), to_rect(b), entry, exit);

    V t_entry, t_exit;
    collision_time(a, entry, exit, t_entry, t_exit, dt);

    auto entry_time = std::min(t_entry.x, t_entry.y);
    auto exit_time = std::max(t_exit.x, t_exit.y);

    if (entry_time < exit_time || t_entry.x < 0.f && t_entry.y < 0.f || t_entry.x > 1.f || t_entry.y > 1.f) {
        return false;
    }

    return true;
}


