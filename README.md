# git-c-rgbmatrix

---

## Compiling and running

Please note: this has been tested on a mac with the following compiler options: `export CC=clang export CXX=clang++`

1. `cmake .`
1. `make`
1. `./rest_test`

## Additional notes

- This app assumes a UTC data endpoint (see the line with `-21600` in `rest_test.cc`)
