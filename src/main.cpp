// local
#include "common.h"
#include "memory.h"

// libs
// FTXUI includes
#include <ftxui/component/component.hpp>          // for Component
#include <ftxui/component/component_options.hpp>  // for Renderer
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp>  // for text, vbox, hbox, separator
#include <ftxui/screen/screen.hpp> // for Screen

// std
#include <cstring>
#include <future>
#include <iostream>
#include <thread>

enum gameStatusEnum {
  firstCard,
  secondCard,
  turnMatch,
  turnNoMatch,
  finished,
};

// Main game loop
int main() {
  // blinking related code
  bool blink = false;
  auto timerDuration = std::chrono::milliseconds(1000);
  auto timer = timerDuration;
  std::chrono::time_point<std::chrono::steady_clock> now, old;
  old = std::chrono::steady_clock::now();

  // game related code
  initializeBoard();

  gameStatusEnum gameStatus = firstCard;

  int pairsFound = 0;

  int current_x = 0;
  int current_y = 0;

  int old_x = 0;
  int old_y = 0;

  int temp_x = 0;
  int temp_y = 0;

  std::string message = "Select first card";
  ftxui::Decorator textStyle =
      ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);

  bool running = true;

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  auto renderer = ftxui::Renderer([&] {
    return ftxui::vbox({
        ftxui::hbox({
            ftxui::text("Memory Game") | ftxui::color(ftxui::Color::Grey100) |
                ftxui::bold,
            ftxui::separator(),

            ftxui::text("Pairs Found: "),
            ftxui::text(std::to_string(pairsFound)) | ftxui::blink,
            ftxui::separator(),

            ftxui::text(message) | textStyle,
        }) | ftxui::center,
        ftxui::separator(),

        CreateBoard(&current_x, &current_y, &blink) | ftxui::center,
    });
  });

  renderer |= ftxui::CatchEvent([&](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      screen.ExitLoopClosure()();
      running = false;
      return true;
    }

    if (event != ftxui::Event::Custom) {
      change_blink(&blink, &timer, false, timerDuration);
    }

    if (event == ftxui::Event::ArrowUp) {
      current_x--;
    }
    if (event == ftxui::Event::ArrowDown) {
      current_x++;
    }
    if (event == ftxui::Event::ArrowRight) {
      current_y++;
    }
    if (event == ftxui::Event::ArrowLeft) {
      current_y--;
    }

    current_x = std::clamp(current_x, 0, SIZE - 1);
    current_y = std::clamp(current_y, 0, SIZE - 1);

    if (event == ftxui::Event::Return && gameStatus != finished) {
      if (gameStatus == firstCard) {
        revealed[current_x][current_y] = true;

        old_x = current_x;
        old_y = current_y;
        message = "Select second card";

        gameStatus = secondCard;

        change_blink(&blink, &timer, true, timerDuration);

      } else if (gameStatus == secondCard) {
        if (old_x == current_x && old_y == current_y) {
          return false;
        }

        revealed[current_x][current_y] = true;

        if (checkMatch(current_x, current_y, old_x, old_y)) {
          pairsFound++;

          revealedColors[current_x][current_y] = ftxui::Color::Green;
          revealedColors[old_x][old_y] = ftxui::Color::Green;

          if (pairsFound < TOTAL_PAIRS) {
            message = "Select first card";

            gameStatus = firstCard;
          } else {
            message = "Congratulations! You've found all pairs!";
            textStyle = ftxui::bold | ftxui::color(ftxui::Color::Green);

            gameStatus = finished;
          }

        } else {
          revealedColors[current_x][current_y] = ftxui::Color::Red;
          revealedColors[old_x][old_y] = ftxui::Color::Red;

          message = "Cards don't match. Press enter to continue...";
          textStyle = ftxui::underlinedDouble | ftxui::color(ftxui::Color::Red);

          temp_x = current_x;
          temp_y = current_y;

          gameStatus = turnNoMatch;
        }

        change_blink(&blink, &timer, true, timerDuration);

      } else if (gameStatus == turnNoMatch) {
        revealed[temp_x][temp_y] = false; // Hide the first card again
        revealed[old_x][old_y] = false;   // Hide the second card again

        revealedColors[temp_x][temp_y] = ftxui::Color::YellowLight;
        revealedColors[old_x][old_y] = ftxui::Color::YellowLight;

        message = "Select first card";
        textStyle =
            ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);

        gameStatus = firstCard;

        change_blink(&blink, &timer, false, timerDuration);
      }
    }

    return false;
  });

  auto blinking_thread = std::async(std::launch::async, [&] {
    while (running) {
      now = std::chrono::steady_clock::now();
      timer -= std::chrono::duration_cast<decltype(timer)>(now - old);
      old = now;
      if (timer.count() < 0.0) {
        timer = timerDuration;
        blink = !blink;
        screen.PostEvent(ftxui::Event::Custom);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  screen.Loop(renderer);

  return 0;
}