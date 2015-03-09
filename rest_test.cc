#include <iostream>
#include <curl/curl.h>
#include <sys/time.h>
#include <unistd.h>
#include "lib/json-parser/json.h"

using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string performGet() {
    CURL *curl; //our curl object

    curl_global_init(CURL_GLOBAL_ALL); //pretty obvious
    curl = curl_easy_init();

    std::string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, "http://pingpongping.cfapps.io/activity");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return readBuffer;
}

int main() {
    for (;;) {
        string data = performGet();

        time_t now;
        time(&now);

        int activityArr[32]; // 0 = inactive, 1 = active, 2 = unknown
        for (int i = 0; i < 32; i++) {
            activityArr[i] = 2;
        }

        char const *input = data.c_str();
        json_value jsonParsedInput;

        jsonParsedInput = *json_parse(input, strlen(input));

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

            int lag = 5; // 5 seconds lag
            if (activitySecondsFromNow >= 0 && activitySecondsFromNow < 32 + lag) {
                activityArr[activitySecondsFromNow - lag] = isActive;
            }
        }

        for (int i = 0; i < 32; i++) {
            cout << activityArr[i] << " ";
        }

        cout << endl;

        usleep(1000000);
    }
}
