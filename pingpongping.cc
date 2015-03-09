// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "lib/matrix/include/led-matrix.h"
#include "lib/matrix/include/threaded-canvas-manipulator.h"

#include <assert.h>
#include <getopt.h>

using namespace rgb_matrix;

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
            canvas()->SetPixel(5, 15, 0, 200, 0);
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