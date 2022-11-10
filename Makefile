inc = $(shell pkg-config --cflags gtk+-3.0)
lib = $(shell pkg-config --libs gtk+-3.0)

run: compile
	./chess

debug: compile
	gdb -tui ./chess

compile: chess.cpp
	g++ chess.cpp -g -o chess $(inc) $(lib)

