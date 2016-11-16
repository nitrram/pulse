DEFINES:= -D__VISUAL__ #-D__OPEN_CL__

PULSE_FLAGS = -lpulse -lpulse-simple -lOpenCL -lGLEW -lGL -lGLU -lglfw

CFLAGS:=-O3 -Wall -Wno-deprecated-declarations $(DEFINES) $(PULSE_FLAGS)
CXXFLAGS:=-O3 -Wall -Wno-deprecated-declarations $(DEFINES) -std=c++11 $(PULSE_FLAGS)
CC=gcc
CXX=g++
CPP=cpp

OBJECTS_CPP = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJECTS_C = $(patsubst %.c,%.o,$(wildcard *.c))
OBJECTS += $(OBJECTS_CPP)
OBJECTS += $(OBJECTS_C)

all: pulse

pulse: $(OBJECTS)
	@echo $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

.c.o:
	@echo "c compile"
	$(CC) $(CFLAGS) -c $<

.cl.o: .cl
	@echo "cl compile"
	$(CPP) -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean

clean:
	rm -f *.o *~ pulse
