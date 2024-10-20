#include <cmath>
#include <cstdint>
#include <cstdlib>

class DynamicPackedBoolArray {
public:
  DynamicPackedBoolArray() : m_SizeInBits(0), m_Data(nullptr) {}

  // Allocate memory and initialize to 0
  DynamicPackedBoolArray(std::size_t size_in_bits)
      : m_SizeInBits(size_in_bits) {
    m_Data = reinterpret_cast<std::uint8_t *>(std::calloc(1, GetSizeInBytes()));
  }

  // Free the data. I don't have to worry about dangling pointer since the
  // m_Data pointer goes out of scope anyway.
  ~DynamicPackedBoolArray() { free(m_Data); }

  // Resize the data. Using calloc instead of realloc since I don't want to copy
  // the data.
  void Resize(std::size_t size_in_bits) {
    m_SizeInBits = size_in_bits;

    m_Data = reinterpret_cast<std::uint8_t *>(std::calloc(1, GetSizeInBytes()));
  }

  // Clear the array. Don't free the memory since it costly and it will be done
  // in dtor anyway.
  void Clear() { m_SizeInBits = 0; }

  // Set bit at position index to val
  void Set(const std::size_t index, const bool val) {
    if (index < m_SizeInBits) {
      std::uint8_t *byte = GetBytePtr(index);

      if (val) {
        *byte = *byte | (1 << (index % 8));
      } else {
        *byte = *byte & ~(1 << (index % 8));
      }
    }
  }

  // Set m_Data to some other pointer of size size_in_bits
  void SetArray(std::uint8_t *ptr, const std::size_t size_in_bits) {
    m_SizeInBits = size_in_bits;

    m_Data = ptr;
  }

  // Set all of the bytes to 0
  void SetToZero() {
    for (std::size_t i = 0; i < GetSizeInBytes(); i++) {
      *(m_Data + i) = 0;
    }
  }

  // Get bit value at position index
  bool GetBit(const std::size_t index) const {
    if (index < m_SizeInBits) {
      std::uint8_t bitmask = 1 << (index % 8);

      std::uint8_t byte = *GetBytePtr(index);

      return (byte & bitmask) != 0;
    }
    return 0;
  }

  // Get pointer to the byte that stores bit at position index
  std::uint8_t *GetBytePtr(const std::size_t index) const {
    if (index < m_SizeInBits) {
      std::size_t byte_index = index / 8;
      return m_Data + byte_index;
    }
    return nullptr;
  }

  // Get pointer to the m_Data
  std::uint8_t *GetPtr() const { return m_Data; }

  // Get size of m_Data in bytes
  std::size_t GetSizeInBytes() const { return (m_SizeInBits + 7) / 8; }

  // Overload [] operator to access bit at position index
  bool operator[](std::size_t index) const { return GetBit(index); }

private:
  std::uint8_t
      *m_Data; // Array that stores the boolean, 1-bit size, 1 or 0 values

  std::size_t m_SizeInBits; // Size of the array in bits. (How many boolean
                            // values it stores)
};