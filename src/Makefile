LANG=en

CXXFLAGS = -O3 -pedantic -Wall -Werror -g -ggdb --std=c++11

INCLUDE = -I../../glm-0.9.4.1/

CXXFLAGS += $(INCLUDE)

OBJS = main.o film.o shapes.o materials.o rgbe.o lodepng.o lisc_gray.o lisc_linalg.o lisc.o trex/trex.o

.PHONY: run clean

run: gray
	./gray.exe

clean:
	rm gray.exe $(OBJS)

gray: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gray $(OBJS)
