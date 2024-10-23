#pragma once

// local
#include <dynamic_packed_bool_array.hpp>

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

  MemoryLogic(std::uint32_t board_size);

  MemoryLogic(std::uint32_t board_size, std::uint32_t player_count);

  // Initialize random game board
  void InitializeBoard();

  // Set board size
  void SetBoardSize(std::uint32_t board_size);

  // Set player count
  void SetPlayerCount(std::uint32_t player_count) {
    m_PlayersCount = player_count;

    InitializeBoard();
  }

  // (On event enter) Select card at specified coordinates
  void SelectCard(std::uint32_t current_x, std::uint32_t current_y);

  // Save current game state to file
  void SaveState(const std::string &filename);

  // Load game state from file
  void LoadState(const std::string &filename);

  // Return const board reference
  const std::vector<std::vector<char>> &GetBoard() const { return m_Board; }

  // Return const revealed cards reference
  const std::vector<DynamicPackedBoolArray> &GetHasCardBeenRevealed() const {
    return m_HasCardBeenRevealed;
  }

  // Return const revealed reference
  const std::vector<DynamicPackedBoolArray> &GetHasCardBeenMatched() const {
    return m_HasCardBeenMatched;
  }

  // Return count of found pairs for current player
  std::uint32_t GetMatchedCardsCount(std::uint32_t player_index) const {
    return m_PlayersMatchedCardsCount[player_index];
  }

  // Player with most matched cards
  std::vector<std::uint32_t> GetWinners() const;

  // Return current players index
  std::uint32_t GetCurrentPlayerIndex() const { return m_PlayerIndex; }

  // Return number of players
  std::uint32_t GetPlayerCount() const { return m_PlayersCount; }

  // Return total number of cards
  std::uint32_t GetTotalCardsCount() const { return std::pow(m_BoardSize, 2); }

  // Return game status
  GameStatus GetGameStatus() const { return m_GameStatus; }

  // Return game status
  std::uint32_t GetBoardSize() const { return m_BoardSize; }

  // Return current turn number
  std::uint32_t GetTurnNumber() const { return m_TurnNumber; }

private: // Methods
  // Check if the selected cards match
  bool CheckMatch(std::uint32_t x1, std::uint32_t y1, std::uint32_t x2,
                  std::uint32_t y2) const;

  // Reset board state
  void ResetState();

private:                                    // Attributes
  std::vector<std::vector<char>> m_Board{}; // 2D vector storing cards (chars)

  std::vector<DynamicPackedBoolArray>
      m_HasCardBeenRevealed{}; // 2D (kinda) vector storing which cards should
                               // be revealed

  std::vector<DynamicPackedBoolArray>
      m_HasCardBeenMatched{}; // 2D (kinda) vector storing whether a card has
                              // been matched

  std::uint32_t m_BoardSize = 4; // Size of the board

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

  std::uint32_t m_TurnNumber = 1; // Current turn number
};

} // namespace memory_game