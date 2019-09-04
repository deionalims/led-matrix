CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
OBJECTS=main.o
BINARIES=matrix-project

RGB_LIB_DISTRIBUTION=./lib/rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

RAPIDJSON_INCDIR=./lib/rapidjson/include

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lcurl

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

matrix-project : main.o  $(RGB_LIBRARY)
	$(CXX) $< -o $@ $(LDFLAGS)

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -I$(RAPIDJSON_INCDIR) -I.  -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)

FORCE:
 .PHONY: FORCE
