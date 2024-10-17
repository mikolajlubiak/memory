// header
#include "memory.h"

// local
#include "common.h"

// std
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

char board[SIZE][SIZE];    // The board to display
char hidden[SIZE][SIZE];   // The hidden board
bool revealed[SIZE][SIZE]; // To track revealed cards

// Function to initialize the game board
void initializeBoard() {
  std::vector<char> cards;
  for (char c = 'A'; c < 'A' + TOTAL_PAIRS; ++c) {
    cards.push_back(c);
    cards.push_back(c); // Add pairs
  }

  // Shuffle the cards using std::shuffle
  std::random_device rd;  // Obtain a random number from hardware
  std::mt19937 eng(rd()); // Seed the generator
  std::shuffle(cards.begin(), cards.end(),
               eng); // Shuffle the cards

  // Fill the board
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE; ++j) {
      board[i][j] = cards[i * SIZE + j];
      hidden[i][j] = '*';     // Hidden state
      revealed[i][j] = false; // Not revealed
    }
  }
}

// Function to display the board
void displayBoard(const int x, const int y) {
  clear_screen();
  std::cout << "Memory Game Board:\n";
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE; ++j) {
      if (i == x && j == y) {
        std::cout << std::setw(2) << '.' << " ";
      } else if (revealed[i][j]) {
        std::cout << std::setw(2) << board[i][j] << " ";
      } else {
        std::cout << std::setw(2) << hidden[i][j] << " ";
      }
    }
    std::cout << std::endl;
  }
}

// Function to check if the selected cards match
bool checkMatch(int x1, int y1, int x2, int y2) {
  return board[x1][y1] == board[x2][y2] && !(x1 == x2 && y1 == y2);
}

void moveAcrossBoard(int *const x, int *const y, const bool select) {
  char ch;

  do {
    ch = getch();
    switch (ch) {
    case 'q':
    case '\n':
      break;
#ifdef _WIN32
    case 0:
    case 224:
      switch (getch()) {
      case 75:  // Left arrow
        (*y)--; // Move left
        break;
      case 77:  // Right arrow
        (*y)++; // Move right
        break;
      case 72:  // Up arrow
        (*x)--; // Move up
        break;
      case 80:  // Down arrow
        (*x)++; // Move down
        break;
      default:
        break;
      }

      *x = std::clamp(*x, 0, SIZE - 1);
      *y = std::clamp(*y, 0, SIZE - 1);

      displayBoard(*x, *y);

      if (select) {
        std::cout << "Select card";
      } else {
        std::cout << "Press enter to continue...";
      }

      break;
#else
    case '\033':
      getch();           // Skip the '[' character
      switch (getch()) { // Get the next character
      case 'A':
        (*x)--; // Move up
        break;
      case 'B':
        (*x)++; // Move down
        break;
      case 'C':
        (*y)++; // Move right
        break;
      case 'D':
        (*y)--; // Move left
        break;
      default:
        break;
      }

      *x = std::clamp(*x, 0, SIZE - 1);
      *y = std::clamp(*y, 0, SIZE - 1);

      displayBoard(*x, *y);

      if (select) {
        std::cout << "Select card";
      } else {
        std::cout << "Cards don't match.\n";
        std::cout << "Press enter to continue...";
      }

      break;
#endif
    default:
      break;
    }

  } while (ch != '\n' || (revealed[*x][*y] && select));
}