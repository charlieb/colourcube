#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <png.h>

typedef struct {
  int x;
  int y;
  int z;
} v3;

bool v3eq(v3 v1, v3 v2) { return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z; }
bool in_cube(v3 size, v3 pos) { 
  return size.x > pos.x && pos.x >= 0 &&
         size.y > pos.y && pos.y >= 0 &&
         size.z > pos.z && pos.z >= 0;
}

/* BOOLCUBE */
typedef struct {
  bool *data;
  v3 size;
} boolcube;

static bool *bool_datap(boolcube *c, v3 pos) { return &c->data[pos.x + pos.y * c->size.x + pos.z * c->size.x * c->size.y]; }
bool bool_get(boolcube *c, v3 pos) { return *bool_datap(c, pos); }
void bool_set(boolcube *c, v3 pos, bool val) { *bool_datap(c, pos) = val; }

int ncubed(int n) { return n*n*n; }
void bool_find_neighbours(boolcube *c, v3 pos, int max_range, v3 **neighbours, int *neighbour_max_range, int *nneighbours)
{
  int range = 0;
  const int expands_range_inc = 10;
  *nneighbours = 0;

  while(*nneighbours <= 0 && range < max_range) {
    range++;
    if(range > *neighbour_max_range) {
       *neighbour_max_range += expands_range_inc;
       *neighbours = realloc(*neighbours, ncubed(2*(*neighbour_max_range)+1) * sizeof(v3)); // a 3x3x3 cube then 5x5x5 etc.
    }

    for(int i = -range; i <= range; i++)
      for(int j = -range; j <= range; j++)
        for(int k = -range; k <= range; k++) {
          if(i == 0 && j == 0 && k == 0) continue;
          v3 neighbour_pos = {pos.x+i, pos.y+j, pos.z+k};
          if(!in_cube(c->size, neighbour_pos)) continue;

          // if the position contains false. it's available
          if(!bool_get(c, neighbour_pos)) {
            // add to potential expansions
            (*neighbours)[(*nneighbours)++] = neighbour_pos;
          }
        }
  }
}

bool bool_find_neighbour(boolcube *c, v3 pos, v3 *neighbour, int max_range)
{
  int nexpands = 0;
  static v3 *expands = NULL;
  static int expands_range = 0; // note: this is range not size
  bool_find_neighbours(c, pos, max_range, &expands, &expands_range, &nexpands);
  if(nexpands > 0) {
    *neighbour = expands[rand() / (RAND_MAX / nexpands)];
    return true;
  }
  return false;
}

void bool_reset(boolcube *c)
{
  memset(c->data, 0, c->size.x * c->size.y * c->size.z * sizeof(bool));
}
boolcube *bool_init_cube(v3 size)
{
  boolcube *bc = malloc(sizeof(boolcube));
  bc->size = size;
  bc->data = malloc(size.x * size.y * size.z * sizeof(bool));
  bool_reset(bc);
  return bc;
}

/* v3 CUBE */
struct cube {
  v3 *data;
  v3 size;
};

static v3 *datap(struct cube *c, v3 pos) { return &c->data[pos.x + pos.y * c->size.x + pos.z * c->size.x * c->size.y]; }
v3 get(struct cube *c, v3 pos) { return *datap(c, pos); }
void set(struct cube *c, v3 pos, v3 val) { *datap(c, pos) = val; }

void printv3(v3 pos) { printf("(%i, %i, %i)\n", pos.x, pos.y, pos.z); fflush(NULL);}

int init_cube(struct cube *c) 
{
  size_t bytes = c->size.x * c->size.y * c->size.z * sizeof(v3);
  c->data = malloc(bytes);
  memset(c->data, 0, bytes);
  return 0;
}

v3 pick_neighbour(struct cube *c, v3 pos, v3 nil, int max_range)
{
  int range = 0;

  while(range < max_range) {
    range++;
    v3 expands[2*range+1]; // a 3x3x3 cube then 5x5x5 etc.
    int nexpands = 0;

    for(int i = -range; i <= range; i++)
      for(int j = -range; j <= range; j++)
        for(int k = -range; k <= range; k++) {
          if(i == 0 && j == 0 && k == 0) continue;
          v3 neighbour_pos = {pos.x+i, pos.y+j, pos.z+k};
          if(!in_cube(c->size, neighbour_pos)) continue;

          v3 neighbour = get(c, neighbour_pos);
          /*
          printv3(neighbour_pos); fflush(NULL);
          printv3(neighbour); fflush(NULL);
          printf("---\n"); fflush(NULL);
          */
          if(v3eq(neighbour, nil)) {
            // add to potential expansions
            expands[nexpands++] = neighbour_pos;
          }
        }
    if(nexpands > 0)
      return expands[rand() / (RAND_MAX / nexpands)];
  }
  return nil;
}

void shuffle(int *array, int size)
{
  // https://en.wikipedia.org/wiki/Fisher-Yates_shuffle
  for(int i = 0; i < size - 1; i++) {
      int j = i + rand() / (RAND_MAX / (size - i));
      printf("swap %i, %i\n", i,j); fflush(NULL);
      int tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
    }
}

bool colour_neighbour(struct cube *c, boolcube *bc, boolcube *cc, v3 pos)
{
   v3 new_pos;
   // does the current voxel have an available neighbour
   if(bool_find_neighbour(bc, pos, &new_pos, 1)) {
      // Find a colour that hasn't been used yet nearest to the 
      // colour of the current voxel
      v3 colour = get(c, pos);
      v3 new_colour;
      if(!bool_find_neighbour(cc, colour, &new_colour, 255)) {
         // oh crap ran out of colour, start over
         bool_reset(cc);
         bool_find_neighbour(cc, colour, &new_colour, 255);
      }
      set(c, new_pos, new_colour);
      bool_set(bc, new_pos, true);
      bool_set(cc, new_colour, true);
      //if(nfilled % 100 == 0) printf("Filled %i\n", nfilled);
      return true;
   }
   return false;
}


int fill_cube(struct cube *c) 
{
  int ncompleted = 0;
  int nfilled = 0;
  const int nvoxels = c->size.x * c->size.y * c->size.z;

  // bool cube - did we fill in a colour
  boolcube *bc = bool_init_cube(c->size);
  // finished cube - is it filled and are all it's neighbours filled
  boolcube *fc = bool_init_cube(c->size);
  boolcube *cc = bool_init_cube((v3){255,255,255});

  // Place some pixels to seed the process
  for(int i = 0; i < 40; i++) {
    v3 pos = (v3){rand() / (RAND_MAX / c->size.x),
                  rand() / (RAND_MAX / c->size.y),
                  rand() / (RAND_MAX / c->size.z)};

    v3 colour = {
        .x = rand() / (RAND_MAX / 255),
        .y = rand() / (RAND_MAX / 255),
        .z = rand() / (RAND_MAX / 255)
    };
    set(c, pos, colour);
    bool_set(bc, pos, true);
  }

  // Here we create random order arrays for the three
  // axes so that we can iterate over every voxel in
  // a random order. This means that any directionality
  // in the animation due to the left-to-right, top-to-bottom
  // order of processing is eliminated (on average)
  int *order_x = malloc(c->size.x * sizeof(int));
  for(int i = 0; i < c->size.x; i++) order_x[i] = i;
  shuffle(order_x, c->size.x);

  int *order_y = malloc(c->size.y * sizeof(int));
  for(int i = 0; i < c->size.y; i++) order_y[i] = i;
  shuffle(order_y, c->size.y);

  int *order_z = malloc(c->size.z * sizeof(int));
  for(int i = 0; i < c->size.z; i++) order_z[i] = i;
  shuffle(order_z, c->size.z);


  // As long as there are uncoloured voxels
  while(ncompleted < nvoxels) {
     for(int i = 0; i < c->size.x; i++)
        for(int j = 0; j < c->size.y; j++)
           for(int k = 0; k < c->size.z; k++) {
              //v3 pos = (v3){i,j,k};
              v3 pos = (v3){order_x[i], order_y[j], order_z[k]};
              if(!bool_get(bc, pos)) continue; // pos does not yet have colour
              if(bool_get(fc, pos)) continue; // pos known to have no unfilled neighbours
              if(colour_neighbour(c, bc, cc, pos))
                 nfilled++;
              else { // no neighbour found
                 bool_set(fc, pos, true);
                 ncompleted++;
                 if(ncompleted % c->size.x == 0) {
                    printf(".");
                    if(ncompleted % (c->size.x * c->size.y) == 0)
                       printf("\n%i / %i, %i actives\n", ncompleted, c->size.x * c->size.y * c->size.z, nvoxels);
                    fflush(NULL);
                 }
              }
           }
  }
  return 0;
}

int write_pngs(struct cube *c, char *filename_base) 
{
  char filename[strlen(filename_base) + 10]; // filename + "00000.png\0"
  FILE *fp = NULL;
  png_bytep *row_pointers = NULL;

  for(int z = 0; z < c->size.z; z++) { // each slice in z gets a frame
    sprintf(filename, "%s%.5d.png",filename_base, z);
    printf("%s\n", filename);
    fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGB format.
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
          v3 colour = get(c, (v3){i,j,z});
          row_pointers[j][3*i+0] = colour.x; // R
          row_pointers[j][3*i+1] = colour.y; // G
          row_pointers[j][3*i+2] = colour.z; // B
       }
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);
    fclose(fp);

  }
  for(int y = 0; y < c->size.y; y++) {
     free(row_pointers[y]);
  }
  free(row_pointers);

  return 0;
}

int main(int argc, char **argv) 
{
  struct cube c;
  // colourcube dimensions in format XXxYYxZZ
  if(3 != sscanf(argv[1], "%ix%ix%ix", &c.size.x, &c.size.y, &c.size.z)) {
    c.size.x = c.size.y = 50;
    c.size.z = 30;
  }
  char *filename_base;
  if(argc >= 3) // filename
     filename_base = argv[2];
  else
     filename_base = "default";
  init_cube(&c);
  fill_cube(&c);
  write_pngs(&c, filename_base);
  return 0;
}
