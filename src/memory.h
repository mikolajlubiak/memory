#pragma once

// libs
// FTXUI includes
#include <ftxui/component/component.hpp>          // for Component
#include <ftxui/component/component_options.hpp>  // for Renderer
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp>  // for text, vbox, hbox, separator
#include <ftxui/screen/screen.hpp> // for Screen

const int SIZE = 4;                             // Size of the board (4x4)
const int TOTAL_PAIRS = (SIZE * SIZE) / 2;      // Total pairs of cards
extern char board[SIZE][SIZE];                  // The board to display
extern bool revealed[SIZE][SIZE];               // To track revealed cards
extern ftxui::Color revealedColors[SIZE][SIZE]; // To track card colors

// Function to initialize the game board
void initializeBoard();

ftxui::Element CreateBoard(const int *const current_x,
                           const int *const current_y, const bool *const blink);

// Function to check if the selected cards match
bool checkMatch(int x1, int y1, int x2, int y2);
