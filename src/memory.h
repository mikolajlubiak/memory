#pragma once

// std
#include <chrono>

const int SIZE = 4;                        // Size of the board (4x4)
const int TOTAL_PAIRS = (SIZE * SIZE) / 2; // Total pairs of cards
extern char board[SIZE][SIZE];             // The board to display
extern char hidden[SIZE][SIZE];            // The hidden board
extern bool revealed[SIZE][SIZE];          // To track revealed cards

// Function to initialize the game board
void initializeBoard();

// Function to display the board
void displayBoard(const int *const x, const int *const y,
                  const char *const text_to_print, bool *const blink,
                  std::chrono::duration<float> *const timer);

// Function to check if the selected cards match
bool checkMatch(int x1, int y1, int x2, int y2);

void moveAcrossBoard(int *const x, int *const y, bool *const blink,
                     std::chrono::duration<float> *const timer,
                     const bool select = true);