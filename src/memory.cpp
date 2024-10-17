#include "memory.h"
#include "common.h"

// std
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

char board[SIZE][SIZE];                  // The board to display
bool revealed[SIZE][SIZE];               // To track revealed cards
ftxui::Color revealedColors[SIZE][SIZE]; // To track card colors

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
  std::shuffle(cards.begin(), cards.end(), eng); // Shuffle the cards

  // Fill the board
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE; ++j) {
      board[i][j] = cards[i * SIZE + j];
      revealed[i][j] = false;                           // Not revealed
      revealedColors[i][j] = ftxui::Color::YellowLight; // YellowLight
    }
  }
}

// Function to create the board display
ftxui::Element CreateBoard(const int *const current_x,
                           const int *const current_y,
                           const bool *const blink) {
  auto rows = std::vector<ftxui::Element>();
  for (int i = 0; i < SIZE; ++i) {
    auto cells = std::vector<ftxui::Element>();
    for (int j = 0; j < SIZE; ++j) {
      ftxui::Element cell;

      // Determine the content of the cell
      if (i == *current_x && j == *current_y && !(*blink)) {
        cell = ftxui::text("_") | ftxui::color(ftxui::Color::Blue);

      } else if (revealed[i][j]) {
        cell = ftxui::text(std::string(1, board[i][j])) |
               ftxui::color(revealedColors[i][j]);

      } else {
        cell = ftxui::text("*") | ftxui::color(ftxui::Color::YellowLight);
      }

      // Create the cell with the determined content
      cell = cell | ftxui::center | ftxui::border | ftxui::bold |
             ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                         16)                                // Set width to 16
             | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 9); // Set height to 9

      cells.push_back(cell);
    }
    rows.push_back(ftxui::hbox(cells));
  }
  return ftxui::vbox(rows);
}

// Function to check if the selected cards match
bool checkMatch(int x1, int y1, int x2, int y2) {
  return board[x1][y1] == board[x2][y2] && !(x1 == x2 && y1 == y2);
}
