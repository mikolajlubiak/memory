// local
#include "common.h"
#include "memory.h"

// std
#include <iostream>

/*
int main() {
  // Clear the screen
  clear_screen();

  // Display the message
  std::cout << "Press any key to continue...\n";

  while (true) {
    // Wait for a key press
    char ch = getch(); // Get the pressed key

    // Clear the screen
    clear_screen();

    // Display the message
    std::cout << "Press any key to continue...\n";

    // Display the pressed key
    std::cout << "You pressed: " << ch << std::endl;
  }

  return 0;
}
*/

// Main game loop
int main() {
  hideCursor();
  initializeBoard();
  int pairsFound = 0;

  while (pairsFound < TOTAL_PAIRS) {
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    displayBoard(0, 0);

    // First card selection
    std::cout << "Select card";
    moveAcrossBoard(&x1, &y1, true);

    revealed[x1][y1] = true;
    displayBoard(0, 0);

    // Second card selection
    std::cout << "Select card";
    moveAcrossBoard(&x2, &y2, true);

    revealed[x2][y2] = true;
    displayBoard(0, 0);

    // Check for a match
    if (checkMatch(x1, y1, x2, y2)) {
      pairsFound++;
    } else {
      std::cout << "Cards don't match.\n";
      std::cout << "Press enter to continue...";

      int temp_x = 0, temp_y = 0;
      moveAcrossBoard(&temp_x, &temp_y, false);

      revealed[x1][y1] = false; // Hide the first card again
      revealed[x2][y2] = false; // Hide the second card again
    }
  }

  std::cout << "Congratulations! You've found all pairs!\n";
  return 0;
}
