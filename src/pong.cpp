#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

const float pi = std::acos(-1);
const float W = 800.f;
const float H = 600.f;
const float bar_h = 10.f; // bar and paddle width
const sf::FloatRect playfield(0, bar_h, W, H - 2 * bar_h);

struct pong_rect : public sf::RectangleShape {
    pong_rect(sf::Vector2f size) : sf::RectangleShape(size) {}
    sf::Vector2f direction; // a unit vector
    float speed = 500.f;
    float dest_y;
};

sf::Vector2f midpoint(const pong_rect & o) {
    auto p = o.getPosition();
    auto s = o.getSize();
    return sf::Vector2f{p.x + s.x/2, p.y + s.y/2};
}

void set_midpoint(pong_rect & o, float x, float y) {
    auto p = o.getPosition();
    auto s = o.getSize();
    o.setPosition(x - s.x / 2, y - s.y / 2);
}

sf::FloatRect to_rect(const sf::RectangleShape & shape) {
    return sf::FloatRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

void advance(pong_rect & b, float dt) {
    b.move(b.direction.x * b.speed * dt, b.direction.y * b.speed * dt);
}

template <typename Action>
void advance(pong_rect & rect, float dt, Action action) {
    advance(rect, dt);
    action(rect);
}

struct centerline : public sf::Drawable, public sf::Transformable {
     centerline(float n, const sf::Color & c) {
        // see fig. 5
        float bx = bar_h;
        float h = playfield.height;
        float by = 2 * h / (3 * n - 1);
        float d = by / 2;

        auto yn = 0.f;
        for (auto i = 0; i < n; ++i) {
            va.emplace_back(sf::Vector2f(0, yn), c);
            va.emplace_back(sf::Vector2f(bx, yn), c);
            va.emplace_back(sf::Vector2f(bx, yn + by), c);
            va.emplace_back(sf::Vector2f(0, yn + by), c);
            yn += by + d;
        }
    }

    virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const {
        states.transform *= getTransform();
        rt.draw(va.data(), va.size(), sf::Quads, states);
    }

    std::vector<sf::Vertex> va;
};

int rng(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

void ai_update(const pong_rect & ball, pong_rect & paddle) {
    auto tan_angle = ball.direction.y / ball.direction.x; // tan(theta) = opp / adj
    auto adj = midpoint(paddle).x - midpoint(ball).x; // find the adjacent (distance from ball to paddle on x)
    auto opp = tan_angle * adj; // calculate the opposite (distance on y)
    auto d = midpoint(ball).y + opp; // destination of ball on the paddle's y axis

    d -= bar_h; // remove the bar height
    int h = playfield.height;
    int n = d / h;
    int dp = std::abs((int)d % h);
    dp += bar_h; // add it back in

    paddle.dest_y = (n % 2 == 0) ? dp : H - dp;

    paddle.dest_y += rng(-paddle.getSize().y / 2, paddle.getSize().y / 2);  // a little variety
    paddle.direction.y = paddle.dest_y > midpoint(paddle).y ? 1 : -1;
}

template <typename T, typename Action>
void collide(pong_rect & ball, const T & wall, Action action) {
    auto b = to_rect(ball);
    auto w = to_rect(wall);
    if (!b.intersects(w)) return;
    const bool topleft = w.contains(b.left, b.top);
    const bool topright = w.contains(b.left + b.width, b.top);
    const bool botleft = w.contains(b.left, b.top + b.height);
    const bool botright = w.contains(b.left + b.width, b.top + b.height);

    if ((topleft || botleft) && !topright && !botright) ball.direction.x = std::abs(ball.direction.x);
    else if ((topright || botright) && !topleft && !botleft) ball.direction.x = -std::abs(ball.direction.x);
    else if ((topleft || topright) && !botleft && !botright) ball.direction.y = std::abs(ball.direction.y);
    else if ((botright || botleft) && !topleft && !topright) ball.direction.y = -std::abs(ball.direction.y);
    action();
}

template <typename T>
void collide(pong_rect & ball, T wall) {
    collide(ball, wall, [&]{});
}

void check_paddle(pong_rect & paddle) {
    auto m = midpoint(paddle);
    if (m.y < 50) set_midpoint(paddle, m.x, 50);
    if (m.y > 550) set_midpoint(paddle, m.x, 550);
    if (m.y > paddle.dest_y && paddle.direction.y > 0) paddle.direction.y = 0;
    if (m.y < paddle.dest_y && paddle.direction.y < 0) paddle.direction.y = 0;
    // if (std::abs(paddle.dest_y - midpoint(paddle).y) <= 5.f) paddle.direction.y = 0;
}

void update_trajectory(pong_rect & ball, const pong_rect & paddle) {
    auto d = (midpoint(ball).y - midpoint(paddle).y) / (paddle.getSize().y / 2);
    auto sign = ball.direction.x < 0 ? -1 : 1;
    auto angle = d * pi / 3;  // 60 degrees
    ball.direction.x = std::cos(angle) * sign;
    ball.direction.y = std::sin(angle);
}

int main(int argc, char* argv[]) {

    try {
        sf::RenderWindow rw{sf::VideoMode(W, H), "pong"};
        rw.setFramerateLimit(120);

        auto clock = sf::Clock();

        auto top = sf::RectangleShape{sf::Vector2f(W, bar_h)};
        auto c = sf::Color::White;
        c.a = 155;
        top.setFillColor(c);
        top.setPosition(0, 0);

        auto bottom = top;
        bottom.setPosition(0, H - bar_h);

        centerline cl(12, c);
        cl.setPosition(W / 2 - bar_h / 2, bar_h);
        
        auto left = pong_rect{sf::Vector2f{10, 80}};
        set_midpoint(left, 50, 300);

        auto right = left;
        set_midpoint(right, W - 50, 500);
        
        auto ball = pong_rect { sf::Vector2f(15, 15) }; 
        ball.direction = sf::Vector2f{pi / 4.f, pi / 4.f};

        auto sound_buff = sf::SoundBuffer();
        if (!sound_buff.loadFromFile("assets/ball.wav")) throw std::runtime_error("cannot open sound file");
        sf::Sound sound(sound_buff);
        set_midpoint(ball, 400, 20);

        auto font = sf::Font();
        if (!font.loadFromFile("assets/FreeMono.ttf")) throw std::runtime_error("cannot open font");
    
        clock.restart();
        float dt;

        ai_update(ball, right);

        while (rw.isOpen()) {
            dt = clock.restart().asSeconds();

            sf::Event e;
            while (rw.pollEvent(e)) {

                switch (e.type) {
                    case sf::Event::Closed:
                        rw.close();	
                        break;

                        case sf::Event::KeyPressed:
                            switch (e.key.code) {
                                case sf::Keyboard::Escape :
                                    rw.close();
                                    break;
                            }
                        break;
                }
            }

            collide(ball, bottom);
            collide(ball, top);
            collide(ball, left, [&]{
                update_trajectory(ball, left);
                ai_update(ball, right);
            });
            collide(ball, right, [&]{
                if (ball.speed < 1000) ball.speed += 50;
                right.speed = left.speed = ball.speed;
                update_trajectory(ball, right);
                ai_update(ball, left);
            });

            advance(right, dt, check_paddle);
            advance(left, dt, check_paddle);

            advance(ball, dt, [&](pong_rect & ball){
                auto x = midpoint(ball).x;
                if (x < 0 || x > W) {
                    right.speed = left.speed = ball.speed = 500;
                    set_midpoint(ball, 400, rng(100, 500));
                    if (ball.direction.x > 0) {
                        std::cout << "score left!\n"; 
                        ai_update(ball, right);
                    } else {
                        std::cout << "score right!\n"; 
                        ai_update(ball, left);
                    }
                } 
            });

            rw.clear();
            rw.draw(top);
            rw.draw(bottom);
            rw.draw(left);
            rw.draw(right);
            rw.draw(ball);
            rw.draw(cl);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
