#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using std::cout;

const float speed = 500.f; // pixels per second when moving ball and paddles
const double pi = std::acos(-1);
const int W = 800;
const int H = 600;

struct ball_type {
    ball_type(sf::Vector2f size, sf::Vector2f angle) : b(size), d(angle) {}
    ball_type(const ball_type &) = default;
    ball_type & operator=(const ball_type &) = default;

    template <typename T>
    void move(T x, T y) { b.move(x, y); }

    sf::RectangleShape b;
    sf::Vector2f d;
    float speed = 400;
};

template <typename T>
sf::IntRect to_rect(T const & shape) {
    return sf::IntRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

sf::IntRect to_rect(const ball_type & b) {
    return to_rect(b.b);
}

template <typename Shape>
int mid_y(const Shape & rect) {
    return rect.getPosition().y + rect.getSize().y / 2;
}

int mid_y(const ball_type & b) {
    return mid_y(b.b);
}

template <typename Shape>
int mid_x(const Shape & rect) {
    return rect.getPosition().x + rect.getSize().x / 2;
}

int mid_x(const ball_type & b) {
    return mid_x(b.b);
}

template <typename Shape>
int set_midpoint(Shape & rect, int x, int y) {
    rect.setPosition(x - rect.getSize().x / 2, y - rect.getSize().y / 2);
}

int set_midpoint(ball_type & b, int x, int y) {
    set_midpoint(b.b, x, y);
}

template <typename Shape>
int set_midpoint(Shape & rect, int y) {
    rect.setPosition(rect.getPosition().x, y - rect.getSize().y / 2);
}

int set_midpoint(ball_type & b, int y) {
    set_midpoint(b.b, y);
}

template <typename Ball, typename Shape>
int ai_update(const Ball & ball, const Shape & paddle) {
    if (mid_y(ball) > mid_y(paddle)) return 1;
    return -1;
}

int rng(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

template <typename Ball, typename Shape, typename Dir>
int ai_update(const Ball & ball, const Shape & paddle) {
    // find the angle of the ball
    auto tan_angle = ball.d.y / ball.d.x;

    // std::cout << "tan_angle is " << tan_angle << '\n';
    auto angle = std::atan(tan_angle);

    // std::cout << "angle is " << angle << '\n';

    // now find the adjacent (distance from ball to paddle)
    auto adj = mid_x(paddle) - mid_x(ball);
    // std::cout << "adj is " << adj << '\n';

    // calculate the opposite
    auto opp = std::tan(angle) * adj;
    // std::cout << "opp is " << opp << '\n';
    
    // std::cout << "mid_y(ball) is " << mid_y(ball) << '\n';
    int dest_y = mid_y(ball) + opp;
    // std::cout << "dest_y is " << dest_y << '\n';

    // if greater than 800, then calc d'
    if (dest_y > H) {
        dest_y = H - (dest_y - H);
        // std::cout << "dest_y' = " << dest_y << '\n';
    }
    // should just return dest_y and let move_paddle do the moving
    auto d = dest_y - mid_y(paddle);

    if (std::abs(d) <= 1) return 0;
    return (dest_y > mid_y(paddle)) ? 1 : -1;
}

void move_paddle(sf::RectangleShape & paddle, float distance) {
    auto y = mid_y(paddle) + distance;
    if (y < 100) set_midpoint(paddle, 100);
    else if (y > 500) set_midpoint(paddle, 500);
    else paddle.move(0, distance);
}

void bounce(const sf::IntRect & ball, const sf::IntRect & wall, sf::Vector2f & vec, sf::Sound & sound) {
    const bool topleft = wall.contains(ball.left, ball.top);
    const bool topright = wall.contains(ball.left + ball.width, ball.top);
    const bool botleft = wall.contains(ball.left, ball.top + ball.height);
    const bool botright = wall.contains(ball.left + ball.width, ball.top + ball.height);

    if (!topleft && !topright && !botleft && !botright) return;

    if ((topleft || botleft) && !topright && !botright) vec.x = std::abs(vec.x); // left of ball
    else if ((topright || botright) && !topleft && !botleft) vec.x = -std::abs(vec.x); // right of ball
    else if ((topleft || topright) && !botleft && !botright) vec.y = std::abs(vec.y); // top of ball
    else if ((botright || botleft) && !topleft && !topright) vec.y = -std::abs(vec.y);// bottom of ball
    
    // sound.play();
}

template <typename Ball, typename T>
void bounce(const Ball & ball, const T & wall, sf::Vector2f & vec, sf::Sound & sound) {
    bounce(to_rect(ball), to_rect(wall), vec, sound);
}

template <typename T, typename Action>
void score(const T & ball, Action action) {
    auto r = to_rect(ball);
    auto field = sf::IntRect(0, 0, W, H);
    if (!r.intersects(field)) action();
}

int main(int argc, char* argv[]) {

    try {
        // main window
        sf::RenderWindow rw(sf::VideoMode(W, H), "pong");
        rw.setFramerateLimit(120);

        // clock for determining time between frame displays
        sf::Clock clock;

        // top bar
        sf::RectangleShape top(sf::Vector2f(W, 10));
        auto c = sf::Color::White;
        c.a = 155;
        top.setFillColor(c);
        top.setPosition(0, 0);

        // bottom bar
        sf::RectangleShape bottom = top;
        bottom.setPosition(0, H - 10);

        // left paddle
        sf::RectangleShape left(sf::Vector2f(10, 80));
        set_midpoint(left, 50, 300);

        // right paddle
        sf::RectangleShape right = left;
        set_midpoint(right, W - 50, 500);

        // ball, direction, and sound
#if 1
        auto ball = ball_type {
            sf::Vector2f(15, 15), 
            sf::Vector2f(45 * 2 * pi / 360.f, 45 * 2 * pi / 360.f) }; // 45 degree angle;
#else 
        sf::RectangleShape ball(sf::Vector2f(15, 15));
#endif
        sf::Vector2f dx(45 * 2 * pi / 360.f, 45 * 2 * pi / 360.f); // 45 degree angle

        sf::SoundBuffer sound_buff;
        if (!sound_buff.loadFromFile("assets/ball.wav")) throw std::runtime_error("cannot open sound file");
        sf::Sound sound(sound_buff);
        set_midpoint(ball, 400, 20);


        // font 
        sf::Font font;
        if (!font.loadFromFile("assets/FreeMono.ttf")) throw std::runtime_error("cannot open font");
    
        clock.restart();
        float dt;
        float dt_count = 0;
        int frames = 0;
        bool paused;

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
                    case sf::Event::LostFocus:
                        paused = true;
                        break;
                    case sf::Event::GainedFocus:
                        paused = false;
                        break;
                }
            }
            if (!paused) {
                bounce(ball, bottom, ball.d, sound);
                bounce(ball, top, ball.d, sound);
                bounce(ball, left, ball.d, sound);
                bounce(ball, right, ball.d, sound);
#if 0
                ball.advance(dt);
#else
                ball.move(ball.d.x * speed * dt, ball.d.y * speed * dt);
#endif
                if (ball.d.x > 0) move_paddle(right, ai_update(ball, dx, right) * speed * dt);
                else move_paddle(left, ai_update(ball, dx, left) * speed * dt);

                score(ball, [&] { 
                    std::cout << "score!\n"; 
                    set_midpoint(ball, 400, rng(100, 500));
                });
            }


            rw.clear();
            rw.draw(top);
            rw.draw(bottom);
            rw.draw(left);
            rw.draw(right);
            rw.draw(ball.b);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
