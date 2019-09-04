#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON

#include "led-matrix.h"
#include "graphics.h"
#include "fx_matrix.h"

#include <curl/curl.h>
#include <string>
#include <iostream>

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace rgb_matrix;
using namespace rapidjson;
using namespace std;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

static string request_domoticz(string id) {
  auto curl = curl_easy_init();
  if (curl) {
    cout << "Requesting Domoticz" << endl;

    string url("http://192.168.86.125:8081/json.htm?type=devices&rid=");
    url.append(id);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    string response_string;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl = NULL;

    return response_string;
  }
  cerr << "Error initializing curl" << endl;
  return "Error";
}

static string request_netatmo() {
  return request_domoticz("2");
}

static string request_linky() {
  return request_domoticz("5");
}

static string request_open_weather_map() {
  return request_domoticz("6");
}

static string parse_data(const char* json) {
  Document document;
  if (document.Parse(json).HasParseError()) {
    cerr << "Error parsing data" << endl;
    return NULL;
  } else {
    const Value& result = document["result"][0];
    auto m = result.FindMember("Data");
    return m->value.GetString();
  }
}

int main(int argc, char *argv[]) {

  FXMatrix *fxMatrix = new FXMatrix();

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  cout << "CTRL-C for exit" << endl;

  while (!interrupt_received) { 

    string jsonNetatmo = request_netatmo();
    string temp = parse_data(jsonNetatmo.c_str());
    fxMatrix->drawNetatmo(temp);

    string jsonOWM = request_open_weather_map();
    string tempOWM = parse_data(jsonOWM.c_str());
    fxMatrix->drawOWM(tempOWM);
   
    string jsonLinky = request_linky();
    string linky = parse_data(jsonLinky.c_str());
    fxMatrix->drawLinky(linky);

    fxMatrix->displayData();

    usleep(10000000);
  }

  delete fxMatrix;

  return 0;
}
