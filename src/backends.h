#ifndef C8_BACKENDS_H
#define C8_BACKENDS_H

#ifdef RL_BACKEND
#include "./backends/rl.h"
typedef RLBackend Backend;
#else
#include "./backends/x11.h"
typedef X11Backend Backend;
#endif

struct C8Emu;

void c8_init_backend(struct C8Emu *c8);
void c8_backend_pool_events(struct C8Emu *c8);
void c8_backend_draw(struct C8Emu *c8);

#endif
