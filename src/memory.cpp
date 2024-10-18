// header
#include "memory.h"

// local
#include "common.h"

// std
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <mutex>
#include <random>
#include <thread>

void Memory::run() {
  screen.SetCursor(ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));

  auto blinking_handle =
      std::async(std::launch::async, [this] { async_blinking(); });

  initializeBoard();

  renderer = createRenderer();
  renderer |= catchEvent();

  int temp_size = 2;

  auto slider_handle =
      std::async(std::launch::async, [&] { slider_changed(&temp_size); });

  auto selector_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
                       ftxui::Slider("Size:", &temp_size, 1, 5, 1),

                       ftxui::Button("Select",
                                     [&] {
                                       std::lock_guard<std::mutex> lock(mtx);

                                       selection_stage = false;

                                       size = temp_size * 2;

                                       total_pairs = std::pow(size, 2) / 2;

                                       initializeBoard();
                                     }) |
                           ftxui::center | ftxui::flex,
                   }) |
                   ftxui::flex,
      }) |
      ftxui::center | ftxui::flex;

  auto main_loop = ftxui::Container::Stacked(
      {ftxui::Maybe(selector_window, &selection_stage), renderer});

  screen.Loop(main_loop);
}

// Function to initialize the game board
void Memory::initializeBoard() {
  // Reset the game state
  board.clear();
  revealed.clear();
  revealedColors.clear();

  pairsFound = 0;
  current_x = 0;
  current_y = 0;
  message = "Select first card";
  textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
  gameStatus = firstCard;

  std::vector<char> cards;
  cards.reserve(total_pairs * 2);

  for (char c = 'A'; c < 'A' + total_pairs; ++c) {
    cards.emplace_back(c);
    cards.emplace_back(c); // Add pairs
  }

  // Shuffle the cards using std::shuffle
  std::random_device rd;  // Obtain a random number from hardware
  std::mt19937 eng(rd()); // Seed the generator
  std::shuffle(cards.begin(), cards.end(), eng); // Shuffle the cards

  board.resize(size, std::vector<char>(size));
  revealed.resize(size, std::vector<bool>(size, false));
  revealedColors.resize(
      size, std::vector<ftxui::Color>(size, ftxui::Color::YellowLight));

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
  std::vector<std::vector<ftxui::Element>> cells;
  cells.resize(size, std::vector<ftxui::Element>(size));

  for (int i = 0; i < size; ++i) {
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
      // cell = cell | ftxui::center | ftxui::border | ftxui::bold |
      //       ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 60 / size) |
      //       ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 30 / size);
      cell =
          cell | ftxui::center | ftxui::border | ftxui::bold |
          ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 120.0f / size) |
          ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN,
                      120.0f / size) | // least common multiple of {2,4,6,8,10}
          ftxui::flex;

      cells[i][j] = cell;
    }
  }
  return ftxui::gridbox(cells) | ftxui::center | ftxui::flex;
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
  return ftxui::window(
             ftxui::hbox({
                 ftxui::text("Memory Game") |
                     ftxui::color(ftxui::Color::Grey100) | ftxui::bold,
                 ftxui::separator(),

                 ftxui::text("Pairs Found: "),
                 ftxui::text(std::to_string(pairsFound)) | ftxui::blink,
                 ftxui::separator(),

                 ftxui::text(message) | textStyle,
             }) | ftxui::center,
             CreateBoard(&current_x, &current_y, &blink)) |
         ftxui::flex;
}

ftxui::ComponentDecorator Memory::catchEvent() {
  return ftxui::CatchEvent(
      [this](ftxui::Event event) { return onEvent(event); });
}

bool Memory::onEvent(ftxui::Event event) {
  std::lock_guard<std::mutex> lock(mtx);

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
    checkXY();
    return true;
  }
  if (event == ftxui::Event::ArrowDown) {
    current_x++;
    checkXY();
    return true;
  }
  if (event == ftxui::Event::ArrowRight) {
    current_y++;
    checkXY();
    return true;
  }
  if (event == ftxui::Event::ArrowLeft) {
    current_y--;
    checkXY();
    return true;
  }

  if (event == ftxui::Event::Return && gameStatus != finished) {
    if (gameStatus == firstCard) {
      change_blink(&blink, &timer, true, timerDuration);

      revealed[current_x][current_y] = true;
      old_x = current_x;
      old_y = current_y;
      message = "Select second card";
      gameStatus = secondCard;

      return true;

    } else if (gameStatus == secondCard) {
      change_blink(&blink, &timer, true, timerDuration);

      if (old_x == current_x && old_y == current_y) {
        return true;
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

      return true;

    } else if (gameStatus == turnNoMatch) {
      change_blink(&blink, &timer, false, timerDuration);

      revealed[temp_x][temp_y] = false; // Hide the first card again
      revealed[old_x][old_y] = false;   // Hide the second card again

      revealedColors[temp_x][temp_y] = ftxui::Color::YellowLight;
      revealedColors[old_x][old_y] = ftxui::Color::YellowLight;

      message = "Select first card";
      textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);

      gameStatus = firstCard;

      return true;
    }
  }

  return false;
}

void Memory::async_blinking() {
  old = std::chrono::steady_clock::now();

  while (running) {
    {
      std::lock_guard<std::mutex> lock(mtx);

      now = std::chrono::steady_clock::now();
      timer -= std::chrono::duration_cast<decltype(timer)>(now - old);
      old = now;
    }

    if (timer.count() < 0.0) {
      {
        std::lock_guard<std::mutex> lock(mtx);

        timer = timerDuration;
        blink = !blink;
      }

      screen.PostEvent(ftxui::Event::Custom);
    }

    std::this_thread::sleep_for(timerDuration + std::chrono::milliseconds(10));
  }
}

void Memory::slider_changed(const int *const temp_size) {
  int old_size = *temp_size;

  while (running && selection_stage) {
    if (old_size != *temp_size) {
      {
        std::lock_guard<std::mutex> lock(mtx);

        old_size = *temp_size;
        size = old_size * 2;
        total_pairs = std::pow(size, 2) / 2;

        initializeBoard();
      }

      screen.PostEvent(ftxui::Event::Custom);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void Memory::checkXY() {
  current_x = std::clamp(current_x, 0, static_cast<std::int32_t>(size - 1));
  current_y = std::clamp(current_y, 0, static_cast<std::int32_t>(size - 1));
}
