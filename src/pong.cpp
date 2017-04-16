#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

const float speed = 500.f; // pixels per second when moving ball and paddles
const double pi = std::acos(-1);

template <typename T>
sf::IntRect to_rect(T const & shape) {
    return sf::IntRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
}

template <typename Shape>
int mid_y(const Shape & rect) {
    return rect.getPosition().y + rect.getSize().y / 2;
}

template <typename Shape>
int mid_x(const Shape & rect) {
    return rect.getPosition().x + rect.getSize().x / 2;
}

template <typename Shape>
int set_midpoint(Shape & rect, int x, int y) {
    rect.setPosition(x - rect.getSize().x / 2, y - rect.getSize().y / 2);
}

template <typename Shape>
int set_midpoint(Shape & rect, int y) {
    rect.setPosition(rect.getPosition().x, y - rect.getSize().y / 2);
}

template <typename Shape>
int ai_update(const Shape & ball, const Shape & paddle) {
    if (mid_y(ball) > mid_y(paddle)) return 1;
    return -1;
}

int rng(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

template <typename Shape, typename Dir>
int ai_update(const Shape & ball, const Dir & dir, const Shape & paddle) {
    // find the angle of the ball
    // std::cout << "dir is : " << dir.x << ", " << dir.y << '\n';
    auto tan_angle = dir.y / dir.x;

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
    auto dest_y = mid_y(ball) + opp;
    // std::cout << "dest_y is " << dest_y << '\n';

    dest_y %= 600;
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

void bounce(const sf::IntRect & ball, const sf::IntRect & wall, sf::Vector2f & dir, sf::Sound & sound) {
    const bool topleft = wall.contains(ball.left, ball.top);
    const bool topright = wall.contains(ball.left + ball.width, ball.top);
    const bool botleft = wall.contains(ball.left, ball.top + ball.height);
    const bool botright = wall.contains(ball.left + ball.width, ball.top + ball.height);

    if (!topleft && !topright && !botleft && !botright) return;

    if ((topleft || botleft) && !topright && !botright) dir.x = std::abs(dir.x); // left of ball
    else if ((topright || botright) && !topleft && !botleft) dir.x = -std::abs(dir.x); // right of ball
    else if ((topleft || topright) && !botleft && !botright) dir.y = std::abs(dir.y); // top of ball
    else if ((botright || botleft) && !topleft && !topright) dir.y = -std::abs(dir.y);// bottom of ball
    
    // sound.play();
}

template <typename T>
void bounce(const T & ball, const T & wall, sf::Vector2f & dir, sf::Sound & sound) {
    bounce(to_rect(ball), to_rect(wall), dir, sound);
}

template <typename T, typename Action>
void score(const T & ball, Action action) {
    auto r = to_rect(ball);
    auto field = sf::IntRect(0, 0, 800, 600);
    if (!r.intersects(field)) action();
}

int main(int argc, char* argv[]) {

    try {
        // main window
        sf::RenderWindow rw(sf::VideoMode(800, 600), "pong64");
        rw.setFramerateLimit(120);

        // clock for determining time between frame displays
        sf::Clock clock;

        // top bar
        sf::RectangleShape top(sf::Vector2f(800, 10));
        auto c = sf::Color::White;
        c.a = 155;
        top.setFillColor(c);
        top.setPosition(0, 0);

        // bottom bar
        sf::RectangleShape bottom = top;
        bottom.setPosition(0, 600 - 10);

        // left paddle
        sf::RectangleShape left(sf::Vector2f(10, 80));
        set_midpoint(left, 50, 300);

        // right paddle
        sf::RectangleShape right = left;
        set_midpoint(right, 800 - 50, 500);

        // ball, direction, and sound
        sf::RectangleShape ball(sf::Vector2f(15, 15));
        sf::Vector2f dir(45 * 2 * pi / 360.f, 45 * 2 * pi / 360.f); // 45 degree angle
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
                bounce(ball, bottom, dir, sound);
                bounce(ball, top, dir, sound);
                bounce(ball, left, dir, sound);
                bounce(ball, right, dir, sound);
                ball.move(dir.x * speed * dt, dir.y * speed * dt);
                if (dir.x > 0) move_paddle(right, ai_update(ball, dir, right) * speed * dt);
                else move_paddle(left, ai_update(ball, dir, left) * speed * dt);

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
            rw.draw(ball);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
