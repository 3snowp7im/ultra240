#pragma once

#include <cstdint>

namespace ultra::util {

  /** 
   * Read count of bits from a field, shifting the field down by the number of
   * bits read. The result will be sign extended to the specified type.
   */
  template<typename T = int32_t>
  T read_signed_bits(uint32_t& field, uint8_t count) {
    uint32_t mask = (1 << count) - 1;
    uint32_t bits = field & mask;
    if (bits & (1 << (count - 1))) {
      bits |= ~0 ^ ~mask;
    }
    field >>= count;
    return static_cast<T>(bits);
  }

  /** 
   * Read count of bits from a field, shifting the field down by the number of
   * bits read. The result will be zero extended to the specified type.
   */
  template<typename T = uint32_t>
  T read_unsigned_bits(uint32_t& field, uint8_t count) {
    uint32_t mask = (1 << count) - 1;
    uint32_t bits = field & mask;
    field >>= count;
    return static_cast<T>(bits);
  }

}
