#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <glib.h>
#include <png.h>

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

void printv3(v3 pos) { printf("(%i, %i, %i)\n", pos.x, pos.y, pos.z); fflush(NULL);}

int init_cube(struct cube *c) {
  size_t bytes = c->size.x * c->size.y * c->size.z * sizeof(v3);
  c->data = malloc(bytes);
  memset(c->data, 0, bytes);

  c->actives = g_queue_new();

  for(int i = 0; i < 40; i++) {
    v3 *pos = get_v3();
    pos->x = rand() / (RAND_MAX / c->size.x);
    pos->y = rand() / (RAND_MAX / c->size.y);
    pos->z = rand() / (RAND_MAX / c->size.z);

    v3 colour = {
        .x = rand() / (RAND_MAX / 255),
        .y = rand() / (RAND_MAX / 255),
        .z = rand() / (RAND_MAX / 255)
    };
    printv3(colour); printf("\n");
    set(c, *pos, colour);
    g_queue_push_head(c->actives, pos);
  }

  return 0;
}

bool v3eq(v3 v1, v3 v2) { return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z; }
bool in_range(struct cube *c, v3 pos) { 
  return c->size.x > pos.x && pos.x >= 0 &&
         c->size.y > pos.y && pos.y >= 0 &&
         c->size.z > pos.z && pos.z >= 0;
}
int fill_cube(struct cube *c) {
  v3 expands[9+9+9] = {{0,},}; // a 3x3x3 cube
  while(g_queue_get_length(c->actives) > 0) {
    v3 colour = {0,0,0};
    int ncolours = 0;
    int nexpands = 0;
    int ncompleted = 0;

    v3 pos = *(v3*)g_queue_peek_tail(c->actives);
    for(int i = -1; i < 2; i++)
      for(int j = -1; j < 2; j++)
        for(int k = -1; k < 2; k++) {
          //if(i == j == k == 0) continue;
          v3 neighbour_pos = {pos.x+i, pos.y+j, pos.z+k};
          if(!in_range(c, neighbour_pos)) continue;

          v3 neighbour = get(c, neighbour_pos);
          if(v3eq(neighbour, (v3){0,0,0})) {
            // add to potential expansions
            expands[nexpands++] = neighbour_pos;
          }
          else { // add to averaging set
            colour.x += neighbour.x;
            colour.y += neighbour.y;
            colour.z += neighbour.z;
            ncolours++;
          }
        }
    if(0 == nexpands) {
      put_v3(g_queue_pop_tail(c->actives)); // return the actives memory
      ncompleted++;
      if(ncompleted % c->size.x == 0) {
        printf(".");
        if(ncompleted % (c->size.x * c->size.y) == 0)
          printf("\n%i / %i\n", ncompleted, c->size.x * c->size.y * c->size.z);
        fflush(NULL);
      }
    }
    else {
      // Choose which neighbour to expand into
      v3 *chosen_pos = get_v3();
      *chosen_pos = expands[rand() / (RAND_MAX / nexpands)];
      colour.x = 5 - rand() / (RAND_MAX / 10) + colour.x / ncolours;
      colour.y = 5 - rand() / (RAND_MAX / 10) + colour.y / ncolours;
      colour.z = 5 - rand() / (RAND_MAX / 10) + colour.z / ncolours;
      set(c, *chosen_pos, colour);
      g_queue_insert_before(c->actives,
                            g_queue_peek_nth_link(c->actives, rand() / (RAND_MAX / g_queue_get_length(c->actives))),
                            chosen_pos);
    }
  }
  return 0;
}

int write_pngs(struct cube *c) {
  char filename[14] = "test00000.png";//\0
  FILE *fp = NULL;
  png_bytep *row_pointers = NULL;

  for(int z = 0; z < c->size.z; z++) { // each slice in z gets a frame
    sprintf(filename, "test%.5d.png", z);
    printf("%s\n", filename);
    fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        c->size.x, c->size.y,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
        );
    png_write_info(png, info);

    // Allocate png image memory
    if(row_pointers == NULL) {
      row_pointers = malloc(sizeof(png_bytep) * c->size.y);
      for(int y = 0; y < c->size.y; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
      }
    }

    // Translate cube data to row_pointers
    for(int i = 0; i < c->size.x; i++)
      for(int j = 0; j < c->size.y; j++) {
        v3 colour = get(c, (v3){i,j,0});
        row_pointers[j][3*i+0] = colour.x; // R
        row_pointers[j][3*i+1] = colour.y; // G
        row_pointers[j][3*i+2] = colour.z; // B
      }
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);
  }

  for(int y = 0; y < c->size.y; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);
  return 0;
}

int main() {
  struct cube c;
  c.size.x = c.size.y = c.size.z = 200;
  init_cube(&c);
  fill_cube(&c);
  write_pngs(&c);
  return 0;
}
