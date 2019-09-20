#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <curl/curl.h>

#include "fx_matrix.h"
#include "rapidjson/document.h"     // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h" // for stringify JSON

using namespace rapidjson;

#define URL "http://192.168.86.125:8123/api/states"

/* number of threads to fire up in parallel */
#define NUM_THREADS 1

/* how many times each URL is transferred per thread */
#define URL_ITERATIONS 1

#define NUM_LOCKS CURL_LOCK_DATA_LAST
static pthread_mutex_t lockarray[NUM_LOCKS];

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data)
{
  (void)ptr;  /* unused */
  (void)data; /* unused */
  data->append((char*) ptr, size * nmemb);
  return (size_t)(size * nmemb);
}

static void lock_cb(CURL *handle, curl_lock_data data,
                    curl_lock_access access, void *userptr)
{
  (void)access;
  (void)userptr;
  (void)handle;
  pthread_mutex_lock(&lockarray[data]);
}

static void unlock_cb(CURL *handle, curl_lock_data data,
                      void *userptr)
{
  (void)userptr;
  (void)handle;
  pthread_mutex_unlock(&lockarray[data]);
}

static void init_locks(void)
{
  int i;

  for(i = 0; i< NUM_LOCKS; i++)
    pthread_mutex_init(&lockarray[i], NULL);
}

static void kill_locks(void)
{
  int i;

  for(i = 0; i < NUM_LOCKS; i++)
    pthread_mutex_destroy(&(lockarray[i]));
}

struct initurl {
  const char *url;
  CURLSH *share;
};

static std::string parse_data(const char* json) {
  Document document;
  if (document.Parse(json).HasParseError()) {
    std::cerr << "Error parsing data" << std::endl;
    return NULL;
  } else {
    for (auto it = document.Begin(); it != document.End(); ++it) {
      auto ta = it->FindMember("state");
      std::cout << ta->value.GetString() << std::endl;
    }
  }
  return NULL;
}

static void *run_thread(void *ptr)
{
  struct initurl *u = (struct initurl *)ptr;

  std::string response_string;
  CURL *curl = curl_easy_init();
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJkZjBkMmQxMTQ0Mzc0ODdkOTY3ZWRhODBhNDk1MzhkYyIsImlhdCI6MTU2ODMyNzMzNiwiZXhwIjoxODgzNjg3MzM2fQ.daKNq5gI9xcCmste6diZYPuTNbLQNGO2NOLiQyQ6gi0");
  curl_easy_setopt(curl, CURLOPT_URL, u->url);
  curl_easy_setopt(curl, CURLOPT_SHARE, u->share);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
  curl_easy_perform(curl); /* ignores error */
  curl_easy_cleanup(curl);
  fprintf(stderr, "Tread completed one\n");

  Document document;
  if (document.Parse(response_string.c_str()).HasParseError()) {
    std::cerr << "Error parsing data" << std::endl;
    return NULL;
  } else {
    for (auto it = document.Begin(); it != document.End(); ++it) {
      auto ta = it->FindMember("state");
      std::cout << ta->value.GetString() << std::endl;
    }
  }
  return NULL;
}

int main(void)
{
  pthread_t tid;
  int error;
  CURLSH *share;
  struct initurl url;

  /* Must initialize libcurl before any threads are started */
  curl_global_init(CURL_GLOBAL_ALL);

  share = curl_share_init();
  curl_share_setopt(share, CURLSHOPT_LOCKFUNC, lock_cb);
  curl_share_setopt(share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
  curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);

  init_locks();

  url.url = URL;
  url.share = share;
  for (int i = 0; i <= 5; ++i){
      error = pthread_create(&tid, NULL, run_thread, &url);
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number 0, errno %d\n", error);
    else
      fprintf(stderr, "Thread 0, gets %s\n", URL);
    
    error = pthread_join(tid, NULL);
    fprintf(stderr, "Thread 0 terminated\n");

    usleep(5000000);
  }

  kill_locks();

  curl_share_cleanup(share);
  curl_global_cleanup();
  return 0;
}

// #include "rapidjson/document.h"     // rapidjson's DOM-style API
// #include "rapidjson/prettywriter.h" // for stringify JSON

// #include "led-matrix.h"
// #include "graphics.h"
// #include "fx_matrix.h"

// #include <curl/curl.h>
// #include <string>
// #include <iostream>

// #include <getopt.h>
// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// #include <thread>

// using namespace rgb_matrix;
// using namespace rapidjson;
// using namespace std;

// #define NUM_LOCKS CURL_LOCK_DATA_LAST
// static pthread_mutex_t lockarray[NUM_LOCKS];

// static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
//     cout << "enter" << endl;
//     data->append((char*) ptr, size * nmemb);
//     return size * nmemb;
// }

// static void init_locks(void)
// {
//   int i;

//   for(i = 0; i< NUM_LOCKS; i++)
//     pthread_mutex_init(&lockarray[i], NULL);
// }

// static void kill_locks(void)
// {
//   int i;

//   for(i = 0; i < NUM_LOCKS; i++)
//     pthread_mutex_destroy(&(lockarray[i]));
// }

// static void lock_cb(CURL *handle, curl_lock_data data,
//                     curl_lock_access access, void *userptr)
// {
//   pthread_mutex_lock(&lockarray[data]);
// }

// static void unlock_cb(CURL *handle, curl_lock_data data,
//                       void *userptr)
// {
//   pthread_mutex_unlock(&lockarray[data]);
// }

// static string request_states(CURLSH *share) {
//   CURL* curl = curl_easy_init();
//   if (curl) {
//     struct curl_slist *chunk = NULL;
//     const char* url = "http://192.168.86.125:8123/api/states";
//     chunk = curl_slist_append(chunk, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJkZjBkMmQxMTQ0Mzc0ODdkOTY3ZWRhODBhNDk1MzhkYyIsImlhdCI6MTU2ODMyNzMzNiwiZXhwIjoxODgzNjg3MzM2fQ.daKNq5gI9xcCmste6diZYPuTNbLQNGO2NOLiQyQ6gi0");

//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
//     curl_easy_setopt(curl, CURLOPT_URL, url);
//     string response_string;

//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
//     curl_easy_setopt(curl, CURLOPT_SHARE, share);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

//     cout << "perform" << endl;
//     curl_easy_perform(curl);
//     cout << "cleanup" << endl;
//     curl_easy_cleanup(curl);
//     curl = NULL;

//     cout << "return" << endl;
//     return response_string;
//   }
//   cerr << "Error initializing curl" << endl;
//   return "Error";
// }

// static string parse_data(const char* json) {
//   Document document;
//   if (document.Parse(json).HasParseError()) {
//     cerr << "Error parsing data" << endl;
//     return NULL;
//   } else {
//     for (auto it = document.Begin(); it != document.End(); ++it) {
//       auto ta = it->FindMember("state");
//       std::cout << ta->value.GetString() << std::endl;
//     }
//   }
//   return NULL;
// }

// void get_data(CURLSH *share)
// {
// 	for (int i = 0; i <= 5; ++i){
//     string response = request_states(share);
//     parse_data(response.c_str());
//     usleep(2000000);
//   }
// }

// int main(int argc, char *argv[]) {

//   CURLSH *share;

//   share = curl_share_init();
//   curl_share_setopt(share, CURLSHOPT_LOCKFUNC, lock_cb);
//   curl_share_setopt(share, CURLSHOPT_UNLOCKFUNC, unlock_cb);
//   curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);

//   curl_global_init(CURL_GLOBAL_ALL);

//   init_locks();

//   std::thread t1(get_data, share);
	
//   FXMatrix *fxMatrix = new FXMatrix();

//   cout << "CTRL-C for exit" << endl;

//   fxMatrix->launchCycle();

//   cout << "Deleting fxMatrix..." << endl;
//   delete fxMatrix;
//   t1.join();

//   kill_locks();

//   curl_share_cleanup(share);
//   curl_global_cleanup();

//   return 0;
// }
