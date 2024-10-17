// header
#include "memory.h"

// local
#include "common.h"

// std
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <random>
#include <thread>

void Memory::run() {
  renderer = createRenderer();
  renderer |= catchEvent();

  auto selector_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              ftxui::Slider("Slider", &size, 2, 10),
              ftxui::Button("Select",
                            [&] {
                              total_pairs = std::pow(size, 2) / 2;
                              initializeBoard();

                              screen.Loop(renderer);

                              message = "Select first card";
                              textStyle =
                                  ftxui::underlined |
                                  ftxui::color(ftxui::Color::LightYellow3);

                              gameStatus = firstCard;

                              pairsFound = 0;
                              current_x = 0;
                              current_y = 0;
                            }) |
                  ftxui::center,
          }),
      }) |
      ftxui::center;

  screen.SetCursor(ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));

  screen.Loop(selector_window);
}

// Function to initialize the game board
void Memory::initializeBoard() {
  std::vector<char> cards;
  for (char c = 'A'; c < 'A' + total_pairs; ++c) {
    cards.push_back(c);
    cards.push_back(c); // Add pairs
  }

  // Shuffle the cards using std::shuffle
  std::random_device rd;  // Obtain a random number from hardware
  std::mt19937 eng(rd()); // Seed the generator
  std::shuffle(cards.begin(), cards.end(),
               eng); // Shuffle the cards

  board.resize(size, std::vector<char>(size));
  revealed.resize(size, std::vector<bool>(size));
  revealedColors.resize(size, std::vector<ftxui::Color>(size));

  // Fill the boards
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      board[i][j] = cards[i * size + j];
      revealed[i][j] = false;
      revealedColors[i][j] = ftxui::Color::YellowLight;
    }
  }
}

// Function to create the board display
ftxui::Element Memory::CreateBoard(const std::int32_t *const current_x,
                                   const std::int32_t *const current_y,
                                   const bool *const blink) {
  auto rows = std::vector<ftxui::Element>();
  for (int i = 0; i < size; ++i) {
    auto cells = std::vector<ftxui::Element>();
    for (int j = 0; j < size; ++j) {
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
             ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 70 / size) |
             ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 35 / size);

      cells.push_back(cell);
    }
    rows.push_back(ftxui::hbox(cells));
  }
  return ftxui::vbox(rows);
}

// Function to check if the selected cards match
bool Memory::checkMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                        std::uint32_t y2) {
  return board[x1][y1] == board[x2][y2] && !(x1 == x2 && y1 == y2);
}

ftxui::Component Memory::createRenderer() {
  return ftxui::Renderer([this] { return createUI(); });
}

ftxui::Element Memory::createUI() {
  return ftxui::vbox({
             ftxui::hbox({
                 ftxui::text("Memory Game") |
                     ftxui::color(ftxui::Color::Grey100) | ftxui::bold,
                 ftxui::separator(),

                 ftxui::text("Pairs Found: "),
                 ftxui::text(std::to_string(pairsFound)) | ftxui::blink,
                 ftxui::separator(),

                 ftxui::text(message) | textStyle,
             }) | ftxui::center,
             ftxui::separator(),

             CreateBoard(&current_x, &current_y, &blink) | ftxui::center,
         }) |
         ftxui::center;
}

ftxui::ComponentDecorator Memory::catchEvent() {
  return ftxui::CatchEvent(
      [this](ftxui::Event event) { return onEvent(event); });
}

bool Memory::onEvent(ftxui::Event event) {
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

  current_x = std::clamp(current_x, 0, static_cast<std::int32_t>(size - 1));
  current_y = std::clamp(current_y, 0, static_cast<std::int32_t>(size - 1));

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

        if (pairsFound < total_pairs) {
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
      textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);

      gameStatus = firstCard;

      change_blink(&blink, &timer, false, timerDuration);
    }
  }

  return false;
}

void Memory::async_blinking() {
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
}
