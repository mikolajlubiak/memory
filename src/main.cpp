// local
#include "common.h"
#include "memory.h"

// Main game loop
int main() {
  Memory game{4};

  game.init();

  game.loop();

  return 0;
}