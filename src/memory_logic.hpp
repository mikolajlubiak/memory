#pragma once

// std
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <set>
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
  MemoryLogic() = default;

  MemoryLogic(std::uint32_t board_size) : m_BoardSize(board_size) {
    // Set total number of cards for given size
    m_TotalCards = std::pow(m_BoardSize, 2);

    // Initialize the board and game state
    InitializeBoard();
  }

  MemoryLogic(std::uint32_t board_size, std::uint32_t player_count)
      : m_BoardSize(board_size), m_PlayersCount(player_count) {
    // Set total number of cards for given size
    m_TotalCards = std::pow(m_BoardSize, 2);

    // Initialize the board and game state
    InitializeBoard();
  }

  // Set board size
  void SetBoardSize(std::uint32_t board_size) {
    m_BoardSize = board_size;

    // Set total number of cards for given size
    m_TotalCards = std::pow(m_BoardSize, 2);

    // Initialize the board and game state
    InitializeBoard();
  }

  // (On event enter) Select card at specified coordinates
  void SelectCard(std::uint32_t current_x, std::uint32_t current_y);

  // Save current game state to file
  void SaveState(const std::string &filename);

  // Load game state from file
  std::uint32_t LoadState(const std::string &filename);

  // Return const board reference
  const std::vector<std::vector<char>> &GetBoard() { return m_Board; }

  // Return const revealed cards reference
  const std::vector<std::vector<bool>> &GetHasCardBeenRevealed() {
    return m_HasCardBeenRevealed;
  }

  // Return const revealed reference
  const std::vector<std::vector<bool>> &GetHasCardBeenMatched() {
    return m_HasCardBeenMatched;
  }

  // Return count of found pairs for current player
  std::uint32_t GetMatchedCardsCount() {
    return m_PlayersMatchedCardsCount[m_PlayerIndex];
  }

  // Player with most matched cards
  std::vector<std::uint32_t> GetWinners() {
    if (m_PlayersMatchedCardsCount.empty()) {
      return {};
    }

    // Find the maximum value
    std::uint32_t maxVal = *std::max_element(m_PlayersMatchedCardsCount.begin(),
                                             m_PlayersMatchedCardsCount.end());

    // Find all indices of the maximum value
    std::set<std::uint32_t> indices;
    for (std::uint32_t i = 0; i < m_PlayersMatchedCardsCount.size(); i++) {
      if (m_PlayersMatchedCardsCount[i] == maxVal) {
        indices.insert(i);
      }
    }

    return std::vector<std::uint32_t>(indices.begin(), indices.end());
  }

  // Return current players index
  std::uint32_t GetCurrentPlayerIndex() { return m_PlayerIndex; }

  // Set player count
  void SetPlayerCount(std::uint32_t player_count) {
    m_PlayersCount = player_count;

    // Initialize the board and game state
    InitializeBoard();
  }

  // Return game status
  GameStatus GetGameStatus() { return m_GameStatus; }

  // Initialize random game board
  void InitializeBoard();

private: // Methods
  // Check if the selected cards match
  bool CheckMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                  std::uint32_t y2);

  // Reset board state
  void ResetState();

private:                                    // Attributes
  std::vector<std::vector<char>> m_Board{}; // 2D vector storing cards (chars)

  std::vector<std::vector<bool>>
      m_HasCardBeenRevealed{}; // 2D vector stroing which cards should be
                               // revealed

  std::vector<std::vector<bool>>
      m_HasCardBeenMatched{}; // 2D vector stroing whether a card has been
                              // matched

  std::uint32_t m_BoardSize = 4;                         // Size of the board
  std::uint32_t m_TotalCards = std::pow(m_BoardSize, 2); // Total number cards

  GameStatus m_GameStatus =
      GameStatus::selectingFirstCard; // Current game status

  std::uint32_t m_PreviousX =
      0; // Previous X (in game state selectingSecondCard)
  std::uint32_t m_PreviousY =
      0; // Previous Y (in game state selectingSecondCard)

  // These variables will be used to store the second selected card index if
  // the cards don't match so that the user can move freely and when the user
  // comes back to stage one the card will be hidden
  std::uint32_t m_TempX = 0;
  std::uint32_t m_TempY = 0;

  std::uint32_t m_PlayersCount = 2; // Number of players
  std::vector<std::uint32_t>
      m_PlayersMatchedCardsCount{}; // Vector storing number of matched cards
                                    // for each player
  std::uint32_t m_PlayerIndex = 0;  // Current players turn
};

} // namespace memory_game