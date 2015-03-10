// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include <iostream>
#include <curl/curl.h>
#include <sys/time.h>
#include <unistd.h>
#include "lib/json-parser/json.h"

using namespace rgb_matrix;

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

class VolumeBars : public ThreadedCanvasManipulator {
public:
    VolumeBars(Canvas *m, int delay_ms = 50, int numBars = 8)
            : ThreadedCanvasManipulator(m), delay_ms_(delay_ms),
              numBars_(numBars), t_(0) {
    }

    ~VolumeBars() {
        delete[] barHeights_;
    }

    void Run() {
        const int width = canvas()->width();
        height_ = canvas()->height();

        // Start the loop
        while (running()) {
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

                for (int j = 0; j < 16; j++) {
                    if (activityArr[i] == 0) {
                        canvas()->SetPixel(i, j, 0, 200, 0);
                    } else if (activityArr[i] == 1) {
                        canvas()->SetPixel(i, j, 200, 0, 0);
                    } else if (activityArr[i] == 2) {
                        canvas()->SetPixel(i, j, 200, 200, 0);
                    }
                }
            }

            cout << endl;

            usleep(1000000);
        }
    }

private:
    void drawBarRow(int bar, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
        for (uint8_t x = bar * barWidth_; x < (bar + 1) * barWidth_; ++x) {
            canvas()->SetPixel(x, height_ - 1 - y, r, g, b);
        }
    }

    int delay_ms_;
    int numBars_;
    int *barHeights_;
    int barWidth_;
    int height_;
    int t_;
};

int main(int argc, char *argv[]) {
    bool as_daemon = false;
    int runtime_seconds = -1;
    int rows = 16;
    int chain = 1;
    int parallel = 1;
    int scroll_ms = 30;
    int pwm_bits = -1;
    bool do_luminance_correct = true;

    if (getuid() != 0) {
        fprintf(stderr, "Must run as root to be able to access /dev/mem\n"
                "Prepend 'sudo' to the command:\n\tsudo %s ...\n", argv[0]);
        return 1;
    }

    // Initialize GPIO pins. This might fail when we don't have permissions.
    GPIO io;
    if (!io.Init())
        return 1;

    // Start daemon before we start any threads.
    if (as_daemon) {
        if (fork() != 0) {
            return 0;
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // The matrix, our 'frame buffer' and display updater.
    RGBMatrix *matrix = new RGBMatrix(&io, rows, chain, parallel);
    matrix->set_luminance_correct(do_luminance_correct);
    if (pwm_bits >= 0 && !matrix->SetPWMBits(pwm_bits)) {
        fprintf(stderr, "Invalid range of pwm-bits\n");
        return 1;
    }

    Canvas *canvas = matrix;

    // The ThreadedCanvasManipulator objects are filling
    // the matrix continuously.
    ThreadedCanvasManipulator *image_gen = new VolumeBars(canvas, scroll_ms, canvas->width() / 2);

    // Image generating demo is crated. Now start the thread.
    image_gen->Start();

    // Now, the image genreation runs in the background. We can do arbitrary
    // things here in parallel. In this demo, we're essentially just
    // waiting for one of the conditions to exit.
    if (as_daemon) {
        sleep(runtime_seconds > 0 ? runtime_seconds : INT_MAX);
    } else if (runtime_seconds > 0) {
        sleep(runtime_seconds);
    } else {
        // Things are set up. Just wait for <RETURN> to be pressed.
        printf("Press <RETURN> to exit and reset LEDs\n");
        getchar();
    }

    // Stop image generating thread.
    delete image_gen;
    delete canvas;

    return 0;
}