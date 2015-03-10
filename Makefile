CXXFLAGS=-Wall -O3 -g
OBJECTS=boom.o pingpongping.o
BINARIES=boom pingpongping
ALL_BINARIES="$(BINARIES)"

RGB_INCDIR=lib/matrix/include
RGB_LIBDIR=lib/matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lcurl

# Imagemagic flags, only needed if actually compiled.
MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`

all : $(BINARIES)

$(RGB_LIBRARY):
	$(MAKE) -C $(RGB_LIBDIR)

boom : boom.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) boom.o -o $@ $(LDFLAGS)

pingpongping : pingpongping.o $(RGB_LIBRARY) lib/json-parser/json.c
	$(CXX) $(CXXFLAGS) pingpongping.o -o $@ $(LDFLAGS) lib/json-parser/json.c

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(ALL_BINARIES)
	$(MAKE) -C lib clean
