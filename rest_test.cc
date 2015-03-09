#include <iostream>
#include <curl/curl.h>
#include <sys/time.h>
#include "lib/json-parser/json.h"

using namespace std;

string data; //will hold the url's contents

void performGet();

int main() {
    performGet();

    time_t now;
    time(&now);

    int activityArr[32]; // 0 = inactive, 1 = active, 2 = unknown
    for (int i = 0; i < 32; i++) {
        activityArr[i] = 2;
    }

    char const *input = data.c_str();
    json_value jsonParsedInput = * json_parse(input, strlen(input));

    for (int i = 0; i < jsonParsedInput.u.array.length; i++) {
        json_value *arrItem = jsonParsedInput.u.array.values[i];

        bool isActive = arrItem->u.object.values[0].value->u.boolean;
        string activityDateString = arrItem->u.object.values[1].value->u.string.ptr;
        activityDateString = activityDateString.substr(0, 19);                      // remove microseconds
        replace(activityDateString.begin(), activityDateString.end(), 'T', ' ');    // remove T in middle

        struct tm tm;
        strptime(activityDateString.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
        time_t activityDateSeconds = mktime(&tm) - 21600; // convert from UTC to MT

        int activitySecondsFromNow = now - activityDateSeconds;

        if (activitySecondsFromNow < 32) {
            activityArr[activitySecondsFromNow] = isActive;
        }
    }

    for (int i = 0; i < 32; i++) {
        cout << activityArr[i] << " ";
    }

    return 0;
}

size_t writeCallback(char *buf, size_t size, size_t nmemb, void *up) {
    //callback must have this declaration
    //buf is a pointer to the data that curl has for us
    //size*nmemb is the size of the buffer

    data.empty();

    for (int c = 0; c < size * nmemb; c++) {
        data.push_back(buf[c]);
    }
    return size * nmemb; //tell curl how many bytes we handled
}

void performGet() {
    CURL *curl; //our curl object

    curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    curl = curl_easy_init();

//    curl_easy_setopt(curl, CURLOPT_URL, "http://pingpongping.cfapps.io/activity");
    curl_easy_setopt(curl, CURLOPT_URL, "http://pingpongping.cfapps.io/activity");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}
