#include "led-matrix.h"
#include "graphics.h"

#include <iostream>
#include <Magick++.h>

#define FONT_PATH "./lib/rpi-rgb-led-matrix/fonts/5x7.bdf"
#define HOME_IMG "./imgs/home.png"
#define WEATHER_IMG "./imgs/10d.png"

using namespace rgb_matrix;
using namespace Magick;

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

      color = rgb_matrix::Color(255, 255, 0);
      bgColor = rgb_matrix::Color(0, 0, 0);

      Magick::InitializeMagick(NULL);
    }

    ~FXMatrix() {
      canvas->Clear();
      delete canvas;
    }

  void drawNetatmo(std::string data) {
    int x = 11;
    rgb_matrix::DrawText(offscreen_canvas, font, x, 10, color, &bgColor, data.c_str(), 0);

    Image home(HOME_IMG);
    for (int i = 0; i < 10; ++i) {
      for (int j = 0; j < 10; ++j) {
        ColorRGB rgb(home.pixelColor(i, j));
        offscreen_canvas->SetPixel(i, j, ScaleQuantumToChar(rgb.redQuantum()),
                                   ScaleQuantumToChar(rgb.greenQuantum()),
                                   ScaleQuantumToChar(rgb.blueQuantum()));
      }
    }
  }

  void drawOWM(std::string data) {
    std::size_t pos = data.find(",");
    std::string temp = data.substr(0, pos);

    int x = 11;
    int y = canvas->height() / 2 + font.baseline() / 2;;
    rgb_matrix::DrawText(offscreen_canvas, font, x, y, color, &bgColor, temp.c_str(), 0);

    Image weather(WEATHER_IMG);
    for (int i = 0; i < 10; ++i) {
      for (int j = 0; j < 10; ++j) {
        ColorRGB rgb(weather.pixelColor(i, j));
        offscreen_canvas->SetPixel(i, j + y, ScaleQuantumToChar(rgb.redQuantum()),
                                   ScaleQuantumToChar(rgb.greenQuantum()),
                                   ScaleQuantumToChar(rgb.blueQuantum()));
      }
    }
  }

  void drawLinky(std::string data) {
    int x = 11;
    int y = 32;
    rgb_matrix::DrawText(offscreen_canvas, font, x, y, color, &bgColor, data.c_str(), 0);
  }

  void displayData() {
    offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
  }

  private:
    float home_temperature = 0;
    float temperature = 0;
    float energy = 0;

    RGBMatrix* canvas;
    FrameCanvas* offscreen_canvas;
    Font font;
    rgb_matrix::Color color;
    rgb_matrix::Color bgColor;

    bool parseColor(rgb_matrix::Color *c, const char *str) {
      return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
    }
};
