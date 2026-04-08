all: c8 c8-rl

c8: main.c
	cc -O3 main.c -o c8 -lX11 -lXext
	
c8-rl: main.c
	cc -O3 -DRL_BACKEND main.c -o c8-rl -lraylib -lm -lX11 -lXext
