/**********************************************************************************************
*
* LICENSE: MIT 
* 
* Copyright (c) 2024 Jove Beckett (@jovecodes)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
**********************************************************************************************/

#include "Jovial/Components/Components2D.h"
#include "Jovial/Components/Util/LazyAssets.h"
#include "Jovial/Components/Util/Screenshot.h"
#include "Jovial/Core/Logger.h"
#include "Jovial/Core/Node.h"
#include "Jovial/Core/Window.h"
#include "Jovial/Input/Actions.h"
#include "Jovial/JovialEngine.h"
#include "Jovial/Renderer/Renderer.h"
#include "Jovial/Renderer/TextRenderer.h"
#include "Jovial/Shapes/Circle.h"
#include "Jovial/Shapes/Color.h"
#include "Jovial/Shapes/ShapeDrawer.h"
#include "Jovial/Std/Vector2i.h"

using namespace jovial;

#define WINDOW_NAME "JPlotter (A plotter by Jove)"
#define WINDOW_SIZE Vector2(1280, 720)
#define WINDOW_RES Vector2(0, 0)

namespace Fonts {
    static LazyFont Normal(fs::Path(JV_FONTS_DIR JV_SEP "jet_brains.ttf"), 18.0);
    static LazyFont Title(fs::Path(JV_FONTS_DIR JV_SEP "jet_brains.ttf"), 25.0);
}// namespace Fonts

#define MARGINS 20.0f
#define LINE_WIDTH 2.0f
#define POINT_SIZE 5.0f

#define MAX_Y 100.0f
#define MIN_Y 0.0f
#define Y_SECTIONS 20
#define Y_LABEL "Corn"

#define MAX_X 50.0f
#define MIN_X 0.0f
#define X_SECTIONS 50
#define X_LABEL "Shakes"

#define TITLE "Corn Half Life Experiment"

void draw_vertical_text(Vector2 position, const char *text, Font *font, TextDrawProps props) {
    if (text[0] == '\0') return;

    float font_size = props.font_size;
    Color color = props.color;
    int z_index = props.z_index;

    if (font_size == 0) font_size = font->size;
    position.y -= font->size;

    Vector2 line_start = position;
    float scale = font_size / font->size;

    rendering::TextureDrawProperties properties;
    properties.color = color;
    properties.z_index = z_index;

    int char_index = 0;
    while (*text) {
        if (*text == '\n') {
            JV_CORE_TODO();
        }

        if (*text == '\t') {
            JV_CORE_TODO();
        }

        Vector2 pos = position;
        draw_char(props.effect, *text, char_index, font, scale, line_start, pos, properties);

        position.y -= font->size;

        ++text;
        ++char_index;
    }
}

class World : public Node {
public:
    World() = default;

    Rect2 get_screen_rect() {
        if (!screen_rect.is_equal_approx({})) {
            return screen_rect;
        }
        screen_rect = rendering::get_visable_rect();
        screen_rect = screen_rect.expand(-MARGINS);

        float left_pad = math::max(measure_text(to_string(MAX_Y), Fonts::Normal.get()).x,
                                   measure_text(to_string(MIN_Y), Fonts::Normal.get()).x);
        screen_rect.x += left_pad;

        Vector2 title_size = measure_text(TITLE, Fonts::Title.get());
        screen_rect.h -= title_size.y + MARGINS;

        screen_rect.y += Fonts::Title->size + MARGINS * 2;
        return screen_rect;
    }

    void draw_y_labels() {
        Rect2 screen_rect = get_screen_rect();

        float section_width = (screen_rect.h - screen_rect.y) / Y_SECTIONS;
        float section_amount = (MAX_Y - MIN_Y) / Y_SECTIONS;
        float current_amount = MIN_Y;
        for (int i = 0; i <= Y_SECTIONS; ++i) {
            float height = screen_rect.y + section_width * (float) i;
            Line line{{screen_rect.x - MARGINS / 2, height}, {screen_rect.x, height}};
            rendering::draw_line(line, LINE_WIDTH, {.color = Colors::Black});

            draw_text({screen_rect.x - MARGINS * 2.0f, height - Fonts::Normal->size / 4},
                      to_string((int) current_amount), Fonts::Normal.get(), {});

            current_amount += section_amount;
        }

        float y_label_size = measure_text(Y_LABEL, Fonts::Title.get()).x;
        Vector2 y_label_pos = {MARGINS, (float) Window::get_current_height() / 2.0f + y_label_size / 2.0f};
        draw_vertical_text(y_label_pos, Y_LABEL, Fonts::Title.get(), {});
    }

    void draw_x_labels() {
        Rect2 screen_rect = get_screen_rect();

        float section_width = (screen_rect.w - screen_rect.x) / X_SECTIONS;
        float section_amount = (MAX_X - MIN_X) / X_SECTIONS;
        float current_amount = MIN_X;
        for (int i = 0; i <= X_SECTIONS; ++i) {
            float width = screen_rect.x + section_width * (float) i;
            Line line{{width, screen_rect.y - MARGINS / 2}, {width, screen_rect.y}};
            rendering::draw_line(line, LINE_WIDTH, {.color = Colors::Black});

            String label = to_string((int) current_amount);
            float label_width = measure_text(label, Fonts::Normal.get()).x;

            draw_text({width - label_width / 1.5f, screen_rect.y - MARGINS * 1.5f},
                      label, Fonts::Normal.get(), {.fix_start_pos = true});

            current_amount += section_amount;
        }
        float size = measure_text(X_LABEL, Fonts::Title.get()).x;
        Vector2 x_label_pos = {((float) Window::get_current_width() - size) / 2.0f, MARGINS};
        draw_text(x_label_pos, X_LABEL, Fonts::Title.get(), {});
    }

    void draw_title() {
        Rect2 screen_rect = get_screen_rect();
        float size = measure_text(TITLE, Fonts::Title.get()).x;
        draw_text({((float) Window::get_current_width() - size) / 2.0f, screen_rect.h + MARGINS}, TITLE, Fonts::Title.get(), {});
        rendering::draw_rect2_outline(screen_rect, LINE_WIDTH, {.color = Colors::Black});
    }

    void draw_graph() {
        screen_rect = {};
        draw_title();
        draw_x_labels();
        draw_y_labels();
    }

    Vector2 point_to_world(Vector2i point) {
        Rect2 screen_rect = get_screen_rect();
        float section_height = (screen_rect.h - screen_rect.y) / MAX_Y;
        float section_width = (screen_rect.w - screen_rect.x) / MAX_X;
        return {
                screen_rect.x + (float) point.x * section_width,
                screen_rect.y + (float) point.y * section_height,
        };
    }

    void plot(const Vec<Vector2i> &points, Color color) {
        for (int i = 0; i < points.size(); ++i) {
            rendering::draw_circle(Circle(POINT_SIZE, point_to_world(points[i])), 16, {color});
            if (i != points.size() - 1) {
                rendering::draw_line({point_to_world(points[i]), point_to_world(points[i + 1])}, LINE_WIDTH, {color});
            }
        }
    }

    void update() override {
        draw_graph();

        static int data[] = {
                100, 94, 87, 82, 77, 73, 69, 64, 59, 56,
                48, 43, 38, 35, 30, 28, 25, 21, 19, 17,
                14, 12, 10, 10, 9, 9, 7, 6, 5, 4,
                4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3,
                2, 2, 1, 1, 1, 1, 1, 1, 1, 0};

        Vec<Vector2i> points;

        for (int i = 0; i < JV_ARRAY_LEN(data); ++i) {
            points.push_back({i, data[i]});
        }
        plot(points, Colors::Blue);

        points.clear();
        for (int i = 0; i < JV_ARRAY_LEN(data); ++i) {
            points.push_back({i, 100 - data[i]});
        }
        plot(points, Colors::Red);

        if (Input::is_just_pressed(Actions::F12)) {
            take_screenshot("../plot.png");
        }
    }

    Rect2 screen_rect = {};
};

int main() {
    Jovial game;

    game.push_plugin(Window::create({WINDOW_NAME, WINDOW_SIZE, WINDOW_RES, nullptr, Colors::White}));
    game.push_plugins(plugins::default_plugins_2d);
    game.push_plugin(new NodePlugin(new World));

    game.run();

    return 0;
}
