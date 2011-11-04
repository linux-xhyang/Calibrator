all: cache-detector

cache-detector: cache-detector.cc
	g++ -g -ggdb -Wall -Werror -O3 -o $@ $<
