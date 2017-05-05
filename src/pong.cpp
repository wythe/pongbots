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
const sf::Vector2f paddle_size(10, 80); 
const int bar_h = 10; // bar and paddle width
const sf::IntRect playfield(0, bar_h, W, H - 2 * bar_h);

enum struct face {
    left, right, top, bottom
};

enum struct side {
    left, right
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
    paddle_type(sf::Vector2f size) : shape(size) {
        };
    paddle_type(const paddle_type &) = default;
    paddle_type & operator=(const paddle_type &) = default;
    sf::RectangleShape shape;
    int dest_y;
};

class centerline : public sf::Drawable, public sf::Transformable {
    public:
     centerline(const sf::Color & c) {
        // fig. 5
        int n = 5;
        int bx = bar_h;
        int h = playfield.height;

        int by = 2 * h / (3 * n - 1);
        int d = by / 2;

		va.resize(4 * n);
        cout << "n = " << n << ", by = " << by << ", d = " << d << '\n';
        
        auto yn = 0;
        for (auto i = va.begin(); i != va.end(); i+=4) {
            auto v = &(*i);
            cout << "yn = " << yn << '\n';
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

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        states.transform *= getTransform();
        states.texture = &t;
        target.draw(va.data(), va.size(), sf::Quads, states);
    }

    private:
    std::vector<sf::Vertex> va;
    sf::Texture t;
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
int ai_update(const Drawable & ball, paddle_type & paddle, Drawable & target) {
    auto tan_angle = ball.d.y / ball.d.x; // tan(theta) = opp / adj
    auto adj = mid_x(paddle) - mid_x(ball); // find the adjacent (distance from ball to paddle on x)
    auto opp = tan_angle * adj; // calculate the opposite (distance on y)
    int d = mid_y(ball) + opp; // destination of ball on the paddle's y axis

    d -= bar_h; // remove the bar height
    int h = playfield.height;
    int n = d / h;
    int dp = std::abs(d % h);
    dp += bar_h; // add it back in

    paddle.dest_y = (n % 2 == 0) ? dp : H - dp;

    set_midpoint(target, mid_x(paddle), paddle.dest_y);

    paddle.dest_y += rng(-paddle.shape.getSize().y / 2, paddle.shape.getSize().y / 2);  // a little variety
}

void move_paddle(paddle_type & paddle, float speed, float dt) {
    auto m = mid_y(paddle);
    if (paddle.dest_y < 100) paddle.dest_y = 100;
    else if (paddle.dest_y > 500) paddle.dest_y = 500;

    auto distance = paddle.dest_y - m;
    if (std::abs(distance) <= 1) return;

    auto sign = (paddle.dest_y > m) ? 1 : -1;
    paddle.shape.move(0, sign * speed * dt);
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

    if ((topleft || botleft) && !topright && !botright) action(face::left);
    else if ((topright || botright) && !topleft && !botleft) action(face::right);
    else if ((topleft || topright) && !botleft && !botright) action(face::top);
    else if ((botright || botleft) && !topleft && !topright) action(face::bottom);
}

template <typename Drawable, typename Action>
void score(const Drawable & ball, Action action) {
    auto x = mid_x(ball);
    if (x < 0 || x > W) action();
}

void update_trajectory(ball_type & ball, const paddle_type & paddle) {
    auto d = (mid_y(ball) - mid_y(paddle)) / (paddle.shape.getSize().y / 2);
    auto angle = d * pi / 3;  // 60 degrees
    ball.d.x = std::cos(angle);
    ball.d.y = std::sin(angle);
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
        
#if 0 // test triangle with vertex array
		sf::VertexArray triangle(sf::Triangles, 3);

		// define the position of the triangle's points
		triangle[0].position = sf::Vector2f(10, 10);
		triangle[1].position = sf::Vector2f(100, 10);
		triangle[2].position = sf::Vector2f(100, 100);

		// define the color of the triangle's points
		triangle[0].color = c;
		triangle[1].color = c;
		triangle[2].color = c;
#endif
        // left paddle
        auto left = paddle_type{paddle_size};
        set_midpoint(left, 50, 300);

        // right paddle
        auto right = left;
        set_midpoint(right, W - 50, 500);
        
        // ball, direction, and sound
        auto ball = ball_type { sf::Vector2f(15, 15), sf::Vector2f(pi / 4.f, pi / 4.f) }; // 45 degree angle;

        auto target = ball;

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

        ai_update(ball, right, target);

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

            collide(ball, bottom, [&](auto) { ball.d.y = -std::abs(ball.d.y); });
            collide(ball, top, [&](auto) { ball.d.y = std::abs(ball.d.y); });
            collide(ball, left, [&](auto) {
                update_trajectory(ball, left);
                ball.d.x = std::abs(ball.d.x);
                ai_update(ball, right, target);
            });
            collide(ball, right, [&](auto) {
                update_trajectory(ball, right);
                ball.d.x = -std::abs(ball.d.x);
                ai_update(ball, left, target);
            });
            advance(ball, dt);
            move_paddle(right, speed, dt);
            move_paddle(left, speed, dt);

            score(ball, [&] { 
                set_midpoint(ball, 400, rng(100, 500));
                if (ball.d.x > 0) {
                    std::cout << "score left!\n"; 
                    ai_update(ball, right, target);
                } else {
                    std::cout << "score right!\n"; 
                    ai_update(ball, left, target);
                }
            });

            rw.clear();
            draw(rw, top);
            draw(rw, bottom);
            draw(rw, left);
            draw(rw, right);
            draw(rw, ball);
            draw(rw, target);
			// rw.draw(triangle);
            rw.draw(cl);
            rw.display();
        }
    } catch (std::exception & e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
