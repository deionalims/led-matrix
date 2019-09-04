#include "led-matrix.h"
#include "graphics.h"
#include <iostream>

#define FONT_PATH "/home/pi/Projects/rpi-rgb-led-matrix/fonts/5x7.bdf"

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

  void displayData(std::string data) {
    rgb_matrix::DrawText(canvas, font, 0, 0 + font.baseline(), color, &bgColor, data.c_str(), 0);
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
