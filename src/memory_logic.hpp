#pragma once

// std
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace memory_game {

// State at which the game is currently
enum class GameStatus : std::uint32_t {
  selectingFirstCard,  // Selecting first card
  selectingSecondCard, // Selecting second card
  cardsDidntMatch,     // Checked selected cards and they don't match
  gameFinished,        // Game finished
};

// Handle game logic
class MemoryLogic {
public:
  MemoryLogic(std::uint32_t board_size) : m_BoardSize(board_size) {
    // Set total number of cards for given size
    m_TotalCards = std::pow(m_BoardSize, 2);

    // Initialize the board and game state
    InitializeBoard();
  }

  // (On event enter) Select card at specified coordinates
  // Returns the prefered blinking status
  bool SelectCard(std::uint32_t current_x, std::uint32_t current_y);

  // Save current game state to file
  void SaveState(const std::string &filename);

  // Load game state from file
  std::uint32_t LoadState(const std::string &filename);

  // Return const board reference
  const std::vector<std::vector<char>> &GetBoard() { return m_Board; }

  // Return const revealed reference
  const std::vector<std::vector<bool>> &GetRevealed() { return m_Revealed; }

  // Return count of found pairs
  std::uint32_t GetPairsFoundCount() { return m_PairsFoundCount; }

  // Return game status
  GameStatus GetGameStatus() { return m_GameStatus; }

private: // Methods
  // Initialize random game board
  void InitializeBoard();

  // Check if the selected cards match
  bool CheckMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                  std::uint32_t y2);

  // Reset board state
  void ResetState();

private:                                    // Attributes
  std::vector<std::vector<char>> m_Board{}; // 2D vector storing cards (chars)

  std::vector<std::vector<bool>>
      m_Revealed{}; // 2D vector stroing which cards should be revealed

  std::uint32_t m_BoardSize = 4;                         // Size of the board
  std::uint32_t m_TotalCards = std::pow(m_BoardSize, 2); // Total number cards

  GameStatus m_GameStatus =
      GameStatus::selectingFirstCard; // Current game status

  std::uint32_t m_PairsFoundCount = 0; // Number of found pairs

  std::uint32_t m_PreviousX =
      0; // Previous X (in game state selectingSecondCard)
  std::uint32_t m_PreviousY =
      0; // Previous Y (in game state selectingSecondCard)

  // These variables will be used to store the second selected card index if
  // the cards don't match so that the user can move freely and when the user
  // comes back to stage one the card will be hidden
  std::uint32_t m_TempX = 0;
  std::uint32_t m_TempY = 0;
};

} // namespace memory_game