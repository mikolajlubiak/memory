#pragma once

// libs
// FTXUI includes
#include <ftxui/component/component.hpp>          // for Component
#include <ftxui/component/component_options.hpp>  // for Renderer
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp>  // for text, vbox, hbox, separator
#include <ftxui/screen/screen.hpp> // for Screen

// std
#include <cstdint>
#include <vector>

class Memory {
public:
  enum gameStatusEnum {
    firstCard,
    secondCard,
    turnMatch,
    turnNoMatch,
    finished,
  };

  Memory(uint32_t size);

  void init();

  void loop();

private:
  // Function to initialize the game board
  void initializeBoard();

  ftxui::Element CreateBoard(const std::int32_t *const current_x,
                             const std::int32_t *const current_y,
                             const bool *const blink);

  // Function to check if the selected cards match
  bool checkMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                  std::uint32_t y2);

  ftxui::Component createRenderer();

  ftxui::Element createUI();

  ftxui::ComponentDecorator catchEvent();

  bool onEvent(ftxui::Event event);

  void async_blinking();

  std::uint32_t size;                        // Size of the board (4x4)
  std::uint32_t total_pairs;                 // Total pairs of cards
  std::vector<std::vector<char>> board{};    // board
  std::vector<std::vector<bool>> revealed{}; // revealed cards
  std::vector<std::vector<ftxui::Color>>
      revealedColors{}; // colors of revealed cards

  // blinking related code
  bool blink = false;
  std::chrono::milliseconds timerDuration = std::chrono::milliseconds(1000);
  std::chrono::milliseconds timer = timerDuration;
  std::chrono::time_point<std::chrono::steady_clock> now, old;

  gameStatusEnum gameStatus = firstCard;

  std::uint32_t pairsFound = 0;

  std::int32_t current_x = 0;
  std::int32_t current_y = 0;

  std::uint32_t old_x = 0;
  std::uint32_t old_y = 0;

  std::uint32_t temp_x = 0;
  std::uint32_t temp_y = 0;

  std::string message = "Select first card";
  ftxui::Decorator textStyle =
      ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);

  bool running = true;

  ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();

  ftxui::Component renderer;
};
