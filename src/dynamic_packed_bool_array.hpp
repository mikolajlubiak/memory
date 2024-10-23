#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

// Proxy class to allow for setting bit value through [] operator
class Proxy {
public:
  Proxy(std::uint8_t *byte, std::size_t bit_position)
      : m_Byte(byte), m_BitPosition(bit_position) {}

  // Conversion operator to bool
  operator bool() const { return (*m_Byte & (1 << m_BitPosition)) != 0; }

  // Assignment operator to set the bit
  Proxy &operator=(const bool value) {
    if (value) {
      *m_Byte |= (1 << m_BitPosition);
    } else {
      *m_Byte &= ~(1 << m_BitPosition);
    }
    return *this;
  }

private:
  std::uint8_t *m_Byte;
  std::size_t m_BitPosition;
};

class DynamicPackedBoolArray {
public:
  DynamicPackedBoolArray() : m_SizeInBits(0), m_Data(nullptr) {}

  // Allocate memory and initialize to 0
  DynamicPackedBoolArray(const std::size_t size_in_bits)
      : m_SizeInBits(size_in_bits) {
    m_Data = reinterpret_cast<std::uint8_t *>(std::calloc(1, GetSizeInBytes()));
  }

  // Free the data. I don't have to worry about dangling pointer since the
  // m_Data pointer goes out of scope anyway.
  ~DynamicPackedBoolArray() { free(m_Data); }

  // Resize the data. Using calloc instead of realloc since I don't want to copy
  // the data.
  void Resize(const std::size_t size_in_bits) {
    // Allocate memory only if the requested size in bytes is larger than
    // currently allocated bytes
    if (BitsToBytes(size_in_bits) > GetSizeInBytes()) {
      m_Data = reinterpret_cast<std::uint8_t *>(
          std::calloc(1, BitsToBytes(size_in_bits)));
    }

    m_SizeInBits = size_in_bits;
  }

  // Clear the array. Don't free the memory since its costly and it will be done
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
    if (index > m_SizeInBits) {
      throw std::out_of_range("Index out of range");
    }

    std::uint8_t bitmask = 1 << (index % 8);
    std::uint8_t byte = *GetBytePtr(index);
    return (byte & bitmask) != 0;
  }

  // Get pointer to the byte that stores bit at position index
  std::uint8_t *GetBytePtr(const std::size_t index) const {
    if (index > m_SizeInBits) {
      throw std::out_of_range("Index out of range");
    }

    std::size_t byte_index = index / 8;
    return m_Data + byte_index;
  }

  // Get pointer to the m_Data
  std::uint8_t *GetPtr() const { return m_Data; }

  // Get size of m_Data in bytes
  std::size_t GetSizeInBytes() const { return (m_SizeInBits + 7) / 8; }

  // Overload [] operator to access bit at position index
  bool operator[](const std::size_t index) const { return GetBit(index); }

  // Overload [] operator to return a Proxy for setting bits
  Proxy operator[](const std::size_t index) {
    return Proxy(GetBytePtr(index), index % 8);
  }

private:
  std::size_t BitsToBytes(const std::size_t size_in_bits) const {
    return (size_in_bits + 7) / 8;
  }

  std::uint8_t
      *m_Data; // Array that stores the boolean, 1-bit size, 1 or 0 values

  std::size_t m_SizeInBits; // Size of the array in bits. (How many boolean
                            // values it stores)
};