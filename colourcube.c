#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int x;
  int y;
  int z;
} v3;

struct cube {
  v3 *data;
  v3 size;
  v3 *actives;
  int nactives;
};

static v3 *datap(struct cube *c, v3 pos) { return &c->data[pos.x + pos.y * c->size.x + pos.z * c->size.x * c->size.y]; }
v3 get(struct cube *c, v3 pos) { return *datap(c, pos); }
void set(struct cube *c, v3 pos, v3 val) { *datap(c, pos) = val; }
int init_cube(struct cube *c) {
  size_t bytes = c->size.x * c->size.y * c->size.z * sizeof(v3);
  c->data = malloc(bytes);
  memset(c->data, 0, bytes);

  /* for now actives gets as much memory as data because it's simpler */
  c->actives = malloc(bytes);
  memset(c->actives, 0, bytes);
  c->nactives = 0;

  return 0;
}

int main() {
  return 0;
}
