LANG=en

CXXFLAGS = -O0 -pedantic -Wall -g -ggdb --std=c++11

INCLUDE = -I../../glm-0.9.4.1/

CXXFLAGS += $(INCLUDE)

OBJS = main.o film.o shapes.o lodepng.o lisc_gray.o lisc.o trex/trex.o

gray: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gray $(OBJS)
