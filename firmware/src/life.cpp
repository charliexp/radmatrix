#include <string.h>
#include <Arduino.h>
#include "config.h"

#define CELLS_X COL_MODULES * 20
#define CELLS_Y ROW_MODULES * 20
#define CELL_COUNT (CELLS_X * CELLS_Y)

bool cells[CELL_COUNT] = {0};

#define XY(x, y) (x + y * CELLS_Y)

void life_setup() {
  // set up initial state
  cells[XY(2, 1)] = true;
  cells[XY(3, 2)] = true;
  cells[XY(1, 3)] = true;
  cells[XY(2, 3)] = true;
  cells[XY(3, 3)] = true;
}

void life_step() {
  bool new_cells[CELL_COUNT] = {0};

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      int neighbors = 0;
      bool wasLive = cells[XY(x, y)];

      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0) {
            continue;
          }

          int check_y = y + dy;
          int check_x = x + dx;

          if (check_y >= 0 && check_y < CELLS_Y) {
            if (check_x >= 0 && check_x < CELLS_X) {
              if (cells[XY(check_x, check_y)]) {
                neighbors++;
              }
            }
          }
        }
      }

      if (wasLive && neighbors < 2) {
        // dies by underpopulation
      } else if (wasLive && (neighbors == 2 || neighbors == 3)) {
        // continues to live
        new_cells[XY(x, y)] = true;
      } else if (wasLive && neighbors > 3) {
        // dies by overpopulation
      } else if (!wasLive && neighbors == 3) {
        // reproduces
        new_cells[XY(x, y)] = true;
      } else {
        // still dead, jim
      }
    }
  }

  // copy
  memcpy(cells, new_cells, sizeof(cells));
}
