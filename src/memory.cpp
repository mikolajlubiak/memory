// header
#include "memory.h"

// local
#include "common.h"
#include "slider_with_callback.h"

// std
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iterator>
#include <mutex>
#include <random>
#include <thread>

void Memory::run() {
  debug_output.open("debug_output.txt", std::ios::app);

  screen.SetCursor(ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));

  auto blinking_handle =
      std::async(std::launch::async, [this] { async_blinking(); });

  initializeBoard();

  renderer = createRenderer();
  renderer |= catchEvent();

  int temp_size = 2;

  auto selector_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              ftxui::Slider(ftxui::SliderWithCallbackOption<int>{
                  .callback =
                      [&](int value) {
                        std::lock_guard<std::mutex> lock(mtx);

                        size = value * 2;
                        total_pairs = std::pow(size, 2) / 2;

                        initializeBoard();
                      },
                  .value = &temp_size,
                  .min = 1,
                  .max = 5,
                  .increment = 1,
                  .color_active = ftxui::Color::White,
                  .color_inactive = ftxui::Color::White,
              }),

              ftxui::Button("Select",
                            [&] {
                              std::lock_guard<std::mutex> lock(mtx);

                              selection_stage = false;
                            }) |
                  ftxui::center | ftxui::flex,
          }),

          .title = "Select board size",
          .left = 0,
          .width = 20,
          .height = 8,
      }) |
      ftxui::vcenter;

  auto save_window =
      ftxui::Window({
          .inner = ftxui::Button("Save",
                                 [&] {
                                   std::lock_guard<std::mutex> lock(mtx);

                                   saveState(get_timestamp_filename());
                                 }) |
                   ftxui::center | ftxui::flex,

          .title = "Save game",
          .width = 10,
          .height = 5,
      }) |
      ftxui::align_right | ftxui::vcenter;

  auto readable_saves_list = get_human_readable_file_list("saves/");
  auto saves_list = get_file_list("saves/");
  int selectedSave = 0;

  auto load_select = [&] {
    std::lock_guard<std::mutex> lock(mtx);

    loadState(saves_list[selectedSave]);

    selection_stage = false;
  };

  ftxui::MenuOption menu_load_option;
  menu_load_option.on_enter = load_select;

  auto menu_load = Menu(&readable_saves_list, &selectedSave, menu_load_option);

  auto load_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              menu_load,
              ftxui::Button("Load", load_select) | ftxui::center | ftxui::flex,
          }),

          .title = "Load game",
          .width = 10 * (saves_list.size() / 2) + 15,
          .height = 5 * (saves_list.size() / 3) + 7,
      }) |
      ftxui::align_right | ftxui::vcenter | ftxui::flex;

  auto main_loop = ftxui::Container::Stacked({
      // selection stage
      ftxui::Maybe(selector_window, &selection_stage),
      ftxui::Maybe(load_window,
                   [&] { return selection_stage && saves_list.size() != 0; }),

      // game
      ftxui::Maybe(save_window, [&] { return !selection_stage; }),
      renderer,
  });

  screen.Loop(main_loop);
}

// Function to initialize the game board
void Memory::initializeBoard() {
  resetState();

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
      cell = cell | ftxui::center | ftxui::border | ftxui::bold |
             ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN,
                         std::ceil(60.0f / size)) |
             ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN,
                         std::ceil(30.0f / size));

      cells[i][j] = cell;
    }
  }
  return ftxui::gridbox(cells) | ftxui::center;
}

// Function to check if the selected cards match
bool Memory::checkMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                        std::uint32_t y2) {
  return board[x1][y1] == board[x2][y2] && !(x1 == x2 && y1 == y2);
}

ftxui::Component Memory::createRenderer() {
  return ftxui::Renderer([this](bool focus) { return createUI(); });
}

ftxui::Element Memory::createUI() {
  return ftxui::window(
      ftxui::hbox({
          ftxui::text("Memory Game") | ftxui::color(ftxui::Color::Grey100) |
              ftxui::bold,
          ftxui::separator(),

          ftxui::text("Pairs Found: "),
          ftxui::text(std::to_string(pairsFound)) | ftxui::blink,
          ftxui::separator(),

          ftxui::text(message) | textStyle,
      }) | ftxui::center,
      CreateBoard(&current_x, &current_y, &blink));
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

      if (revealed[current_x][current_y]) {
        return true;
      }

      revealed[current_x][current_y] = true;
      old_x = current_x;
      old_y = current_y;

      gameStatus = secondCard;
      gameStateTextAndStyle();

      return true;

    } else if (gameStatus == secondCard) {
      change_blink(&blink, &timer, true, timerDuration);

      if ((old_x == current_x && old_y == current_y) ||
          revealed[current_x][current_y]) {
        return true;
      }

      revealed[current_x][current_y] = true;

      if (checkMatch(current_x, current_y, old_x, old_y)) {
        pairsFound++;

        revealedColors[current_x][current_y] = ftxui::Color::Green;
        revealedColors[old_x][old_y] = ftxui::Color::Green;

        if (pairsFound < total_pairs) {
          gameStatus = firstCard;
          gameStateTextAndStyle();
        } else {
          gameStatus = finished;
          gameStateTextAndStyle();
        }

      } else {
        revealedColors[current_x][current_y] = ftxui::Color::Red;
        revealedColors[old_x][old_y] = ftxui::Color::Red;

        temp_x = current_x;
        temp_y = current_y;

        gameStatus = turnNoMatch;
        gameStateTextAndStyle();
      }

      return true;

    } else if (gameStatus == turnNoMatch) {
      change_blink(&blink, &timer, false, timerDuration);

      revealed[temp_x][temp_y] = false; // Hide the first card again
      revealed[old_x][old_y] = false;   // Hide the second card again

      revealedColors[temp_x][temp_y] = ftxui::Color::YellowLight;
      revealedColors[old_x][old_y] = ftxui::Color::YellowLight;

      gameStatus = firstCard;
      gameStateTextAndStyle();

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

void Memory::checkXY() {
  current_x = std::clamp(current_x, 0, static_cast<std::int32_t>(size - 1));
  current_y = std::clamp(current_y, 0, static_cast<std::int32_t>(size - 1));
}

void Memory::saveState(const std::string &filename) {

  if (gameStatus == turnNoMatch) {
    revealed[temp_x][temp_y] = false; // Hide the first card again
    revealed[old_x][old_y] = false;   // Hide the second card again

    revealedColors[temp_x][temp_y] = ftxui::Color::YellowLight;
    revealedColors[old_x][old_y] = ftxui::Color::YellowLight;

    gameStatus = firstCard;
    gameStateTextAndStyle();
  }

  std::ofstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    debug_output << "Unable to open file: " << filename << std::endl;
    return;
  }

  std::size_t message_size = message.size();

  file.write(reinterpret_cast<const char *>(&size), sizeof(size));
  file.write(reinterpret_cast<const char *>(&gameStatus), sizeof(gameStatus));
  file.write(reinterpret_cast<const char *>(&pairsFound), sizeof(pairsFound));

  // No need to store the message
  /*
  file.write(reinterpret_cast<const char *>(&message_size),
             sizeof(message_size));
  file.write(reinterpret_cast<const char *>(&message[0]),
             message_size * sizeof(char));
  */

  for (int i = 0; i < size; i++) {
    file.write(reinterpret_cast<const char *>(&board[i][0]),
               board[i].size() * sizeof(board[i][0]));

    file.write(reinterpret_cast<const char *>(&revealedColors[i][0]),
               revealedColors[i].size() * sizeof(revealedColors[i][0]));

    for (int j = 0; j < size; j++) {
      char temp = static_cast<char>(revealed[i][j]);
      file.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
    }
  }

  file.close();
}

void Memory::loadState(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {

    debug_output << "Unable to open file: " << filename << std::endl;
    return;
  }

  std::size_t message_size;

  file.read(reinterpret_cast<char *>(&size), sizeof(size));
  file.read(reinterpret_cast<char *>(&gameStatus), sizeof(gameStatus));
  file.read(reinterpret_cast<char *>(&pairsFound), sizeof(pairsFound));

  /*
  file.read(reinterpret_cast<char *>(&message_size), sizeof(message_size));
  file.read(reinterpret_cast<char *>(&message[0]), message_size * sizeof(char));
  */

  total_pairs = std::pow(size, 2) / 2;
  gameStateTextAndStyle();

  board.resize(size, std::vector<char>(size));
  revealed.resize(size, std::vector<bool>(size));
  revealedColors.resize(size, std::vector<ftxui::Color>(size));

  for (int i = 0; i < size; i++) {
    file.read(reinterpret_cast<char *>(&board[i][0]),
              size * sizeof(board[i][0]));

    file.read(reinterpret_cast<char *>(&revealedColors[i][0]),
              size * sizeof(revealedColors[i][0]));

    for (int j = 0; j < size; j++) {
      char temp;
      file.read(reinterpret_cast<char *>(&temp), sizeof(temp));
      revealed[i][j] = static_cast<bool>(temp);
    }
  }

  file.close();
}

void Memory::resetState() {
  // Reset the game state
  board.clear();
  revealed.clear();
  revealedColors.clear();

  pairsFound = 0;
  current_x = 0;
  current_y = 0;
  gameStatus = firstCard;
  gameStateTextAndStyle();
}

void Memory::gameStateTextAndStyle() {
  switch (gameStatus) {
  case firstCard:
    message = "Select first card";
    textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  case secondCard:
    message = "Select second card";
    textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  case finished:
    message = "Congratulations! You've found all pairs!";
    textStyle = ftxui::bold | ftxui::color(ftxui::Color::Green);
    break;
  case turnNoMatch:
    message = "Cards don't match. Press enter to continue...";
    textStyle = ftxui::underlinedDouble | ftxui::color(ftxui::Color::Red);
    break;
  default:
    message = "Select first card";
    textStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  }
}