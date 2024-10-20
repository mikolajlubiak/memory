// header
#include "memory_ui.hpp"

// local
#include "common.hpp"
#include "slider_with_callback.hpp"

// std
#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <future>
#include <iterator>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace memory_game {

MemoryUI::MemoryUI() {
  m_DebugStream.open("debug_output.txt", std::ios::app);

  m_Screen.SetCursor(
      ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));

  m_Renderer = CreateRenderer();
  m_Renderer |= HandleEvents();

  MainGame();
}

// Create all needed components and loop
void MemoryUI::MainGame() {
  // Mouse X and Y needed for dynamic background
  float mouse_y = 0.0f;
  float mouse_x = 0.0f;

  // Background
  auto background = ftxui::Renderer([&] {
    // Scale to fit canvas 2x4 braille dot
    std::uint32_t width = m_Screen.dimx() * 2;
    std::uint32_t height = m_Screen.dimy() * 4;

    // Initialize dynamic canvas component
    auto c = ftxui::Canvas(width, height);

    // Set size and transformation offset
    std::uint32_t size = std::max(width, height);
    std::int32_t offset = size;

    // Black magic math
    float my = (mouse_y - offset) / -5.f;
    float mx = (mouse_x - 3 * my + offset) / 5.f;

    std::vector<std::vector<float>> ys(size, std::vector<float>(size));

    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        float dx = x - mx;
        float dy = y - my;
        ys[y][x] = -1.5 + 3.0 * std::exp(-0.2f * (dx * dx + dy * dy));
      }
    }
    for (int y = 0; y < size; y++) {
      for (int x = 0; x < size; x++) {
        if (x != 0) {
          c.DrawPointLine(5 * (x - 1) + 3 * (y - 0) - offset,
                          offset - 5 * (y - 0) - 5 * ys[y][x - 1],
                          5 * (x - 0) + 3 * (y - 0) - offset,
                          offset - 5 * (y - 0) - 5 * ys[y][x]);
        }
        if (y != 0) {
          c.DrawPointLine(5 * (x - 0) + 3 * (y - 1) - offset,
                          offset - 5 * (y - 1) - 5 * ys[y - 1][x],
                          5 * (x - 0) + 3 * (y - 0) - offset,
                          offset - 5 * (y - 0) - 5 * ys[y][x]);
        }
      }
    }

    return ftxui::canvas(std::move(c));
  });

  // Scale mouse coordinates and update variables
  background |= ftxui::CatchEvent([&](ftxui::Event e) {
    if (e.is_mouse()) {
      mouse_x = (e.mouse().x - 1) * 2;
      mouse_y = (e.mouse().y - 1) * 4;
    }
    return false;
  });

  // Player count
  std::int32_t player_count = m_pGameLogic->GetPlayerCount();

  bool is_selection_stage = true; // Is the size selected (NOT)

  // Whether to add background or not
  bool add_background = false;

  // Select options
  auto options_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              // Select board size
              ftxui::Slider(ftxui::text("Board size") |
                                ftxui::color(ftxui::Color::YellowLight),
                            ftxui::SliderWithCallbackOption<std::int32_t>{
                                .callback =
                                    [&](std::int32_t board_size) {
                                      m_BoardSize = static_cast<std::uint32_t>(
                                                        board_size) *
                                                    2;

                                      m_pGameLogic->SetBoardSize(m_BoardSize);
                                    },
                                .value = 2,
                                .min = 1,
                                .max = 5,
                                .increment = 1,
                                .color_active = ftxui::Color::YellowLight,
                                .color_inactive = ftxui::Color::YellowLight,
                            }),

              // Select player count
              ftxui::Slider(ftxui::text("Player count") |
                                ftxui::color(ftxui::Color::YellowLight),
                            ftxui::SliderOption<std::int32_t>{
                                .value = &player_count,
                                .min = 1,
                                .max = 5,
                                .increment = 1,
                                .color_active = ftxui::Color::YellowLight,
                                .color_inactive = ftxui::Color::YellowLight,
                            }),

              // Select whether to add background
              ftxui::Checkbox("Background", &add_background) | ftxui::center |
                  ftxui::flex | ftxui::color(ftxui::Color::Yellow),

              ftxui::Renderer([] {
                return ftxui::separator();
              }), // Separate select button from options

              // Select/save options
              ftxui::Button("Select",
                            [&] {
                              m_pGameLogic->SetPlayerCount(
                                  static_cast<std::uint32_t>(player_count));

                              is_selection_stage = false;
                            }) |
                  ftxui::center | ftxui::flex |
                  ftxui::color(ftxui::Color::Yellow),
          }),

          .title = "Options",
          .left = 0,
          .width = 30,
          .height = 9,
      }) |
      ftxui::vcenter;

  // Save window
  auto save_window =
      ftxui::Window({
          .inner =
              ftxui::Button("Save",
                            [&] {
                              m_pGameLogic->SaveState(get_timestamp_filename());

                              MessageAndStyleFromGameState();
                            }) |
              ftxui::center | ftxui::flex | ftxui::color(ftxui::Color::Cyan),

          .title = "Save game",
          .width = 12,
          .height = 5,
      }) |
      ftxui::vcenter;

  // Get saves
  auto readable_saves_list = get_human_readable_file_list("saves/");
  auto saves_list = get_file_list("saves/");
  int selected_save = 0; // Index of selected file

  // Load selected save
  auto load_select = [&] {
    m_pGameLogic->LoadState(saves_list[selected_save]);

    m_BoardSize = m_pGameLogic->GetBoardSize();

    MessageAndStyleFromGameState();

    is_selection_stage = false;
  };

  // Menu to select save to load
  ftxui::MenuOption menu_load_option;
  menu_load_option.on_enter = load_select;

  auto menu_load = Menu(&readable_saves_list, &selected_save, menu_load_option);

  // Load window
  auto load_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
                       menu_load,
                       ftxui::Renderer([] { return ftxui::separator(); }),
                       ftxui::Button("Load", load_select) | ftxui::center |
                           ftxui::flex,
                   }) |
                   ftxui::color(ftxui::Color::Cyan),

          .title = "Load game",
          .width = 25,
          .height = static_cast<int>(saves_list.size()) + 6,
      }) |
      ftxui::align_right | ftxui::vcenter | ftxui::flex;

  // Main component stacking all the others
  auto main_game_component = ftxui::Container::Stacked({
      // selection stage
      ftxui::Maybe(options_window, &is_selection_stage),

      // game
      ftxui::Maybe(load_window, [&] { return saves_list.size() > 0; }),
      ftxui::Maybe(save_window, [&] { return !is_selection_stage; }),
      m_Renderer,
      ftxui::Maybe(background, &add_background),
  });

  // Update/draw component in loop
  m_Screen.Loop(main_game_component);
}

// Handle game events and update game UI
ftxui::Component MemoryUI::CreateRenderer() {
  return ftxui::Renderer([this](bool focus) { return CreateUI(); });
}

// Handle events (arrows and enter)
ftxui::ComponentDecorator MemoryUI::HandleEvents() {
  return ftxui::CatchEvent([this](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      m_Screen.ExitLoopClosure()();
      return true;
    } else if (event == ftxui::Event::Character('r')) {
      m_pGameLogic->InitializeBoard();
      return true;
    }

    if (event == ftxui::Event::ArrowUp) {
      m_CurrentX--;
      CheckBoundsXY();
      return true;
    }
    if (event == ftxui::Event::ArrowDown) {
      m_CurrentX++;
      CheckBoundsXY();
      return true;
    }
    if (event == ftxui::Event::ArrowRight) {
      m_CurrentY++;
      CheckBoundsXY();
      return true;
    }
    if (event == ftxui::Event::ArrowLeft) {
      m_CurrentY--;
      CheckBoundsXY();
      return true;
    }

    if (event == ftxui::Event::Return) {
      m_pGameLogic->SelectCard(m_CurrentX, m_CurrentY);

      MessageAndStyleFromGameState();

      return true;
    }

    return false;
  });
}

// Clamp m_CurrentX and m_CurrentY
void MemoryUI::CheckBoundsXY() {
  m_CurrentX =
      std::clamp(m_CurrentX, 0, static_cast<std::int32_t>(m_BoardSize - 1));
  m_CurrentY =
      std::clamp(m_CurrentY, 0, static_cast<std::int32_t>(m_BoardSize - 1));
}

// Create static UI game element
ftxui::Element MemoryUI::CreateUI() {
  return ftxui::window(
      ftxui::hbox({
          ftxui::text("Memory Game") | ftxui::color(ftxui::Color::Grey100) |
              ftxui::bold,
          ftxui::separator(),

          ftxui::text(std::format("Player's {} turn",
                                  m_pGameLogic->GetCurrentPlayerIndex() + 1)),
          ftxui::separator(),

          ftxui::text("Cards matched: "),
          ftxui::text(std::to_string(m_pGameLogic->GetMatchedCardsCount())) |
              ftxui::blink,
          ftxui::separator(),

          ftxui::text(m_Message) | m_TextStyle,
      }) | ftxui::center,
      CreateBoard(&m_CurrentX, &m_CurrentY));
}

// Create gridbox of cards
ftxui::Element MemoryUI::CreateBoard(const std::int32_t *const current_x,
                                     const std::int32_t *const current_y) {
  std::vector<std::vector<ftxui::Element>> cells;
  cells.resize(m_BoardSize, std::vector<ftxui::Element>(m_BoardSize));

  for (int i = 0; i < m_BoardSize; ++i) {
    for (int j = 0; j < m_BoardSize; ++j) {
      ftxui::Element cell;
      ftxui::Decorator color;

      // Determine the content of the cell
      if (m_pGameLogic->GetHasCardBeenRevealed()[i][j]) {
        cell = ftxui::text(std::string(1, m_pGameLogic->GetBoard()[i][j]));
        color = ftxui::color(ftxui::Color::White);
      } else {
        cell = ftxui::text("*");
        color = ftxui::color(ftxui::Color::Grey50);
      }

      // If the cell is the one user selected light it in blue
      if (i == *current_x && j == *current_y) {
        color = ftxui::color(ftxui::Color::Blue);
      } else if (m_pGameLogic->GetHasCardBeenMatched()[i][j]) {
        color = ftxui::color(ftxui::Color::Green);
      }

      cell = cell | ftxui::bold | ftxui::center | ftxui::border | color |
             ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN,
                         std::ceil(60.0f / m_BoardSize)) |
             ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN,
                         std::ceil(30.0f / m_BoardSize));

      cells[i][j] = cell;
    }
  }
  return ftxui::gridbox(cells) | ftxui::center;
}

// Update m_Message and m_TextStyle based on the game state
void MemoryUI::MessageAndStyleFromGameState() {
  switch (m_pGameLogic->GetGameStatus()) {
  case GameStatus::selectingFirstCard:
    m_Message = "Select first card";
    m_TextStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  case GameStatus::selectingSecondCard:
    m_Message = "Select second card";
    m_TextStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  case GameStatus::gameFinished:
    if (m_pGameLogic->GetWinners().size() == 1) {
      m_Message = std::format("Player {} won. Congratulations!",
                              m_pGameLogic->GetWinners()[0] + 1);
    } else {
      // Handle multiple winners
      std::string winnerNames;
      for (const auto &winner : m_pGameLogic->GetWinners()) {
        winnerNames += std::to_string(winner + 1) + ", ";
      }

      // Remove trailing comma and space
      if (!winnerNames.empty()) {
        winnerNames.pop_back();
        winnerNames.pop_back();
      }

      m_Message = std::format("Players {} won. Congratulations!", winnerNames);
    }

    m_TextStyle = ftxui::bold | ftxui::color(ftxui::Color::Green);
    break;
  case GameStatus::cardsDidntMatch:
    m_Message = "Cards don't match. Press enter to continue...";
    m_TextStyle = ftxui::underlinedDouble | ftxui::color(ftxui::Color::Red);
    break;
  default:
    m_Message = "Select first card";
    m_TextStyle = ftxui::underlined | ftxui::color(ftxui::Color::LightYellow3);
    break;
  }
}

} // namespace memory_game