LANG=en

CC= gcc

WRAP_MALLOC=1

CXXFLAGS = -O3 -pedantic -Wall -g -ggdb --std=c++11
CXXFLAGS += -DENABLE_DEBUG=1

ifdef WRAP_MALLOC
	CXXFLAGS += -DWRAP_MALLOC -Wl,--wrap,malloc,--wrap,free,--wrap,realloc,--wrap,calloc
	CFLAGS += -DWRAP_MALLOC -Wl,--wrap,malloc,--wrap,free,--wrap,realloc,--wrap,calloc
endif

INCLUDE = -I../../glm-0.9.4.1/

CXXFLAGS += $(INCLUDE)

OBJS = main.o film.o shapes.o materials.o cameras.o skylights.o \
	lisc_gray.o lisc_linalg.o lisc.o malloc.o renderjob.o \
	rgbe.o lodepng.o trex/trex.o

.PHONY: run clean

default: gray.exe

%.o: %.cpp
	$(CC) -MMD -c -o $@ $< $(CXXFLAGS)

-include *.d

run: gray
	./gray.exe

clean:
	rm -f gray.exe $(OBJS)

gray.exe: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gray.exe $(OBJS)
