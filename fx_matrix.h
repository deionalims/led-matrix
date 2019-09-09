#include "led-matrix.h"
#include "graphics.h"
#include "content-streamer.h"
#include "utf8-internal.h"

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <iostream>
#include <Magick++.h>

#define FONT_PATH "./lib/rpi-rgb-led-matrix/fonts/5x7.bdf"
#define HOME_IMG "./imgs/home.png"
#define WEATHER_IMG "./imgs/10d.png"
#define MARIO "./imgs/mario.gif"

using rgb_matrix::GPIO;
using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamReader;
using namespace Magick;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

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

      LoadMarioImageAndStoreInStream();

      signal(SIGTERM, InterruptHandler);
      signal(SIGINT, InterruptHandler);
    }

    ~FXMatrix() {
      canvas->Clear();
      delete canvas;
    }

  void launchCycle() {
    while(!interrupt_received) {
      DisplayClockAnimation(mario_stream);
    }
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
    rgb_matrix::Font font;
    rgb_matrix::Color color;
    rgb_matrix::Color bgColor;

    rgb_matrix::StreamIO *mario_stream = new rgb_matrix::MemStreamIO();

    bool parseColor(rgb_matrix::Color *c, const char *str) {
      return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
    }

    void StoreInStream(const Magick::Image &img, int delay_time_us,
                          int x_offset,
                          int y_offset,
                          rgb_matrix::FrameCanvas *scratch,
                          rgb_matrix::StreamWriter *output) {
      scratch->Clear();
      for (size_t y = 0; y < img.rows(); ++y) {
        for (size_t x = 0; x < img.columns(); ++x) {
          const Magick::Color &c = img.pixelColor(x, y);
          if (c.alphaQuantum() < 256) {
            scratch->SetPixel(x + x_offset, y + y_offset,
                              ScaleQuantumToChar(c.redQuantum()),
                              ScaleQuantumToChar(c.greenQuantum()),
                              ScaleQuantumToChar(c.blueQuantum()));
          }
        }
      }
      output->Stream(*scratch, delay_time_us);
    }

    // Load still image or animation.
    // Scale, so that it fits in "width" and "height" and store in "result".
    bool LoadImageAndScale(const char *filename,
                            int target_width, int target_height,
                            bool fill_width, bool fill_height,
                            std::vector<Magick::Image> *result,
                            std::string *err_msg) {
      std::vector<Magick::Image> frames;
      try {
        readImages(&frames, filename);
      } catch (std::exception& e) {
        if (e.what()) *err_msg = e.what();
        return false;
      }
      if (frames.size() == 0) {
        fprintf(stderr, "No image found.");
        return false;
      }

      // Put together the animation from single frames. GIFs can have nasty
      // disposal modes, but they are handled nicely by coalesceImages()
      if (frames.size() > 1) {
        Magick::coalesceImages(result, frames.begin(), frames.end());
      } else {
        result->push_back(frames[0]);   // just a single still image.
      }

      const int img_width = (*result)[0].columns();
      const int img_height = (*result)[0].rows();
      const float width_fraction = (float)target_width / img_width;
      const float height_fraction = (float)target_height / img_height;
      if (fill_width && fill_height) {
        // Scrolling diagonally. Fill as much as we can get in available space.
        // Largest scale fraction determines that.
        const float larger_fraction = (width_fraction > height_fraction)
          ? width_fraction
          : height_fraction;
        target_width = (int) roundf(larger_fraction * img_width);
        target_height = (int) roundf(larger_fraction * img_height);
      }
      else if (fill_height) {
        // Horizontal scrolling: Make things fit in vertical space.
        // While the height constraint stays the same, we can expand to full
        // width as we scroll along that axis.
        target_width = (int) roundf(height_fraction * img_width);
      }
      else if (fill_width) {
        // dito, vertical. Make things fit in horizontal space.
        target_height = (int) roundf(width_fraction * img_height);
      }

      for (size_t i = 0; i < result->size(); ++i) {
        (*result)[i].scale(Magick::Geometry(target_width, target_height));
      }

      return true;
    }

    void LoadMarioImageAndStoreInStream() {
      std::vector<Magick::Image> image_sequence;
      std::string err_msg;
      if (LoadImageAndScale(MARIO, canvas->width(), canvas->height(),
                            false, true, &image_sequence, &err_msg)) {
        rgb_matrix::StreamWriter out(mario_stream);
        const Magick::Image &img_tmp = image_sequence[0];
        int x = 0 - img_tmp.columns();
        size_t img_idx = 0;
        while (x < canvas->width()) {
          const Magick::Image &img = image_sequence[img_idx++];
          StoreInStream(img, 100 * 1000, x, 0, offscreen_canvas, &out);
          x++;
          img_idx = img_idx == image_sequence.size() ? 0 : img_idx;
        }
      }
    }

    void DisplayClockAnimation(rgb_matrix::StreamIO *stream) {

      char text_buffer[16];
      struct timespec next_time;
      next_time.tv_sec = time(NULL);
      next_time.tv_nsec = 0;
      struct tm tm;
      localtime_r(&next_time.tv_sec, &tm);
      strftime(text_buffer, sizeof(text_buffer), "%H:%M", &tm);
      int clock_length = 0;
      const char* toto = text_buffer;
      while (*toto) {
        const uint32_t cp = utf8_next_codepoint(toto);
        clock_length += font.CharacterWidth(cp);
      }

      int x_anchor = canvas->width() / 2 - clock_length / 2;
      int y_anchor = canvas->height() / 2 + font.height() / 2;
      int x = 0 - clock_length - 32;

      rgb_matrix::StreamReader reader(stream);
      while (reader.GetNext(offscreen_canvas, 0)) {
        rgb_matrix::DrawText(offscreen_canvas, font, x, y_anchor, color, &bgColor, text_buffer, 0);

        x++;
        if (x >= x_anchor)
          x = x_anchor;

        offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
        usleep(75000);
      }

      usleep(10000000);


      while (y_anchor > -2) {
        offscreen_canvas->Clear();
        rgb_matrix::DrawText(offscreen_canvas, font, x, y_anchor, color, &bgColor, text_buffer, 0);

        y_anchor--;

        offscreen_canvas = canvas->SwapOnVSync(offscreen_canvas);
        usleep(75000);
      }

        offscreen_canvas->Clear();
    }
};
