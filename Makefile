SOURCES = $(wildcard ./src/*.c) $(wildcard ./src/backends/*.c)

all: c8 c8-rl

c8: $(SOURCES)
	cc -O3 $^ -o $@ -lX11 -lXext
	
c8-rl: $(SOURCES)
	cc -O3 -DRL_BACKEND $^ -o $@ -lraylib -lm -lX11 -lXext
