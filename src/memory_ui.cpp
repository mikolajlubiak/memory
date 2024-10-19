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
  m_DebugStream.open("debug_output.txt", std::ios::app);

  m_Screen.SetCursor(
      ftxui::Screen::Cursor(0, 0, ftxui::Screen::Cursor::Hidden));

  auto blinking_handle =
      std::async(std::launch::async, [this] { AsyncBlinkingUpdater(); });

  m_Renderer = CreateRenderer();
  m_Renderer |= HandleEvents();

  MainGame();
}

// Create all needed components and loop
void MemoryUI::MainGame() {
  auto selector_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              ftxui::Slider(ftxui::SliderWithCallbackOption<int>{
                  .callback =
                      [&](int value) {
                        std::lock_guard<std::mutex> lock(m_Lock);

                        m_BoardSize = value * 2;

                        m_pGameLogic =
                            std::make_unique<MemoryLogic>(m_BoardSize);
                      },
                  .value = 2,
                  .min = 1,
                  .max = 5,
                  .increment = 1,
                  .color_active = ftxui::Color::White,
                  .color_inactive = ftxui::Color::White,
              }),

              ftxui::Button("Select",
                            [&] {
                              std::lock_guard<std::mutex> lock(m_Lock);

                              m_IsSelectionStage = false;
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
                                   std::lock_guard<std::mutex> lock(m_Lock);

                                   m_pGameLogic->SaveState(
                                       get_timestamp_filename());

                                   MessageAndStyleFromGameState();
                                 }) |
                   ftxui::center | ftxui::flex,

          .title = "Save game",
          .width = 10,
          .height = 5,
      }) |
      ftxui::align_right | ftxui::vcenter;

  auto readable_saves_list = get_human_readable_file_list("saves/");
  auto saves_list = get_file_list("saves/");
  int selected_save = 0;

  auto load_select = [&] {
    std::lock_guard<std::mutex> lock(m_Lock);

    m_BoardSize = m_pGameLogic->LoadState(saves_list[selected_save]);

    MessageAndStyleFromGameState();

    m_IsSelectionStage = false;
  };

  ftxui::MenuOption menu_load_option;
  menu_load_option.on_enter = load_select;

  auto menu_load = Menu(&readable_saves_list, &selected_save, menu_load_option);

  auto load_window =
      ftxui::Window({
          .inner = ftxui::Container::Vertical({
              menu_load,
              ftxui::Button("Load", load_select) | ftxui::center | ftxui::flex,
          }),

          .title = "Load game",
          .width = static_cast<int>(10 * (saves_list.size() / 2) + 15),
          .height = static_cast<int>(5 * (saves_list.size() / 3) + 7),
      }) |
      ftxui::align_right | ftxui::vcenter | ftxui::flex;

  auto main_game_component = ftxui::Container::Stacked({
      // selection stage
      ftxui::Maybe(selector_window, &m_IsSelectionStage),
      ftxui::Maybe(
          load_window,
          [&] { return m_IsSelectionStage && saves_list.size() != 0; }),

      // game
      ftxui::Maybe(save_window, [&] { return !m_IsSelectionStage; }),
      m_Renderer,
  });

  m_Screen.Loop(main_game_component);
}

// Handle game events and update game UI
ftxui::Component MemoryUI::CreateRenderer() {
  return ftxui::Renderer([this](bool focus) { return CreateUI(); });
}

// Handle events (arrows and enter)
ftxui::ComponentDecorator MemoryUI::HandleEvents() {
  return ftxui::CatchEvent([this](ftxui::Event event) {
    std::lock_guard<std::mutex> lock(m_Lock);

    if (event == ftxui::Event::Character('q')) {
      m_Screen.ExitLoopClosure()();
      m_ShouldRun = false;
      return true;
    }

    if (event != ftxui::Event::Custom) {
      OverrideBlinking(false, m_TimerDuration);
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
      bool should_blink = m_pGameLogic->SelectCard(m_CurrentX, m_CurrentY);
      OverrideBlinking(should_blink, m_TimerDuration);

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

          ftxui::text("Pairs Found: "),
          ftxui::text(std::to_string(m_pGameLogic->GetPairsFoundCount())) |
              ftxui::blink,
          ftxui::separator(),

          ftxui::text(m_Message) | m_TextStyle,
      }) | ftxui::center,
      CreateBoard(&m_CurrentX, &m_CurrentY, &m_ShouldBlink));
}

// Create gridbox of cards
ftxui::Element MemoryUI::CreateBoard(const std::int32_t *const current_x,
                                     const std::int32_t *const current_y,
                                     const bool *const blink) {
  std::vector<std::vector<ftxui::Element>> cells;
  cells.resize(m_BoardSize, std::vector<ftxui::Element>(m_BoardSize));

  for (int i = 0; i < m_BoardSize; ++i) {
    for (int j = 0; j < m_BoardSize; ++j) {
      ftxui::Element cell;

      // Determine the content of the cell
      if (i == *current_x && j == *current_y && !(*blink)) {
        cell = ftxui::text("_") | ftxui::color(ftxui::Color::Blue);

      } else if (m_pGameLogic->GetRevealed()[i][j]) {
        cell = ftxui::text(std::string(1, m_pGameLogic->GetBoard()[i][j])) |
               ftxui::color(ftxui::Color::White);

      } else {
        cell = ftxui::text("*") | ftxui::color(ftxui::Color::YellowLight);
      }

      cell = cell | ftxui::center | ftxui::border | ftxui::bold |
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
    m_Message = "Congratulations! You've found all pairs!";
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

// Function to update blinker bool that will be launched asynchronously
void MemoryUI::AsyncBlinkingUpdater() {
  m_TimeLast = std::chrono::steady_clock::now();

  while (m_ShouldRun) {
    {
      std::lock_guard<std::mutex> lock(m_Lock);

      m_TimeNow = std::chrono::steady_clock::now();
      m_Timer -=
          std::chrono::duration_cast<decltype(m_Timer)>(m_TimeNow - m_TimeLast);
      m_TimeLast = m_TimeNow;
    }

    if (m_Timer.count() < 0.0) {
      {
        std::lock_guard<std::mutex> lock(m_Lock);

        m_Timer = m_TimerDuration;
        m_ShouldBlink = !m_ShouldBlink;
      }

      m_Screen.PostEvent(ftxui::Event::Custom);
    }

    std::this_thread::sleep_for(m_TimerDuration +
                                std::chrono::milliseconds(10));
  }
}

} // namespace memory_game