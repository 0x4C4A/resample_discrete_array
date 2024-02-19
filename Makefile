SRCS=resample_discrete_array.cpp

all:
	g++ $(SRCS)	-o resample_discrete_array -std=c++14

