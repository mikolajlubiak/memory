#pragma once

// local
#include "common.hpp"
#include "memory_logic.hpp"

// libs
// FTXUI includes
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

// std
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>

namespace memory_game {

// Create UI to interact with game logic
class MemoryUI {
public:
  MemoryUI();

private: // Methods
  // Create all needed components and loop
  void MainGame();

  // Handle game events and update game UI
  ftxui::Component CreateRenderer() const;

  // Handle events (arrows and enter)
  ftxui::ComponentDecorator HandleEvents();

  // Clamp m_CurrentX and m_CurrentY
  void CheckBoundsXY();

  // Create static UI game element
  ftxui::Element CreateUI() const;

  // Create gridbox of cards
  ftxui::Element CreateBoard(const std::int32_t current_x,
                             const std::int32_t current_y) const;

  // Update m_Message and m_TextStyle based on the game state
  void MessageAndStyleFromGameState();

  // Components
  // Background
  ftxui::Component GetBackgroundComponent() const;
  // Options window
  ftxui::Component GetOptionsWindow();
  // Save game window
  ftxui::Component GetSaveWindow();
  // Load save window
  ftxui::Component GetLoadWindow();

private: // Attributes
  // Size of the board
  std::uint32_t m_BoardSize = 4;

  // Current cursor position
  std::int32_t m_CurrentX = 0;
  std::int32_t m_CurrentY = 0;

  // Show options window
  bool m_ShowOptions = true;

  // Add background
  bool m_AddBackground = false;

  // Player count
  std::int32_t m_PlayerCount;

  // Index of selected file
  int m_SelectedSave = 0;

  // Get saves
  std::vector<std::string> m_ReadableSaveList =
      get_human_readable_file_list("saves/");
  std::vector<std::string> m_SaveList = get_file_list("saves/");

  // Load window height
  int m_LoadWindowHeight = static_cast<int>(m_SaveList.size()) + 6;

  std::string m_Message = "Select first card"; // Status message

  ftxui::Decorator m_TextStyle =
      ftxui::underlined |
      ftxui::color(ftxui::Color::LightYellow3); // Message style

  ftxui::ScreenInteractive m_Screen = ftxui::ScreenInteractive::Fullscreen();

  // Handle events and update static UI
  ftxui::Component m_Renderer;

  // Debug output stream
  std::ofstream m_DebugStream;

  // Handle the game logic
  std::unique_ptr<MemoryLogic> m_pGameLogic = std::make_unique<MemoryLogic>();
};
} // namespace memory_game