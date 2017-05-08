#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using std::cout;

const float pi = std::acos(-1);
const float W = 800.f;
const float H = 600.f;
const sf::Vector2f paddle_size(10, 80); 
const float bar_h = 10.f; // bar and paddle width
const sf::FloatRect playfield(0, bar_h, W, H - 2 * bar_h);

struct pong_rect : public sf::RectangleShape {
    pong_rect(sf::Vector2f size) : sf::RectangleShape(size) {}
    sf::Vector2f direction; // a unit vector
    float speed = 500.f;
    float dest_y;
};

using ball_type = pong_rect;
using paddle_type = pong_rect;

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

class centerline : public sf::Drawable, public sf::Transformable {
    public:
     centerline(const sf::Color & c) {
        // see fig. 5
        float n = 7.f;
        float bx = bar_h;
        float h = playfield.height;
        float by = 2 * h / (3 * n - 1);
        float d = by / 2;

		va.resize(4 * n);
        
        auto yn = 0.f;
        for (auto i = va.begin(); i != va.end(); i+=4) {
            auto v = &(*i);
            v[0].position = sf::Vector2f(0, yn);
            v[0].color = c;
            v[1].position = sf::Vector2f(bx, yn);
            v[1].color = c;
            v[2].position = sf::Vector2f(bx, yn + by);
            v[2].color = c;
            v[3].position = sf::Vector2f(0, yn + by);
            v[3].color = c;
            yn += by + d;
        }
    }

    virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const {
        states.transform *= getTransform();
        states.texture = &t;
        rt.draw(va.data(), va.size(), sf::Quads, states);
    }

    private:
    std::vector<sf::Vertex> va;
    sf::Texture t;
};

int rng(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

template <typename Drawable>
void ai_update(const Drawable & ball, paddle_type & paddle) {
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

    paddle.direction.y = paddle.dest_y > midpoint(paddle).x ? 1 : -1;
}

void move_paddle(paddle_type & paddle, float dt) {
    auto m = midpoint(paddle).y;
    if (paddle.dest_y < 100) paddle.dest_y = 100;
    else if (paddle.dest_y > 500) paddle.dest_y = 500;

    auto distance = paddle.dest_y - m;
    if (std::abs(distance) <= 2.f) return;

    auto sign = (paddle.dest_y > m) ? 1 : -1;
    paddle.move(0, sign * paddle.speed * dt);
}

template <typename T, typename Action>
void collide(pong_rect & ball, const T & wall, Action action) {
    auto b = to_rect(ball);
    auto w = to_rect(wall);
    const bool topleft = w.contains(b.left, b.top);
    const bool topright = w.contains(b.left + b.width, b.top);
    const bool botleft = w.contains(b.left, b.top + b.height);
    const bool botright = w.contains(b.left + b.width, b.top + b.height);

    if (!topleft && !topright && !botleft && !botright) return;

    if ((topleft || botleft) && !topright && !botright) {
        ball.direction.x = std::abs(ball.direction.x);
    }
    else if ((topright || botright) && !topleft && !botleft) {
        ball.direction.x = -std::abs(ball.direction.x);
    }
    else if ((topleft || topright) && !botleft && !botright) {
        ball.direction.y = std::abs(ball.direction.y);
    }
    else if ((botright || botleft) && !topleft && !topright) {
        ball.direction.y = -std::abs(ball.direction.y);
    }
    action();
}

template <typename T>
void collide(ball_type & ball, T wall) {
    collide(ball, wall, [&]{});
}

template <typename Drawable, typename Action>
void score(const Drawable & ball, Action action) {
    auto x = midpoint(ball).x;
    if (x < 0 || x > W) action();
}

void update_trajectory(ball_type & ball, const paddle_type & paddle) {
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

        centerline cl(c);
        cl.setPosition(W / 2 - bar_h / 2, bar_h);
        
        auto left = paddle_type{paddle_size};
        set_midpoint(left, 50, 300);

        auto right = left;
        set_midpoint(right, W - 50, 500);
        
        auto ball = ball_type { sf::Vector2f(15, 15) }; 
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
                update_trajectory(ball, right);
                ai_update(ball, left);
            });
            advance(ball, dt);
            move_paddle(right, dt);
            move_paddle(left, dt);

            score(ball, [&] { 
                set_midpoint(ball, 400, rng(100, 500));
                if (ball.direction.x > 0) {
                    std::cout << "score left!\n"; 
                    ai_update(ball, right);
                } else {
                    std::cout << "score right!\n"; 
                    ai_update(ball, left);
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
