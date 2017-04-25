#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using std::cout;

// The Drawable concept
// If concepts were supported in the compiler, this is what it would look something like this:
#if 0 
template <class T>
concept bool Drawable() {
    return requires(T a) {
        a.shape -> sf::RectangleShape;
    };
}
#endif

const float speed = 500.f; // pixels per second when moving paddles
const double pi = std::acos(-1);
const int W = 800;
const int H = 600;

enum side {
    left, right, top, bottom
};

struct ball_type {
    ball_type(sf::Vector2f size, sf::Vector2f angle) : shape(size), d(angle) {}
    ball_type(const ball_type &) = default;
    ball_type & operator=(const ball_type &) = default;

    template <typename T>
    void move(T x, T y) { shape.move(x, y); }

    sf::RectangleShape shape;
    sf::Vector2f d; // direction
    float s = 500.f; // speed
};

struct paddle_type {
    paddle_type(sf::Vector2f size) : shape(size) {};
    paddle_type(const paddle_type &) = default;
    paddle_type & operator=(const paddle_type &) = default;
    sf::RectangleShape shape;
    int dest_y;
};

void advance(ball_type & b, float dt) {
    b.move(b.d.x * b.s * dt, b.d.y * b.s * dt);
}

template <typename Drawable>
sf::IntRect to_rect(Drawable & d) {
    return sf::IntRect(d.shape.getPosition().x, d.shape.getPosition().y, d.shape.getSize().x, d.shape.getSize().y);
}

// walls are still RectangleShapes
sf::IntRect to_rect(sf::RectangleShape & shape) {
    return sf::IntRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

template <typename Drawable>
int mid_y(const Drawable & d) {
    return d.shape.getPosition().y + d.shape.getSize().y / 2;
}

template <typename Drawable>
int mid_x(const Drawable & d) {
    return d.shape.getPosition().x + d.shape.getSize().x / 2;
}

template <typename Drawable>
int set_midpoint(Drawable & d, int x, int y) {
    d.shape.setPosition(x - d.shape.getSize().x / 2, y - d.shape.getSize().y / 2);
}

template <typename Drawable>
int set_midpoint(Drawable & d, int y) {
    d.shape.setPosition(d.shape.getPosition().x, y - d.shape.getSize().y / 2);
}

template <typename Window, typename Drawable>
void draw(Window & rw, Drawable & d) {
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
int ai_update(const Drawable & ball, paddle_type & paddle) {
    // find the angle of the ball
    auto tan_angle = ball.d.y / ball.d.x;

    auto angle = std::atan(tan_angle);

    auto adj = mid_x(paddle) - mid_x(ball); // now find the adjacent (distance from ball to paddle)
    auto opp = std::tan(angle) * adj; // calculate the opposite
    paddle.dest_y = mid_y(ball) + opp; // destination of ball on the paddle's y axis

    // if greater than 800, then calc d'
    if (paddle.dest_y > H) paddle.dest_y = H - (paddle.dest_y - H);
    else if (paddle.dest_y < 0) paddle.dest_y = -paddle.dest_y;
}

void move_paddle(paddle_type & paddle, float speed, float dt) {
    auto m = mid_y(paddle);
    if (paddle.dest_y < 100) paddle.dest_y = 100;
    else if (paddle.dest_y > 500) paddle.dest_y = 500;

    auto distance = paddle.dest_y - m;
    if (std::abs(distance) <= 5) return;

    auto sign = (paddle.dest_y > m) ? 1 : -1;
    paddle.shape.move(0, sign * speed * dt);
}

void move_paddle(paddle_type & d, float distance) {
    auto y = mid_y(d) + distance;
    if (y < 100) set_midpoint(d, 100);
    else if (y > 500) set_midpoint(d, 500);
    else d.shape.move(0, distance);
}

template <typename T, typename Action>
void collide(const ball_type & ball, T wall, Action action) {
    auto b = to_rect(ball);
    auto w = to_rect(wall);
    const bool topleft = w.contains(b.left, b.top);
    const bool topright = w.contains(b.left + b.width, b.top);
    const bool botleft = w.contains(b.left, b.top + b.height);
    const bool botright = w.contains(b.left + b.width, b.top + b.height);

    if (!topleft && !topright && !botleft && !botright) return;

    if ((topleft || botleft) && !topright && !botright) action(side::left);
    else if ((topright || botright) && !topleft && !botleft) action(side::right);
    else if ((topleft || topright) && !botleft && !botright) action(side::top);
    else if ((botright || botleft) && !topleft && !topright) action(side::bottom);
}

template <typename Drawable, typename Action>
void score(const Drawable & ball, Action action) {
    auto r = to_rect(ball);
    auto field = sf::IntRect(0, 0, W, H);
    if (!r.intersects(field)) action();
}

int main(int argc, char* argv[]) {

    try {
        // main window
        sf::RenderWindow rw{sf::VideoMode(W, H), "pong"};
        rw.setFramerateLimit(120);

        // clock for determining time between frame displays
        auto clock = sf::Clock();

        // top bar
        auto top = sf::RectangleShape{sf::Vector2f(W, 10)};
        auto c = sf::Color::White;
        c.a = 155;
        top.setFillColor(c);
        top.setPosition(0, 0);

        // bottom bar
        auto bottom = top;
        bottom.setPosition(0, H - 10);

        // left paddle
        auto left = paddle_type{sf::Vector2f(10, 80)};
        set_midpoint(left, 50, 300);

        // right paddle
        auto right = left;
        set_midpoint(right, W - 50, 500);
        
        // ball, direction, and sound
        auto ball = ball_type {
            sf::Vector2f(15, 15), 
            sf::Vector2f(45 * 2 * pi / 360.f, 45 * 2 * pi / 360.f) }; // 45 degree angle;

        auto sound_buff = sf::SoundBuffer();
        if (!sound_buff.loadFromFile("assets/ball.wav")) throw std::runtime_error("cannot open sound file");
        sf::Sound sound(sound_buff);
        set_midpoint(ball, 400, 20);

        // font 
        auto font = sf::Font();
        if (!font.loadFromFile("assets/FreeMono.ttf")) throw std::runtime_error("cannot open font");
    
        clock.restart();
        float dt;
        float dt_count = 0;
        int frames = 0;

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

            collide(ball, bottom, [&](auto) {
                ball.d.y = -std::abs(ball.d.y);
            });
            collide(ball, top, [&](auto) {
                ball.d.y = std::abs(ball.d.y);
            });
            collide(ball, left, [&](auto) {
                ball.d.x = std::abs(ball.d.x);
                ai_update(ball, right);
            });
            collide(ball, right, [&](auto) {
                ball.d.x = -std::abs(ball.d.x);
                ai_update(ball, left);
            });
            advance(ball, dt);
            move_paddle(right, speed, dt);
            move_paddle(left, speed, dt);

            score(ball, [&] { 
                set_midpoint(ball, 400, rng(100, 500));
                if (ball.d.x > 0) {
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
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
