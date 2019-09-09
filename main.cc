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

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

static string request_domoticz(string id) {
  auto curl = curl_easy_init();
  if (curl) {
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

  cout << "CTRL-C for exit" << endl;

  fxMatrix->launchCycle();

  cout << "Deleting fxMatrix..." << endl;
  delete fxMatrix;

  return 0;
}
