#include "led-matrix.h"
#include "graphics.h"
#include <iostream>

#define FONT_PATH "./lib/rpi-rgb-led-matrix/fonts/5x7.bdf"

using namespace rgb_matrix;

class FXMatrix {
  public:
    FXMatrix() {
      RGBMatrix::Options options;
      options.hardware_mapping = "adafruit-hat";
      options.rows = 32;
      options.cols = 64;
      options.chain_length = 1;
      options.parallel = 1;
      options.brightness = 100;
      options.pwm_bits = 11;

      rgb_matrix::RuntimeOptions runtimeOptions;
      runtimeOptions.gpio_slowdown = 2;
      runtimeOptions.daemon = 0;
      runtimeOptions.drop_privileges = 0;

      canvas = rgb_matrix::CreateMatrixFromOptions(options, runtimeOptions);
      offscreen_canvas = canvas->CreateFrameCanvas();

      if (!font.LoadFont(FONT_PATH)) {
        std::cout << "Couldn't load font " << FONT_PATH << std::endl;
      }

      color = Color(255, 255, 0);
      bgColor = Color(0, 0, 0);
    }

    ~FXMatrix() {
      canvas->Clear();
      delete canvas;
    }

  void drawNetatmo(std::string data) {
    int x = (canvas->width() / 4) - 15;
    rgb_matrix::DrawText(offscreen_canvas, font, x, 0 + font.baseline(), color, &bgColor, data.c_str(), 0);
  }

  void drawOWM(std::string data) {
    std::size_t pos = data.find(",");
    std::string temp = data.substr(0, pos);

    int x = (canvas->width() / 4) * 3 - 15;
    rgb_matrix::DrawText(offscreen_canvas, font, x, 0 + font.baseline(), color, &bgColor, temp.c_str(), 0);
  }

  void drawLinky(std::string data) {
    int y = canvas->height() / 2 + 3;
    rgb_matrix::DrawText(offscreen_canvas, font, 0, y + font.baseline(), color, &bgColor, data.c_str(), 0);
  }

  void displayData() {
    rgb_matrix::DrawLine(offscreen_canvas, 0, canvas->height() / 2, canvas->width(), canvas->height() / 2, Color(0, 0, 255));
    rgb_matrix::DrawLine(offscreen_canvas, canvas->width() / 2, 0, canvas->width() / 2, canvas->height() / 2, Color(0, 0, 255));

    offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
  }

  private:
    float home_temperature = 0;
    float temperature = 0;
    float energy = 0;

    RGBMatrix* canvas;
    FrameCanvas* offscreen_canvas;
    Font font;
    Color color;
    Color bgColor;

    bool parseColor(Color *c, const char *str) {
      return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
    }
};
