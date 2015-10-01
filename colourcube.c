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

void printv3(v3 pos) { printf("(%i, %i, %i)\n", pos.x, pos.y, pos.z); fflush(NULL);}
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
int count(boolcube *c, bool t_or_f)
{
  int total = 0;
  for(int i = 0; i < c->size.x * c->size.y * c->size.z; i++) 
    if(c->data[i] == t_or_f)
      total++;
  return total;
}

int ncubed(int n) { return n*n*n; }
bool bool_find_neighbours(boolcube *c, v3 pos, bool occupied, int max_range, v3 **neighbours, int *neighbour_max_range, int *nneighbours)
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
          if(occupied == bool_get(c, neighbour_pos)) {
            // add to potential expansions
            (*neighbours)[(*nneighbours)++] = neighbour_pos;
          }
        }
  }
  return *nneighbours;
}

bool bool_find_neighbour(boolcube *c, v3 pos, v3 *neighbour, int max_range)
{
  int nexpands = 0;
  static v3 *expands = NULL;
  static int expands_range = 0; // note: this is range not size
  bool_find_neighbours(c, pos, false, max_range, &expands, &expands_range, &nexpands);
  if(nexpands > 0) {
    *neighbour = expands[rand() / (RAND_MAX / nexpands)];
    return true;
  }
  return false;
}

void bool_reset(boolcube *c)
{
  printf("Reset boolcube\n");
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
    int tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
  }
}

v3 average_neighbour_colour(struct cube *c, boolcube *bc, boolcube *cc, v3 pos, int neighbour_range, int colour_range)
{
  int nexpands = 0;
  static v3 *expands = NULL;
  static int expands_range = 0; // note: this is range not size

  // Make sure we find at least 1 coloured neighbour
  if(!bool_find_neighbours(bc, pos, true, neighbour_range, &expands, &expands_range, &nexpands)) {
    // utter failure, return random colour
    printf("RANDOM!\n");
    return (v3){
      .x = rand() / (RAND_MAX / 255),
        .y = rand() / (RAND_MAX / 255),
        .z = rand() / (RAND_MAX / 255)
    };
  }

  v3 total_colour = {0,0,0};
  for(int i = 0; i < nexpands; i++) {
    v3 tmp_colour = get(c, expands[i]);
    total_colour.x += tmp_colour.x;
    total_colour.y += tmp_colour.y;
    total_colour.z += tmp_colour.z;
  }
  // nexpands can't be zero because we have at least 1 coloured neighbour
  // or we couldn't be picked
  v3 colour = {
    total_colour.x / nexpands,
    total_colour.y / nexpands,
    total_colour.z / nexpands
  };

  v3 new_colour;
  if(!bool_find_neighbour(cc, colour, &new_colour, colour_range)) {
    colour.x += colour_range - rand() / (RAND_MAX / (2 * colour_range));
    if(colour.x > 255) colour.x = 255;
    if(colour.x < 0  ) colour.x = 0;
    colour.y += colour_range - rand() / (RAND_MAX / (2 * colour_range));
    if(colour.y > 255) colour.y = 255;
    if(colour.y < 0  ) colour.y = 0;
    colour.z += colour_range - rand() / (RAND_MAX / (2 * colour_range));
    if(colour.z > 255) colour.z = 255;
    if(colour.z < 0  ) colour.z = 0;
    return colour; // just return the average don't reset the colour cube
    // oh crap ran out of colour, start over
    bool_reset(cc);
    bool_find_neighbour(cc, colour, &new_colour, colour_range);
  }
  return new_colour;
}

bool colour_neighbour_av(struct cube *c, boolcube *bc, boolcube *cc, v3 pos)
{

  v3 new_pos;
  // does the current voxel have an available neighbour
  if(bool_find_neighbour(bc, pos, &new_pos, 1)) {
    // Find a colour that hasn't been used yet nearest to the 
    // colour of the current voxel
    v3 new_colour = average_neighbour_colour(c, bc, cc, new_pos, 1, 1);
    set(c, new_pos, new_colour);
    bool_set(bc, new_pos, true);
    bool_set(cc, new_colour, true);
    return true;
  }
  return false;
}

bool colour_neighbour(struct cube *c, boolcube *bc, boolcube *cc, v3 pos)
{
  v3 new_pos;
  // does the current voxel have an available neighbour
  if(bool_find_neighbour(bc, pos, &new_pos, 1)) {
    // Find a colour that hasn't been used yet nearest to the 
    // colour of the current voxel
    v3 colour = get(c, pos);
    v3 new_colour = {0,0,0};
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

void init_random_points(struct cube *c, boolcube *bc, boolcube *cc, int n)
{
  // Place some pixels to seed the process
  for(int i = 0; i < n; i++) {
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
    bool_set(cc, colour, true);
  }
}

void init_random_points_near_center(struct cube *c, boolcube *bc, boolcube *cc, int n, v3 range)
{
  // Place some pixels to seed the process
  for(int i = 0; i < n; i++) {
    v3 pos = {
      c->size.x / 2 + range.x / 2 - rand() / (RAND_MAX / range.x),
      c->size.y / 2 + range.y / 2 - rand() / (RAND_MAX / range.y),
      c->size.z / 2 + range.z / 2 - rand() / (RAND_MAX / range.z),
    };

    v3 colour = {
      .x = rand() / (RAND_MAX / 255),
      .y = rand() / (RAND_MAX / 255),
      .z = rand() / (RAND_MAX / 255)
    };
    set(c, pos, colour);
    bool_set(bc, pos, true);
    bool_set(cc, colour, true);
  }
}

void init_random_center(struct cube *c, boolcube *bc)
{
  // Place some pixels to seed the process
  v3 pos = {
    c->size.x / 2,
    c->size.y / 2,
    c->size.z / 2
  };

  v3 colour = {
    .x = rand() / (RAND_MAX / 255),
    .y = rand() / (RAND_MAX / 255),
    .z = rand() / (RAND_MAX / 255)
  };
  set(c, pos, colour);
  bool_set(bc, pos, true);
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
  // colour cube - did we use a colour already?
  boolcube *cc = bool_init_cube((v3){255,255,255});

  //init_random_points(c, bc, 100);
  init_random_points_near_center(c, bc, cc, 1, (v3){c->size.x - 1, c->size.y - 1, 1 }); //c->size.z - 1});
  //init_random_center(c, bc);

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
          //if(colour_neighbour(c, bc, cc, pos))
          if(colour_neighbour_av(c, bc, cc, pos))
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

int walk_cube(struct cube *c) 
{
  int nfilled = 0;
  const int nvoxels = c->size.x * c->size.y * c->size.z;

  // bool cube - did we fill in a colour
  boolcube *bc = bool_init_cube(c->size);
  // finished cube - is it filled and are all it's neighbours filled
  boolcube *fc = bool_init_cube(c->size);
  // colour cube - did we use a colour already?
  boolcube *cc = bool_init_cube((v3){255,255,255});

  v3 search_start = {0,0,0};

  /*
  init_random_points(c, bc, cc, 1024);
  nfilled = 1024;
  */

  v3 active = {
    rand() / (RAND_MAX / c->size.x),
    rand() / (RAND_MAX / c->size.y),
    rand() / (RAND_MAX / c->size.z)
  };
  v3 colour = {
    .x = rand() / (RAND_MAX / 255),
    .y = rand() / (RAND_MAX / 255),
    .z = rand() / (RAND_MAX / 255)
  };
  set(c, active, colour);
  bool_set(bc, active, true);
  bool_set(cc, colour, true);
  nfilled++;

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
  while(nfilled < nvoxels) {
    // get an unfilled neighbour to the active voxel
    v3 next;
    if(bool_find_neighbour(bc, active, &next, 1)) {
      // get a colour near the active voxel's
      colour = get(c, active);
      v3 next_colour;
      next_colour = average_neighbour_colour(c, bc, cc, next, 1, 5);
      /*
      if(!bool_find_neighbour(cc, colour, &next_colour, 255)) {
        // we ran out of colours: reset
        bool_reset(cc);
        bool_find_neighbour(cc, colour, &next_colour, 255);
        printf("used all colours\n"); 
      }
      */
      set(c, next, next_colour);
      bool_set(bc, next, true);
      bool_set(cc, next_colour, true);
      active = next;
      nfilled++;
      // Reporting progress
      if(nfilled % c->size.x == 0) { printf("."); fflush(NULL); }
      if(nfilled % (c->size.x * c->size.y) == 0) {
        printf(" %i / %i\n", nfilled / (c->size.x * c->size.y), c->size.z);
        fflush(NULL);
      }
    }
    else { // set this voxel finished and find a new voxel to activate
      bool_set(fc, active, true);
      bool start = true;
      bool in_finished_run = true; // track the first non-finished voxel
      for(int i = search_start.x; i < c->size.x; i++)
        for(int j = (start ? search_start.y : 0); j < c->size.y; j++)
          for(int k = (start ? search_start.z : 0); k < c->size.z; k++) {
            start = false;
            v3 pos = (v3){order_x[i], order_y[j], order_z[k]};
            //v3 pos = (v3){i, j, k};
            bool finished = bool_get(fc, pos);
            if(in_finished_run && finished) 
              search_start = (v3){i,j,k};
            else
              in_finished_run = false;
            if(bool_get(bc, pos) && !finished) { // pos does have colour but isn't finished
              active = pos;
              i = c->size.x;
              j = c->size.y;
              k = c->size.z;
            }
          }
    }
  }

  printf("Used %i colours\n", count(cc, true));
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
  //fill_cube(&c);
  walk_cube(&c);
  write_pngs(&c, filename_base);
  return 0;
}
