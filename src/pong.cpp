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

//#define NO_SHAPE
struct pong_rect : public sf::RectangleShape {
    pong_rect(sf::Vector2f size) : sf::RectangleShape(size)
#ifndef NO_SHAPE
    , shape(*this) 
#endif
    {}
    pong_rect(const pong_rect &) = default;
    pong_rect & operator=(const pong_rect &) = default;
    sf::Vector2f direction = sf::Vector2f{pi / 4.f, pi / 4.f};
    float speed = 500.f;
    float dest_y;

#ifndef NO_SHAPE
    RectangleShape & shape;
#endif
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

sf::FloatRect to_rect(pong_rect & shape) {
    return sf::FloatRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

// TODO: idea: add a lambda parameter and then use this for score() and move_paddle()
void advance(pong_rect & b, float dt) {
    b.move(b.direction.x * b.speed * dt, b.direction.y * b.speed * dt);
}


// remove these later
float mid_y(const pong_rect & o) {
    return midpoint(o).y;
}

float mid_x(const pong_rect & o) {
    return midpoint(o).x;
}

template <typename Window>
void draw(Window & rw, const pong_rect & o) {
    rw.draw(o);
}

#if 1
using ball_type = pong_rect;
#else
struct ball_type {
    ball_type(sf::Vector2f size) : shape(size){}
    ball_type(const ball_type &) = default;
    ball_type & operator=(const ball_type &) = default;

    template <typename T>
    void move(T x, T y) { shape.move(x, y); }

    sf::RectangleShape shape;
    sf::Vector2f direction = sf::Vector2f{pi / 4.f, pi / 4.f};
    float speed = 500.f;
};
void advance(ball_type & b, float dt) {
    b.move(b.direction.x * b.speed * dt, b.direction.y * b.speed * dt);
}
#endif

#if 1
using paddle_type = pong_rect;
#else
struct paddle_type {
    paddle_type(sf::Vector2f size) : shape(size) { };
    paddle_type(const paddle_type &) = default;
    paddle_type & operator=(const paddle_type &) = default;
    sf::RectangleShape shape;
    float dest_y;
    float speed = 500.f;
};
#endif

class centerline : public sf::Drawable, public sf::Transformable {
    public:
     centerline(const sf::Color & c) {
        // fig. 5
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

template <typename Drawable>
sf::FloatRect to_rect(Drawable & d) {
    return sf::FloatRect(d.shape.getPosition().x, d.shape.getPosition().y, d.shape.getSize().x, d.shape.getSize().y);
}

// walls are still RectangleShapes
sf::FloatRect to_rect(sf::RectangleShape & shape) {
    return sf::FloatRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

template <typename Drawable>
float mid_y(const Drawable & d) {
    return d.shape.getPosition().y + d.shape.getSize().y / 2;
}

template <typename Drawable>
float mid_x(const Drawable & d) {
    return d.shape.getPosition().x + d.shape.getSize().x / 2;
}

template <typename Drawable>
float set_midpoint(Drawable & d, float x, float y) {
    d.shape.setPosition(x - d.shape.getSize().x / 2, y - d.shape.getSize().y / 2);
}

template <typename Drawable>
float set_midpoint(Drawable & d, float y) {
    d.shape.setPosition(d.shape.getPosition().x, y - d.shape.getSize().y / 2);
}

template <typename Window, typename Drawable>
void draw(Window & rw, const Drawable & d) {
    rw.draw(d.shape);
}

template <typename Window>
void draw(Window & rw, sf::RectangleShape & r) {
    rw.draw(r);
}

int rng(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

template <typename Drawable>
void ai_update(const Drawable & ball, paddle_type & paddle) {
    auto tan_angle = ball.direction.y / ball.direction.x; // tan(theta) = opp / adj
    auto adj = mid_x(paddle) - mid_x(ball); // find the adjacent (distance from ball to paddle on x)
    auto opp = tan_angle * adj; // calculate the opposite (distance on y)
    auto d = mid_y(ball) + opp; // destination of ball on the paddle's y axis

    d -= bar_h; // remove the bar height
    int h = playfield.height;
    int n = d / h;
    int dp = std::abs((int)d % h);
    dp += bar_h; // add it back in

    paddle.dest_y = (n % 2 == 0) ? dp : H - dp;

    // paddle.dest_y += rng(-paddle.shape.getSize().y / 2, paddle.shape.getSize().y / 2);  // a little variety
    paddle.dest_y += rng(-paddle.getSize().y / 2, paddle.getSize().y / 2);  // a little variety
}

void move_paddle(paddle_type & paddle, float dt) {
    auto m = mid_y(paddle);
    if (paddle.dest_y < 100) paddle.dest_y = 100;
    else if (paddle.dest_y > 500) paddle.dest_y = 500;

    auto distance = paddle.dest_y - m;
    if (std::abs(distance) <= 2.f) return;

    auto sign = (paddle.dest_y > m) ? 1 : -1;
    paddle.move(0, sign * paddle.speed * dt);
}

template <typename T, typename Action>
void collide(ball_type & ball, T wall, Action action) {
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
    auto x = mid_x(ball);
    if (x < 0 || x > W) action();
}

void update_trajectory(ball_type & ball, const paddle_type & paddle) {
    auto d = (mid_y(ball) - mid_y(paddle)) / (paddle.getSize().y / 2);
    auto sign = ball.direction.x < 0 ? -1 : 1;
    auto angle = d * pi / 3;  // 60 degrees
    ball.direction.x = std::cos(angle) * sign;
    ball.direction.y = std::sin(angle);
}

int main(int argc, char* argv[]) {

    try {
        // main window
        sf::RenderWindow rw{sf::VideoMode(W, H), "pong"};
        rw.setFramerateLimit(120);

        // clock for determining time between frame displays
        auto clock = sf::Clock();

        // top bar
        auto top = sf::RectangleShape{sf::Vector2f(W, bar_h)};
        auto c = sf::Color::White;
        c.a = 155;
        top.setFillColor(c);
        top.setPosition(0, 0);

        // bottom bar
        auto bottom = top;
        bottom.setPosition(0, H - bar_h);

        centerline cl(c);
        cl.setPosition(W / 2 - bar_h / 2, bar_h);
        
        // left paddle
        auto left = paddle_type{paddle_size};
        set_midpoint(left, 50, 300);

        // right paddle
        auto right = paddle_type{paddle_size};
        set_midpoint(right, W - 50, 500);
        
        // ball, direction, and sound
        auto ball = ball_type { sf::Vector2f(15, 15) }; 

        auto sound_buff = sf::SoundBuffer();
        if (!sound_buff.loadFromFile("assets/ball.wav")) throw std::runtime_error("cannot open sound file");
        sf::Sound sound(sound_buff);
        set_midpoint(ball, 400, 20);

        // font 
        auto font = sf::Font();
        if (!font.loadFromFile("assets/FreeMono.ttf")) throw std::runtime_error("cannot open font");
    
        clock.restart();
        float dt;

        ai_update(ball, right);

        // the loop
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
            draw(rw, top);
            draw(rw, bottom);
            draw(rw, left);
            draw(rw, right);
            draw(rw, ball);
            rw.draw(cl);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
