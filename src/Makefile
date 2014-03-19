LANG=en

CC= gcc

CXXFLAGS = -O3 -pedantic -Wall -g -ggdb --std=c++11
CXXFLAGS += -Wl,--wrap,malloc,--wrap,free,--wrap,realloc,--wrap,calloc
CFLAGS += -Wl,--wrap,malloc,--wrap,free,--wrap,realloc,--wrap,calloc

INCLUDE = -I../../glm-0.9.4.1/

CXXFLAGS += $(INCLUDE)

OBJS = main.o film.o shapes.o materials.o cameras.o skylights.o rgbe.o lodepng.o lisc_gray.o lisc_linalg.o lisc.o malloc.o trex/trex.o

.PHONY: run clean

default: gray.exe

run: gray
	./gray.exe

clean:
	rm gray.exe $(OBJS)

gray.exe: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gray.exe $(OBJS)
