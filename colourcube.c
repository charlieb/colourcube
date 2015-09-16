#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

typedef struct {
  int x;
  int y;
  int z;
} v3;

struct cube {
  v3 *data;
  v3 size;
  GQueue *actives;
};

static GTrashStack *pool = NULL;
gpointer get_v3() {
  const int block_size = 500;

  // allocate more trash if necessary
  if(!g_trash_stack_peek(&pool)) {
    v3 *block = malloc(block_size * sizeof(v3));
    for(int i = 0; i < block_size; i++)
      g_trash_stack_push(&pool, block + i);
  }
  return g_trash_stack_pop(&pool);
}
void put_v3(gpointer data) { g_trash_stack_push(&pool, data); }

static v3 *datap(struct cube *c, v3 pos) { return &c->data[pos.x + pos.y * c->size.x + pos.z * c->size.x * c->size.y]; }
v3 get(struct cube *c, v3 pos) { return *datap(c, pos); }
void set(struct cube *c, v3 pos, v3 val) { *datap(c, pos) = val; }

int init_cube(struct cube *c) {
  size_t bytes = c->size.x * c->size.y * c->size.z * sizeof(v3);
  c->data = malloc(bytes);
  memset(c->data, 0, bytes);

  c->actives = g_queue_new();

  for(int i = 0; i < 10; i++) {
    v3 *pos = get_v3();
    pos->x = rand() / (RAND_MAX / c->size.x);
    pos->y = rand() / (RAND_MAX / c->size.y);
    pos->z = rand() / (RAND_MAX / c->size.z);

    v3 colour = {
        .x = rand() / (RAND_MAX / 255),
        .y = rand() / (RAND_MAX / 255),
        .z = rand() / (RAND_MAX / 255)
    };
    set(c, *pos, colour);
    g_queue_push_head(c->actives, pos);
  }

  return 0;
}

int main() {
  return 0;
}
