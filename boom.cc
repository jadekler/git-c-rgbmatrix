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

using std::min;
using std::max;

using namespace rgb_matrix;

// Imitation of volume bars
// Purely random height doesn't look realistic
// Contributed by: Vliedel
class VolumeBars : public ThreadedCanvasManipulator {
public:
    VolumeBars(Canvas *m, int delay_ms = 50, int numBars = 8)
            : ThreadedCanvasManipulator(m), delay_ms_(delay_ms),
              numBars_(numBars), t_(0) {
    }

    ~VolumeBars() {
        delete[] barHeights_;
        delete[] barFreqs_;
        delete[] barMeans_;
    }

    void Run() {
        const int width = canvas()->width();
        height_ = canvas()->height();
        barWidth_ = width / numBars_;
        barHeights_ = new int[numBars_];
        barMeans_ = new int[numBars_];
        barFreqs_ = new int[numBars_];
        heightGreen_ = height_ * 4 / 12;
        heightYellow_ = height_ * 8 / 12;
        heightOrange_ = height_ * 10 / 12;
        heightRed_ = height_ * 12 / 12;

        // Array of possible bar means
        int numMeans = 10;
        int means[10] = {1, 2, 3, 4, 5, 6, 7, 8, 16, 32};
        for (int i = 0; i < numMeans; ++i) {
            means[i] = height_ - means[i] * height_ / 8;
        }
        // Initialize bar means randomly
        srand(time(NULL));
        for (int i = 0; i < numBars_; ++i) {
            barMeans_[i] = rand() % numMeans;
            barFreqs_[i] = 1 << (rand() % 3);
        }

        // Start the loop
        while (running()) {
            if (t_ % 8 == 0) {
                // Change the means
                for (int i = 0; i < numBars_; ++i) {
                    barMeans_[i] += rand() % 3 - 1;
                    if (barMeans_[i] >= numMeans)
                        barMeans_[i] = numMeans - 1;
                    if (barMeans_[i] < 0)
                        barMeans_[i] = 0;
                }
            }

            // Update bar heights
            t_++;
            for (int i = 0; i < numBars_; ++i) {
                barHeights_[i] = (height_ - means[barMeans_[i]]) * sin(0.1 * t_ * barFreqs_[i]) + means[barMeans_[i]];
                if (barHeights_[i] < height_ / 8) {
                    barHeights_[i] = rand() % (height_ / 8) + 1;
                }
            }

            for (int i = 0; i < numBars_; ++i) {
                int y;
                for (y = 0; y < barHeights_[i]; ++y) {
                    if (y < heightGreen_) {
                        drawBarRow(i, y, 0, 200, 0);
                    } else if (y < heightYellow_) {
                        drawBarRow(i, y, 150, 150, 0);
                    } else if (y < heightOrange_) {
                        drawBarRow(i, y, 250, 100, 0);
                    } else {
                        drawBarRow(i, y, 200, 0, 0);
                    }
                }
                // Anything above the bar should be black
                for (; y < height_; ++y) {
                    drawBarRow(i, y, 0, 0, 0);
                }
            }
            usleep(delay_ms_ * 1000);
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
    int heightGreen_;
    int heightYellow_;
    int heightOrange_;
    int heightRed_;
    int *barFreqs_;
    int *barMeans_;
    int t_;
};

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s <options> -D <demo-nr> [optional parameter]\n",
            progname);
    fprintf(stderr, "Options:\n"
            "\t-r <rows>     : Display rows. 16 for 16x32, 32 for 32x32. "
            "Default: 32\n"
            "\t-P <parallel> : For Plus-models or RPi2: parallel chains. 1..3. "
            "Default: 1\n"
            "\t-c <chained>  : Daisy-chained boards. Default: 1.\n"
            "\t-L            : 'Large' display, composed out of 4 times 32x32\n"
            "\t-p <pwm-bits> : Bits used for PWM. Something between 1..11\n"
            "\t-j            : Low jitter. Experimental. Only RPi2\n"
            "\t-l            : Don't do luminance correction (CIE1931)\n"
            "\t-D <demo-nr>  : Always needs to be set\n"
            "\t-d            : run as daemon. Use this when starting in\n"
            "\t                /etc/init.d, but also when running without\n"
            "\t                terminal (e.g. cron).\n"
            "\t-t <seconds>  : Run for these number of seconds, then exit.\n"
            "\t       (if neither -d nor -t are supplied, waits for <RETURN>)\n");
    fprintf(stderr, "Demos, choosen with -D\n");
    fprintf(stderr, "\t0  - some rotating square\n"
            "\t1  - forward scrolling an image (-m <scroll-ms>)\n"
            "\t2  - backward scrolling an image (-m <scroll-ms>)\n"
            "\t3  - test image: a square\n"
            "\t4  - Pulsing color\n"
            "\t5  - Grayscale Block\n"
            "\t6  - Abelian sandpile model (-m <time-step-ms>)\n"
            "\t7  - Conway's game of life (-m <time-step-ms>)\n"
            "\t8  - Langton's ant (-m <time-step-ms>)\n"
            "\t9  - Volume bars (-m <time-step-ms>)\n");
    fprintf(stderr, "Example:\n\t%s -t 10 -D 1 runtext.ppm\n"
            "Scrolls the runtext for 10 seconds\n", progname);
    return 1;
}

int main(int argc, char *argv[]) {
    bool as_daemon = false;
    int runtime_seconds = -1;
    int demo = -1;
    int rows = 32;
    int chain = 1;
    int parallel = 1;
    int scroll_ms = 30;
    int pwm_bits = -1;
    bool large_display = false;
    bool do_luminance_correct = true;

    const char *demo_parameter = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "dlD:t:r:P:c:p:m:L")) != -1) {
        switch (opt) {
            case 'D':
                demo = atoi(optarg);
                break;

            case 'd':
                as_daemon = true;
                break;

            case 't':
                runtime_seconds = atoi(optarg);
                break;

            case 'r':
                rows = atoi(optarg);
                break;

            case 'P':
                parallel = atoi(optarg);
                break;

            case 'c':
                chain = atoi(optarg);
                break;

            case 'm':
                scroll_ms = atoi(optarg);
                break;

            case 'p':
                pwm_bits = atoi(optarg);
                break;

            case 'l':
                do_luminance_correct = !do_luminance_correct;
                break;

            case 'L':
                // The 'large' display assumes a chain of four displays with 32x32
                chain = 4;
                rows = 32;
                large_display = true;
                break;

            default: /* '?' */
                return usage(argv[0]);
        }
    }

    if (optind < argc) {
        demo_parameter = argv[optind];
    }

    if (demo < 0) {
        fprintf(stderr, "Expected required option -D <demo>\n");
        return usage(argv[0]);
    }

    if (getuid() != 0) {
        fprintf(stderr, "Must run as root to be able to access /dev/mem\n"
                "Prepend 'sudo' to the command:\n\tsudo %s ...\n", argv[0]);
        return 1;
    }

    if (rows != 16 && rows != 32) {
        fprintf(stderr, "Rows can either be 16 or 32\n");
        return 1;
    }

    if (chain < 1) {
        fprintf(stderr, "Chain outside usable range\n");
        return 1;
    }
    if (chain > 8) {
        fprintf(stderr, "That is a long chain. Expect some flicker.\n");
    }
    if (parallel < 1 || parallel > 3) {
        fprintf(stderr, "Parallel outside usable range.\n");
        return 1;
    }

    // Initialize GPIO pins. This might fail when we don't have permissions.
    GPIO io;
    if (!io.Init())
        return 1;

    // Start daemon before we start any threads.
    if (as_daemon) {
        if (fork() != 0)
            return 0;
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

    if (image_gen == NULL)
        return usage(argv[0]);

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
