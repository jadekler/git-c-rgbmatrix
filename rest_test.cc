#include <iostream>
#include <string>
#include <curl/curl.h>
#include "lib/json-parser/json.h"

using namespace std;

string data; //will hold the url's contents

void performGet();

int main() {
    performGet();

    char const *input = data.c_str();

    json_value res = * json_parse(input, strlen(input));

    json_value *arr = res.u.array.values[0];

    for (int i = 0; i < res.u.array.length; i++) {
        json_value *arrItem = res.u.array.values[i];
        cout << arrItem->u.object.values[1].value->u.string.ptr << " : " << arrItem->u.object.values[0].value->u.boolean << endl;
    }

    return 0;
}

size_t writeCallback(char *buf, size_t size, size_t nmemb, void *up) { //callback must have this declaration
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

    curl_easy_setopt(curl, CURLOPT_URL, "http://pingpongping.cfapps.io/activity");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}
