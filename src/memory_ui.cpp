// header
#include "memory_ui.hpp"

// local
#include "common.hpp"
#include "slider_with_callback.hpp"

// std
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iterator>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace memory_game {

MemoryUI::MemoryUI() {
  create_dir(m_SaveDir);

  m_Screen.SetCursor(ftxui::Screen::Cursor{
      .x = 0, .y = 0, .shape = ftxui::Screen::Cursor::Hidden});

  m_PlayerCount = m_pGameLogic->GetPlayerCount();
}

// Create all needed components and loop
void MemoryUI::MainGame() {

  // Main component stacking all the others
  auto main_game_component = ftxui::Container::Stacked({
      ftxui::Maybe(GetOptionsWindow() | ftxui::vcenter | ftxui::flex,
                   &m_ShowOptions),

      ftxui::Maybe(GetLoadWindow() | ftxui::align_right | ftxui::vcenter,
                   [&] { return m_SaveList.size() > 0; }),

      GetSaveWindow() | ftxui::vcenter,

      ftxui::Maybe(GetShortcutsWindow(), &m_ShowShortcuts),

      GameBoardUI() | HandleMemoryEvents(),

      ftxui::Maybe(GetBackgroundComponent(), &m_AddBackground),
  });

  main_game_component |= HandleGlobalEvents();

  // Update/draw component in loop
  m_Screen.Loop(main_game_component);
}

// Handle game events and update game UI
ftxui::Component MemoryUI::GameBoardUI() const {
  return ftxui::Renderer([this](bool focus) { return CreateUI(); });
}

// Handle global events (shortcuts)
ftxui::ComponentDecorator MemoryUI::HandleGlobalEvents() {
  return ftxui::CatchEvent([this](ftxui::Event event) {
    if (event == ftxui::Event::Character('q')) {
      m_Screen.ExitLoopClosure()();
      return true;
    } else if (event == ftxui::Event::Character('r')) {
      m_pGameLogic->InitializeBoard();
      MessageAndStyleFromGameState();
      return true;
    } else if (event == ftxui::Event::Character('o')) {
      m_ShowOptions = !m_ShowOptions;
      return true;
    }

    return false;
  });
}

// Handle game specific events (arrows and enter)
ftxui::ComponentDecorator MemoryUI::HandleMemoryEvents() {
  return ftxui::CatchEvent([this](ftxui::Event event) {
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
ftxui::Element MemoryUI::CreateUI() const {
  return ftxui::window(
      ftxui::hbox({
          ftxui::text("Memory Game") | ftxui::color(ftxui::Color::Grey100) |
              ftxui::bold,
          ftxui::separator(),

          ftxui::text(
              "Player's " +
              std::to_string(m_pGameLogic->GetCurrentPlayerIndex() + 1) +
              " turn"),
          ftxui::separator(),

          ftxui::text("Player matched "),
          ftxui::text(std::to_string(m_pGameLogic->GetMatchedCardsCount(
              m_pGameLogic->GetCurrentPlayerIndex()))) |
              ftxui::blink,
          ftxui::text(" cards"),
          ftxui::separator(),

          ftxui::text("Turn number: " +
                      std::to_string(m_pGameLogic->GetTurnNumber())),
          ftxui::separator(),

          ftxui::text("Board size: " + std::to_string(m_BoardSize) + "x" +
                      std::to_string(m_BoardSize)),
          ftxui::separator(),

          ftxui::text("Player count: " +
                      std::to_string(m_pGameLogic->GetPlayerCount())),
          ftxui::separator(),

          ftxui::text(m_Message) | m_TextStyle,
      }) | ftxui::center,
      CreateBoard(m_CurrentX, m_CurrentY));
}

// Create gridbox of cards
ftxui::Element MemoryUI::CreateBoard(const std::int32_t current_x,
                                     const std::int32_t current_y) const {
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
      if (i == current_x && j == current_y) {
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
      m_Message = "Player " +
                  std::to_string(m_pGameLogic->GetWinners()[0] + 1) +
                  " won and a matched total of " +
                  std::to_string(m_pGameLogic->GetMatchedCardsCount(
                      m_pGameLogic->GetWinners()[0])) +
                  " cards. Congratulations!";
    } else {
      // Handle multiple winners
      std::string winnerNames;
      std::uint32_t matchedSum = 0;
      for (const auto &winner : m_pGameLogic->GetWinners()) {
        winnerNames += std::to_string(winner + 1) + ", ";
        matchedSum += m_pGameLogic->GetMatchedCardsCount(winner);
      }

      // Remove trailing comma and space
      if (!winnerNames.empty()) {
        winnerNames.pop_back();
        winnerNames.pop_back();
      }

      m_Message = "Players " + winnerNames + " won and a matched total of " +
                  std::to_string(matchedSum) + " cards. Congratulations!";
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

/* Components */

// Background
ftxui::Component MemoryUI::GetBackgroundComponent() const {
  // Mouse X and Y needed for dynamic background
  float mouse_y = 0.0f;
  float mouse_x = 0.0f;

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
      if (e.mouse().x > 1 || e.mouse().y > 1) {
        mouse_x = (e.mouse().x - 1) * 2;
        mouse_y = (e.mouse().y - 1) * 4;
      }
    }
    return false;
  });

  return background;
}

// Options window
ftxui::Component MemoryUI::GetOptionsWindow() {
  auto options_window =
      ftxui::Window({
          .inner =
              ftxui::Container::Vertical({
                  // Select board size
                  ftxui::Slider(
                      ftxui::text("Board size") |
                          ftxui::color(ftxui::Color::YellowLight),
                      ftxui::SliderWithCallbackOption<std::int32_t>{
                          .callback =
                              [&](std::int32_t board_size) {
                                m_BoardSize =
                                    static_cast<std::uint32_t>(board_size) * 2;

                                m_pGameLogic->SetBoardSize(m_BoardSize);
                              },
                          .value = 2,
                          .min = 1,
                          .max = 5,
                          .increment = 1,
                          .color_active = ftxui::Color::YellowLight,
                          .color_inactive = ftxui::Color::YellowLight,
                      }),

                  ftxui::Renderer([] {
                    return ftxui::filler();
                  }), // Make some space between components

                  // Select player count
                  ftxui::Slider(ftxui::text("Player count") |
                                    ftxui::color(ftxui::Color::YellowLight),
                                ftxui::SliderWithCallbackOption<std::int32_t>{
                                    .callback =
                                        [&](std::int32_t player_count) {
                                          m_pGameLogic->SetPlayerCount(
                                              static_cast<std::uint32_t>(
                                                  player_count));
                                        },
                                    .value = &m_PlayerCount,
                                    .min = 1,
                                    .max = 5,
                                    .increment = 1,
                                    .color_active = ftxui::Color::YellowLight,
                                    .color_inactive = ftxui::Color::YellowLight,
                                }),

                  ftxui::Renderer([] {
                    return ftxui::filler();
                  }), // Make some space between components

                  // Select whether to add background
                  ftxui::Checkbox("Background", &m_AddBackground) |
                      ftxui::center | ftxui::color(ftxui::Color::Yellow),

                  ftxui::Renderer([] {
                    return ftxui::separator();
                  }), // Separate select button from options

                  // Hide window
                  ftxui::Button("Hide", [&] { m_ShowOptions = false; }) |
                      ftxui::center | ftxui::color(ftxui::Color::Yellow),
              }) |
              ftxui::flex,

          .title = "Options",
          .left = 0,
          .width = 34,
          .height = 11,
      });

  return options_window;
}

// Save game window
ftxui::Component MemoryUI::GetSaveWindow() {
  auto save_window = ftxui::Window({
      .inner = ftxui::Button("Save",
                             [&] {
                               m_pGameLogic->SaveState(
                                   m_SaveDir / get_timestamp_filename());

                               m_ReadableSaveList =
                                   get_human_readable_file_list(m_SaveDir);
                               m_SaveList = get_file_list(m_SaveDir);

                               m_LoadWindowHeight =
                                   static_cast<int>(m_SaveList.size()) + 6;

                               MessageAndStyleFromGameState();
                             }) |
               ftxui::center | ftxui::flex | ftxui::color(ftxui::Color::Cyan),

      .title = "Save game",
      .width = 12,
      .height = 5,
  });

  return save_window;
}

// Load save window
ftxui::Component MemoryUI::GetLoadWindow() {
  // Load selected save
  auto load_select = [&] {
    m_pGameLogic->LoadState(m_SaveList[m_SelectedSave]);

    m_BoardSize = m_pGameLogic->GetBoardSize();

    MessageAndStyleFromGameState();
  };

  // Menu to select save to load
  ftxui::MenuOption menu_load_option;
  menu_load_option.on_enter = load_select;

  auto menu_load = Menu(&m_ReadableSaveList, &m_SelectedSave, menu_load_option);

  auto load_window = ftxui::Window({
      .inner = ftxui::Container::Vertical({
                   menu_load | ftxui::flex,
                   ftxui::Renderer([] { return ftxui::separator(); }),
                   ftxui::Button("Load", load_select) | ftxui::center,
               }) |
               ftxui::color(ftxui::Color::Cyan),

      .title = "Load game",
      .width = 25,
      .height = &m_LoadWindowHeight,
  });

  return load_window;
}

// Shortcuts window
ftxui::Component MemoryUI::GetShortcutsWindow() {
  return ftxui::Window({
      .inner = ftxui::Container::Vertical({
                   // Shortcuts
                   ftxui::Renderer([] {
                     return ftxui::vbox({
                         ftxui::text("q - Quit") | ftxui::flex,
                         ftxui::filler(),
                         ftxui::text("o - Open/hide options") | ftxui::flex,
                         ftxui::filler(),
                         ftxui::text("r - Reset the board state") | ftxui::flex,
                         ftxui::separator(),
                     });
                   }),
                   // Hide window
                   ftxui::Button("Hide", [&] { m_ShowShortcuts = false; }) |
                       ftxui::center,
               }) |
               ftxui::color(ftxui::Color::Violet),

      .title = "Shortcuts",
      .width = 28,
      .height = 9,
  });
}

} // namespace memory_game