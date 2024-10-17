// local
#include "common.h"
#include "memory.h"

// std
#include <cstring>
#include <future>
#include <iostream>
#include <thread>

// Main game loop
int main() {
  hideCursor();
  initializeBoard();

  int pairsFound = 0;

  int current_x = 0;
  int current_y = 0;
  int old_x = 0;
  int old_y = 0;

  bool blink = false;

  std::chrono::duration<float> timer = std::chrono::milliseconds(1000);

  char *const text_to_print(new char[512]);
  std::strcpy(text_to_print, "Select first card");

  auto display_board = std::async(std::launch::async, displayBoard, &current_x,
                                  &current_y, text_to_print, &blink, &timer);

  while (pairsFound < TOTAL_PAIRS) {

    // First card selection
    std::strcpy(text_to_print, "Select first card");

    moveAcrossBoard(&current_x, &current_y, &blink, &timer);

    revealed[current_x][current_y] = true;

    // Second card selection
    old_x = current_x;
    old_y = current_y;
    std::strcpy(text_to_print, "Select second card");

    moveAcrossBoard(&current_x, &current_y, &blink, &timer);

    revealed[current_x][current_y] = true;

    // Check for a match
    if (checkMatch(current_x, current_y, old_x, old_y)) {
      pairsFound++;
    } else {
      std::strcpy(text_to_print,
                  "Cards don't match.\nPress enter to continue...");

      int temp_x = current_x;
      int temp_y = current_y;

      moveAcrossBoard(&current_x, &current_y, &blink, &timer, false);

      revealed[temp_x][temp_y] = false; // Hide the first card again
      revealed[old_x][old_y] = false;   // Hide the second card again
    }
  }

  std::strcpy(text_to_print, "Congratulations! You've found all pairs!");

  while (true) {
    moveAcrossBoard(&current_x, &current_y, &blink, &timer, false);
  }

  return 0;
}
