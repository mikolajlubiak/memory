// header
#include "memory_logic.hpp"

// std
#include <algorithm>
#include <format>
#include <numeric>
#include <random>

namespace memory_game {

bool MemoryLogic::SelectCard(std::uint32_t current_x, std::uint32_t current_y) {
  // Check whether the coordinates exceed board size
  if (current_x > m_BoardSize || current_y > m_BoardSize) {
    std::ofstream debug_stream("debug_output.txt",
                               std::ios::app); // Debug output stream

    debug_stream << "[MemoryLogic::SelectCard] Selected card coordinates "
                    "exceed board size"
                 << std::endl;

    debug_stream.close();
    return false;
  }

  // If game is finished do nothing
  if (m_GameStatus == GameStatus::gameFinished) {
    return false;
  }

  if (m_GameStatus == GameStatus::selectingFirstCard) {
    // If the card is already revlead: return
    if (m_Revealed[current_x][current_y]) {
      return true;
    }

    // Reveal card
    m_Revealed[current_x][current_y] = true;

    // Store the card coordinates for next stage
    m_PreviousX = current_x;
    m_PreviousY = current_y;

    // Precede to next stage
    m_GameStatus = GameStatus::selectingSecondCard;

    return true;

  } else if (m_GameStatus == GameStatus::selectingSecondCard) {
    // If the card is already revlead: return
    if (m_Revealed[current_x][current_y]) {
      return true;
    }

    // Reveal card
    m_Revealed[current_x][current_y] = true;

    // Check if the cards match
    if (CheckMatch(current_x, current_y, m_PreviousX, m_PreviousY)) {
      m_PlayersMatchedCardsCount[m_PlayerIndex]++;

      // Check if all cards are matched
      if (std::reduce(m_PlayersMatchedCardsCount.begin(),
                      m_PlayersMatchedCardsCount.end()) *
              2 <
          m_TotalCards) {
        m_GameStatus = GameStatus::selectingFirstCard;
      } else {
        m_GameStatus = GameStatus::gameFinished;
      }

    } else {
      // Store the second selected card index if
      // the cards don't match so that the user can move freely and when the
      // user comes back to stage one the card will be hidden
      m_TempX = current_x;
      m_TempY = current_y;

      // Next players turn
      m_PlayerIndex = (m_PlayerIndex + 1) % m_PlayersCount;

      m_GameStatus = GameStatus::cardsDidntMatch;
    }

    return true;

  } else if (m_GameStatus == GameStatus::cardsDidntMatch) {
    // Hide cards after they didn't match
    m_Revealed[m_TempX][m_TempY] = false;
    m_Revealed[m_PreviousX][m_PreviousY] = false;

    // Go back to first card selection stage
    m_GameStatus = GameStatus::selectingFirstCard;

    return false;
  }

  return false;
}

void MemoryLogic::InitializeBoard() {
  // Clear board and game state
  ResetState();

  // Generate cards
  std::vector<char> cards;
  cards.resize(m_TotalCards);

  for (int i = 0; i < m_TotalCards; i += 2) {
    char c = 'A' + i / 2;
    cards[i] = c;
    cards[i + 1] = c;
  }

  // Randomize/shuffle cards
  std::random_device rd;  // Obtain a random number from hardware
  std::mt19937 eng(rd()); // Seed the generator
  std::shuffle(cards.begin(), cards.end(), eng); // Shuffle the cards

  // Initialize and fill the vectors
  m_Board.resize(m_BoardSize, std::vector<char>(m_BoardSize));
  m_Revealed.resize(m_BoardSize, std::vector<bool>(m_BoardSize, false));
  m_PlayersMatchedCardsCount.resize(m_PlayersCount, 0);

  for (int i = 0; i < m_BoardSize; ++i) {
    for (int j = 0; j < m_BoardSize; ++j) {
      m_Board[i][j] = cards[i * m_BoardSize + j];
    }
  }
}

bool MemoryLogic::CheckMatch(std::uint32_t x1, std::uint32_t y1,
                             std::uint32_t x2, std::uint32_t y2) {
  return m_Board[x1][y1] == m_Board[x2][y2] && !(x1 == x2 && y1 == y2);
}

void MemoryLogic::ResetState() {
  // Clear vectors
  m_Board.clear();
  m_Revealed.clear();
  m_PlayersMatchedCardsCount.clear();

  // Reset game state
  m_PlayerIndex = 0;
  m_GameStatus = GameStatus::selectingFirstCard;
}

void MemoryLogic::SaveState(const std::string &filename) {
  // Prevent a weird bug when you save on
  // `m_GameStatus == GameStatus::cardsDidntMatch`
  //
  // After loading and pressing enter the
  // `m_Revealed[m_TempX][m_TempY]` wouldn't hide since
  // m_TempX and m_TempY are not stored.
  if (m_GameStatus == GameStatus::cardsDidntMatch) {
    m_Revealed[m_TempX][m_TempY] = false;
    m_Revealed[m_PreviousX][m_PreviousY] = false;

    m_GameStatus = GameStatus::selectingFirstCard;
  }

  // Open file for writing in binary format
  std::ofstream file(filename, std::ios::binary);

  // If the file didn't open note and return
  if (!file.is_open()) {
    std::ofstream debug_stream("debug_output.txt",
                               std::ios::app); // Debug output stream

    debug_stream << "[MemoryLogic::SaveState] Unable to open file: " << filename
                 << std::endl;

    debug_stream.close();
    return;
  }

  // Save board state
  file.write(reinterpret_cast<const char *>(&m_BoardSize), sizeof(m_BoardSize));
  file.write(reinterpret_cast<const char *>(&m_GameStatus),
             sizeof(m_GameStatus));
  file.write(reinterpret_cast<const char *>(&m_PlayersCount),
             sizeof(m_PlayersCount));
  file.write(reinterpret_cast<const char *>(&m_PlayerIndex),
             sizeof(m_PlayerIndex));

  // Save cursor state
  file.write(reinterpret_cast<const char *>(&m_PreviousX), sizeof(m_PreviousX));
  file.write(reinterpret_cast<const char *>(&m_PreviousY), sizeof(m_PreviousY));

  // Save players matched cards count
  file.write(reinterpret_cast<const char *>(&m_PlayersMatchedCardsCount[0]),
             m_PlayersMatchedCardsCount.size() *
                 sizeof(m_PlayersMatchedCardsCount[0]));

  for (int i = 0; i < m_BoardSize; i++) {
    // Save board
    file.write(reinterpret_cast<const char *>(&m_Board[i][0]),
               m_Board[i].size() * sizeof(m_Board[i][0]));

    // Save which cards are revealed
    for (int j = 0; j < m_BoardSize; j++) {
      char temp = static_cast<char>(m_Revealed[i][j]);
      file.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
    }
  }

  // Close file
  file.close();
}

std::uint32_t MemoryLogic::LoadState(const std::string &filename) {
  // Open file for reading in binary format
  std::ifstream file(filename, std::ios::binary);

  // If the file didn't open note and return
  if (!file.is_open()) {
    std::ofstream debug_stream("debug_output.txt",
                               std::ios::app); // Debug output stream

    debug_stream << "[MemoryLogic::SaveState] Unable to open file: " << filename
                 << std::endl;

    debug_stream.close();
    return 0;
  }

  // Load board state
  file.read(reinterpret_cast<char *>(&m_BoardSize), sizeof(m_BoardSize));
  file.read(reinterpret_cast<char *>(&m_GameStatus), sizeof(m_GameStatus));
  file.read(reinterpret_cast<char *>(&m_PlayersCount), sizeof(m_PlayersCount));
  file.read(reinterpret_cast<char *>(&m_PlayerIndex), sizeof(m_PlayerIndex));

  // Load cursor state
  file.read(reinterpret_cast<char *>(&m_PreviousX), sizeof(m_PreviousX));
  file.read(reinterpret_cast<char *>(&m_PreviousY), sizeof(m_PreviousY));

  // Set total number of cards for given size
  m_TotalCards = std::pow(m_BoardSize, 2);

  // Resize the vectors to fit the board size
  m_Board.resize(m_BoardSize, std::vector<char>(m_BoardSize));
  m_Revealed.resize(m_BoardSize, std::vector<bool>(m_BoardSize));

  // Resize vector to fit number of players
  m_PlayersMatchedCardsCount.resize(m_PlayersCount);

  // Load players matched cards count
  file.read(reinterpret_cast<char *>(&m_PlayersMatchedCardsCount[0]),
            m_PlayersMatchedCardsCount.size() *
                sizeof(m_PlayersMatchedCardsCount[0]));

  for (int i = 0; i < m_BoardSize; i++) {
    // Load board
    file.read(reinterpret_cast<char *>(&m_Board[i][0]),
              m_Board.size() * sizeof(m_Board[i][0]));

    // Load which cards should be revealed
    for (int j = 0; j < m_Revealed.size(); j++) {
      char temp;
      file.read(reinterpret_cast<char *>(&temp), sizeof(temp));
      m_Revealed[i][j] = static_cast<bool>(temp);
    }
  }

  // Close file
  file.close();

  return m_BoardSize;
}

} // namespace memory_game