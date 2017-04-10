#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

template <typename T>
sf::IntRect to_rect(T const & shape) {
    return sf::IntRect(shape.getPosition().x, shape.getPosition().y, shape.getSize().x, shape.getSize().y);
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

int main(int argc, char* argv[]) {

    try {
        // main window
        sf::RenderWindow rw(sf::VideoMode(800, 600), "pong64");
        rw.setFramerateLimit(120);

        // clock for determining time between frame displays
        sf::Clock clock;
        const float speed = 500.f; // pixels per second when moving ball and paddles
        const double pi = std::acos(-1);

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
        sf::RectangleShape left(sf::Vector2f(10, 100));
        left.setFillColor(c);
        left.setPosition(50, 200);

        // right paddle
        sf::RectangleShape right = left;
        right.setPosition(800 - 50, 400);

        // ball, direction, and sound
        sf::RectangleShape ball(sf::Vector2f(15, 15));
        sf::Vector2f dir(45 * 2 * pi / 360.f, 45 * 2 * pi / 360.f); // 45 degree angle
        sf::SoundBuffer sound_buff;
        if (!sound_buff.loadFromFile("assets/ball.wav")) throw std::runtime_error("cannot open sound file");
        sf::Sound sound(sound_buff);
        ball.setPosition(100, 100);

        // font 
        sf::Font font;
        if (!font.loadFromFile("assets/FreeMono.ttf")) throw std::runtime_error("cannot open font");
    
        // fps text
        sf::Text fps_message("ready", font);
        fps_message.setPosition(730.f, 10.f);

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
#if 0
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { // down 
                if (!intersects(bottom, ball)) ball.move(0.f, velocity * dt);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) { // up
                if (!intersects(top, ball)) ball.move(0.f, -velocity * dt);
            }
#endif
            bounce(ball, bottom, dir, sound);
            bounce(ball, top, dir, sound);
            bounce(ball, left, dir, sound);
            bounce(ball, right, dir, sound);
            if (!paused) ball.move(dir.x * speed * dt, dir.y * speed * dt);

            ++frames;
            dt_count += dt;
            if (dt_count > .5f) {
                fps_message.setString(std::to_string(frames * 2));
                frames = 0;
                dt_count = 0;
            }

            rw.clear();
            rw.draw(top);
            rw.draw(bottom);
            rw.draw(left);
            rw.draw(right);
            rw.draw(ball);
            rw.draw(fps_message);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
