//
// Created by Mike Loomis on 6/22/2019.
//

#ifndef CPPACK_PACKER_HPP
#define CPPACK_PACKER_HPP

#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <cmath>
#include <bitset>

namespace msgpack {

enum FormatConstants : uint8_t {
  // positive fixint = 0x00 - 0x7f
  // fixmap = 0x80 - 0x8f
  // fixarray = 0x90 - 0x9a
  // fixstr = 0xa0 - 0xbf
  // negative fixint = 0xe0 - 0xff

  nil = 0xc0,
  false_bool = 0xc2,
  true_bool = 0xc3,
  bin8 = 0xc4,
  bin16 = 0xc5,
  bin32 = 0xc6,
  ext8 = 0xc7,
  ext16 = 0xc8,
  ext32 = 0xc9,
  float32 = 0xca,
  float64 = 0xcb,
  uint8 = 0xcc,
  uint16 = 0xcd,
  uint32 = 0xce,
  uint64 = 0xcf,
  int8 = 0xd0,
  int16 = 0xd1,
  int32 = 0xd2,
  int64 = 0xd3,
  fixext1 = 0xd4,
  fixext2 = 0xd5,
  fixext4 = 0xd6,
  fixext8 = 0xd7,
  fixext16 = 0xd8,
  str8 = 0xd9,
  str16 = 0xda,
  str32 = 0xdb,
  array16 = 0xdc,
  array32 = 0xdd,
  map16 = 0xde,
  map32 = 0xdf
};

template<class T>
struct is_container {
  static const bool value = false;
};

template<class T, class Alloc>
struct is_container<std::vector<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::list<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::map<T, Alloc> > {
  static const bool value = true;
};

template<class T, class Alloc>
struct is_container<std::set<T, Alloc> > {
  static const bool value = true;
};

template<class T>
struct is_map {
  static const bool value = false;
};

template<class T, class Alloc>
struct is_map<std::map<T, Alloc> > {
  static const bool value = true;
};

class Packer {
 public:
  template<class ... Types>
  void process(Types &... args) {
    (pack_type(args), ...);
  }

  const std::vector<uint8_t> &vector() const {
    return serialized_object;
  }

  void clear() {
    serialized_object.clear();
  }

 private:
  std::vector<uint8_t> serialized_object;

  template<class T>
  void pack_type(const T &value) {
    if constexpr(is_map<T>::value) {
      pack_map(value);
    } else if constexpr (is_container<T>::value) {
      pack_array(value);
    } else {
      std::cerr << "Unknown type\n";
    }
  }

  template<class T>
  void pack_array(const T &array) {
    if (array.size() < 16) {
      auto size_mask = uint8_t(0b10010000);
      serialized_object.emplace_back(uint8_t(array.size() | size_mask));
    } else if (array.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(array16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(array.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (array.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(array32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(array.size() >> (8U * (i - 1)) & 0xff));
      }
    } else {
      return; // Give up if string is too long
    }
    for (const auto &elem : array) {
      pack_type(elem);
    }
  }

  template<class T>
  void pack_map(const T &map) {
    if (map.size() < 16) {
      auto size_mask = uint8_t(0b10000000);
      serialized_object.emplace_back(uint8_t(map.size() | size_mask));
    } else if (map.size() < std::numeric_limits<uint16_t>::max()) {
      serialized_object.emplace_back(map16);
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    } else if (map.size() < std::numeric_limits<uint32_t>::max()) {
      serialized_object.emplace_back(map32);
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        serialized_object.emplace_back(uint8_t(map.size() >> (8U * (i - 1)) & 0xff));
      }
    }
    for (const auto &elem : map) {
      pack_type(std::get<0>(elem));
      pack_type(std::get<1>(elem));
    }
  }
};

std::bitset<64> twos_complement(int64_t value) {
  if (value < 0) {
    auto abs_v = llabs(value);
    return ~abs_v + 1;
  } else {
    return {(uint64_t) value};
  }
}

std::bitset<32> twos_complement(int32_t value) {
  if (value < 0) {
    auto abs_v = abs(value);
    return ~abs_v + 1;
  } else {
    return {(uint32_t) value};
  }
}

std::bitset<16> twos_complement(int16_t value) {
  if (value < 0) {
    auto abs_v = abs(value);
    return ~abs_v + 1;
  } else {
    return {(uint16_t) value};
  }
}

std::bitset<8> twos_complement(int8_t value) {
  if (value < 0) {
    auto abs_v = abs(value);
    return ~abs_v + 1;
  } else {
    return {(uint8_t) value};
  }
}

template<>
void Packer::pack_type(const int8_t &value) {
  if (value > 31 || value < -32) {
    serialized_object.emplace_back(int8);
  }
  serialized_object.emplace_back(uint8_t(twos_complement(value).to_ulong()));
}

template<>
void Packer::pack_type(const int16_t &value) {
  if (abs(value) < abs(std::numeric_limits<int8_t>::min())) {
    pack_type(int8_t(value));
  } else {
    serialized_object.emplace_back(int16);
    auto serialize_value = uint16_t(twos_complement(value).to_ulong());
    for (auto i = sizeof(value); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
    }
  }
}

template<>
void Packer::pack_type(const int32_t &value) {
  if (abs(value) < abs(std::numeric_limits<int16_t>::min())) {
    pack_type(int16_t(value));
  } else {
    serialized_object.emplace_back(int32);
    auto serialize_value = uint32_t(twos_complement(value).to_ulong());
    for (auto i = sizeof(value); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
    }
  }
}

template<>
void Packer::pack_type(const int64_t &value) {
  if (llabs(value) < llabs(std::numeric_limits<int32_t>::min()) && value != std::numeric_limits<int64_t>::min()) {
    pack_type(int32_t(value));
  } else {
    serialized_object.emplace_back(int64);
    auto serialize_value = uint64_t(twos_complement(value).to_ullong());
    for (auto i = sizeof(value); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(serialize_value >> (8U * (i - 1)) & 0xff));
    }
  }
}

template<>
void Packer::pack_type(const uint8_t &value) {
  if (value <= 0x7f) {
    serialized_object.emplace_back(value);
  } else {
    serialized_object.emplace_back(uint8);
    serialized_object.emplace_back(value);
  }
}

template<>
void Packer::pack_type(const uint16_t &value) {
  if (value > std::numeric_limits<uint8_t>::max()) {
    serialized_object.emplace_back(uint16);
    for (auto i = sizeof(value); i > 0U; --i) {
      serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
    }
  } else {
    pack_type(uint8_t(value));
  }
}

template<>
void Packer::pack_type(const uint32_t &value) {
  if (value > std::numeric_limits<uint16_t>::max()) {
    serialized_object.emplace_back(uint32);
    for (auto i = sizeof(value); i > 0U; --i) {
      serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
    }
  } else {
    pack_type(uint16_t(value));
  }
}

template<>
void Packer::pack_type(const uint64_t &value) {
  if (value > std::numeric_limits<uint32_t>::max()) {
    serialized_object.emplace_back(uint64);
    for (auto i = sizeof(value); i > 0U; --i) {
      serialized_object.emplace_back(uint8_t(value >> (8U * (i - 1)) & 0xff));
    }
  } else {
    pack_type(uint32_t(value));
  }
}

template<>
void Packer::pack_type(const std::nullptr_t &/*value*/) {
  serialized_object.emplace_back(nil);
}

template<>
void Packer::pack_type(const bool &value) {
  if (value) {
    serialized_object.emplace_back(true_bool);
  } else {
    serialized_object.emplace_back(false_bool);
  }
}

template<>
void Packer::pack_type(const float &value) {
  double integral_part;
  auto fractional_remainder = float(modf(value, &integral_part));

  if (fractional_remainder == 0) { // Just pack as int
    pack_type(int64_t(integral_part));
  } else {
    static_assert(std::numeric_limits<float>::radix == 2); // TODO: Handle decimal floats
    auto exponent = ilogb(value);
    float full_mantissa = value / float(scalbn(1.0, exponent));
    auto sign_mask = std::bitset<32>(uint32_t(std::signbit(full_mantissa)) << 31);
    auto excess_127_exponent_mask = std::bitset<32>(uint32_t(exponent + 127) << 23);
    auto normalized_mantissa_mask = std::bitset<32>();
    float implied_mantissa = fabs(full_mantissa) - 1.0f;
    for (auto i = 23U; i > 0; --i) {
      integral_part = 0;
      implied_mantissa *= 2;
      implied_mantissa = float(modf(implied_mantissa, &integral_part));
      if (uint8_t(integral_part) == 1) {
        normalized_mantissa_mask |= std::bitset<32>(uint32_t(1 << (i - 1)));
      }
    }

    auto ieee754_float32 = (sign_mask | excess_127_exponent_mask | normalized_mantissa_mask).to_ulong();
    std::clog << "Exponent: " << exponent << '\n';
    std::clog << "Full Mantissa: " << full_mantissa << '\n';
    std::clog << "Implied mantissa: " << implied_mantissa << '\n';
    std::clog << "ieee float: " << ieee754_float32 << '\n';
    serialized_object.emplace_back(float32);
    for (auto i = sizeof(ieee754_float32); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(ieee754_float32 >> (8U * (i - 1)) & 0xff));
    }
  }
}

template<>
void Packer::pack_type(const double &value) {
  double integral_part;
  double fractional_remainder = modf(value, &integral_part);

  if (fractional_remainder == 0) { // Just pack as int
    pack_type(int64_t(integral_part));
  } else {
    static_assert(std::numeric_limits<float>::radix == 2); // TODO: Handle decimal floats
    auto exponent = ilogb(value);
    double full_mantissa = value / scalbn(1.0, exponent);
    auto sign_mask = std::bitset<64>(uint64_t(std::signbit(full_mantissa)) << 63);
    auto excess_127_exponent_mask = std::bitset<64>(uint64_t(exponent + 1023) << 52);
    auto normalized_mantissa_mask = std::bitset<64>();
    double implied_mantissa = fabs(full_mantissa) - 1.0f;

    for (auto i = 52U; i > 0; --i) {
      integral_part = 0;
      implied_mantissa *= 2;
      implied_mantissa = modf(implied_mantissa, &integral_part);
      if (uint8_t(integral_part) == 1) {
        normalized_mantissa_mask |= std::bitset<64>(uint64_t(1) << (i - 1));
      }
    }
    auto ieee754_float64 = (sign_mask | excess_127_exponent_mask | normalized_mantissa_mask).to_ullong();
    serialized_object.emplace_back(float64);
    for (auto i = sizeof(ieee754_float64); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(ieee754_float64 >> (8U * (i - 1)) & 0xff));
    }
  }
}

template<>
void Packer::pack_type(const std::string &value) {
  if (value.size() < 32) {
    serialized_object.emplace_back(uint8_t(value.size()) | 0b10100000);
  } else if (value.size() < std::numeric_limits<uint8_t>::max()) {
    serialized_object.emplace_back(str8);
    serialized_object.emplace_back(uint8_t(value.size()));
  } else if (value.size() < std::numeric_limits<uint16_t>::max()) {
    serialized_object.emplace_back(str16);
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
    }
  } else if (value.size() < std::numeric_limits<uint32_t>::max()) {
    serialized_object.emplace_back(str32);
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
    }
  } else {
    return; // Give up if string is too long
  }
  for (char i : value) {
    serialized_object.emplace_back(static_cast<uint8_t>(i));
  }
}

template<>
void Packer::pack_type(const std::vector<uint8_t> &value) {
  if (value.size() < std::numeric_limits<uint8_t>::max()) {
    serialized_object.emplace_back(bin8);
    serialized_object.emplace_back(uint8_t(value.size()));
  } else if (value.size() < std::numeric_limits<uint16_t>::max()) {
    serialized_object.emplace_back(bin16);
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
    }
  } else if (value.size() < std::numeric_limits<uint32_t>::max()) {
    serialized_object.emplace_back(bin32);
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      serialized_object.emplace_back(uint8_t(value.size() >> (8U * (i - 1)) & 0xff));
    }
  } else {
    return; // Give up if vector is too large
  }
  for (const auto &elem : value) {
    serialized_object.emplace_back(elem);
  }
}

class Unpacker {
 public:
  Unpacker() : data_pointer(nullptr) {};

  explicit Unpacker(const uint8_t *data_start) : data_pointer(data_start) {};

  template<class ... Types>
  void process(Types &... args) {
    (unpack_type(args), ...);
  }

  void set_data(const uint8_t *pointer) {
    data_pointer = pointer;
  }

 private:
  const uint8_t *data_pointer;

  template<class T>
  void unpack_type(T &value) {
    if constexpr(is_map<T>::value) {
      unpack_map(value);
    } else if constexpr (is_container<T>::value) {
      unpack_array(value);
    } else {
      std::cerr << "Unknown type\n";
    }
  }

  template<class T>
  void unpack_array(T &array) {
    using ValueType = typename T::value_type;
    if (*data_pointer == array32) {
      data_pointer++;
      std::size_t array_size = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        array_size += uint32_t(*data_pointer++) << 8 * (i - 1);
      }
      std::vector<uint32_t> x{};
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    } else if (*data_pointer == array16) {
      data_pointer++;
      std::size_t array_size = 0;
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        array_size += uint16_t(*data_pointer++) << 8 * (i - 1);
      }
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    } else {
      std::size_t array_size = *data_pointer & 0b00001111;
      data_pointer++;
      for (auto i = 0U; i < array_size; ++i) {
        ValueType val{};
        unpack_type(val);
        array.emplace_back(val);
      }
    }
  }

  template<class T>
  void unpack_map(T &map) {
    using KeyType = typename T::key_type;
    using MappedType = typename T::mapped_type;
    if (*data_pointer == map32) {
      data_pointer++;
      std::size_t map_size = 0;
      for (auto i = sizeof(uint32_t); i > 0; --i) {
        map_size += uint32_t(*data_pointer++) << 8 * (i - 1);
      }
      std::vector<uint32_t> x{};
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    } else if (*data_pointer == map16) {
      data_pointer++;
      std::size_t map_size = 0;
      for (auto i = sizeof(uint16_t); i > 0; --i) {
        map_size += uint16_t(*data_pointer++) << 8 * (i - 1);
      }
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    } else {
      std::size_t map_size = *data_pointer & 0b00001111;
      data_pointer++;
      for (auto i = 0U; i < map_size; ++i) {
        KeyType key{};
        MappedType value{};
        unpack_type(key);
        unpack_type(value);
        map.insert_or_assign(key, value);
      }
    }
  }
};

template<>
void Unpacker::unpack_type(int8_t &value) {
  if (*data_pointer == int8) {
    value = *++data_pointer;
    data_pointer++;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(int16_t &value) {
  if (*data_pointer == int16) {
    data_pointer++;
    std::bitset<16> bits;
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      bits |= uint16_t(*data_pointer++) << 8 * (i - 1);
    }
    if (bits[15]) {
      value = -1 * (uint16_t((~bits).to_ulong()) + 1);
    } else {
      value = uint16_t(bits.to_ulong());
    }
  } else if (*data_pointer == int8) {
    int8_t val;
    unpack_type(val);
    value = val;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(int32_t &value) {
  if (*data_pointer == int32) {
    data_pointer++;
    std::bitset<32> bits;
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      bits |= uint32_t(*data_pointer++) << 8 * (i - 1);
    }
    if (bits[31]) {
      value = -1 * ((~bits).to_ulong() + 1);
    } else {
      value = bits.to_ulong();
    }
  } else if (*data_pointer == int16) {
    int16_t val;
    unpack_type(val);
    value = val;
  } else if (*data_pointer == int8) {
    int8_t val;
    unpack_type(val);
    value = val;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(int64_t &value) {
  if (*data_pointer == int64) {
    data_pointer++;
    std::bitset<64> bits;
    for (auto i = sizeof(value); i > 0; --i) {
      bits |= std::bitset<8>(*data_pointer++).to_ullong() << 8 * (i - 1);
    }
    if (bits[63]) {
      value = -1 * ((~bits).to_ullong() + 1);
    } else {
      value = bits.to_ullong();
    }
  } else if (*data_pointer == int32) {
    int32_t val;
    unpack_type(val);
    value = val;
  } else if (*data_pointer == int16) {
    int16_t val;
    unpack_type(val);
    value = val;
  } else if (*data_pointer == int8) {
    int8_t val;
    unpack_type(val);
    value = val;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(uint8_t &value) {
  if (*data_pointer == uint8) {
    value = *++data_pointer;
    data_pointer++;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(uint16_t &value) {
  if (*data_pointer == uint16) {
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      value += *++data_pointer << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint8) {
    value = *++data_pointer;
    data_pointer++;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(uint32_t &value) {
  if (*data_pointer == uint32) {
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      value += *++data_pointer << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint16) {
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      value += *++data_pointer << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint8) {
    value = *++data_pointer;
    data_pointer++;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(uint64_t &value) {
  if (*data_pointer == uint64) {
    for (auto i = sizeof(uint64_t); i > 0; --i) {
      value += uint64_t(*++data_pointer) << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint32) {
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      value += uint64_t(*++data_pointer) << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint16) {
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      value += uint64_t(*++data_pointer) << 8 * (i - 1);
    }
    data_pointer++;
  } else if (*data_pointer == uint8) {
    value = *++data_pointer;
    data_pointer++;
  } else {
    value = *data_pointer++;
  }
}

template<>
void Unpacker::unpack_type(std::nullptr_t &/*value*/) {
  data_pointer++;
}

template<>
void Unpacker::unpack_type(bool &value) {
  value = *data_pointer++ != 0xc2;
}

template<>
void Unpacker::unpack_type(float &value) {
  if (*data_pointer == float32) {
    data_pointer++;
    uint32_t data = 0;
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      data += *data_pointer++ << 8 * (i - 1);
    }
    auto bits = std::bitset<32>(data);
    auto mantissa = 1.0f;
    for (auto i = 23U; i > 0; --i) {
      if (bits[i - 1]) {
        mantissa += 1.0f / (1 << (24 - i));
      }
    }
    if (bits[31]) {
      mantissa *= -1;
    }
    uint8_t exponent = 0;
    for (auto i = 0U; i < 8; ++i) {
      exponent += bits[i + 23] << i;
    }
    exponent -= 127;
    value = ldexp(mantissa, exponent);
  } else {
    if (*data_pointer == int8 || *data_pointer == int16 || *data_pointer == int32 || *data_pointer == int64) {
      int64_t val = 0;
      unpack_type(val);
      value = float(val);
    } else {
      uint64_t val = 0;
      unpack_type(val);
      value = float(val);
    }
  }
}

template<>
void Unpacker::unpack_type(double &value) {
  if (*data_pointer == float64) {
    data_pointer++;
    uint64_t data = 0;
    for (auto i = sizeof(uint64_t); i > 0; --i) {
      data += uint64_t(*data_pointer++) << 8 * (i - 1);
    }
    auto bits = std::bitset<64>(data);
    auto mantissa = 1.0;
    for (auto i = 52U; i > 0; --i) {
      if (bits[i - 1]) {
        mantissa += 1.0 / (uint64_t(1) << (53 - i));
      }
    }
    if (bits[63]) {
      mantissa *= -1;
    }
    uint16_t exponent = 0;
    for (auto i = 0U; i < 11; ++i) {
      exponent += bits[i + 52] << i;
    }
    exponent -= 1023;
    value = ldexp(mantissa, exponent);
  } else {
    if (*data_pointer == int8 || *data_pointer == int16 || *data_pointer == int32 || *data_pointer == int64) {
      int64_t val = 0;
      unpack_type(val);
      value = float(val);
    } else {
      uint64_t val = 0;
      unpack_type(val);
      value = float(val);
    }
  }
}

template<>
void Unpacker::unpack_type(std::string &value) {
  if (*data_pointer == str32) {
    data_pointer++;
    std::size_t str_size = 0;
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      str_size += uint32_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::string{data_pointer, data_pointer + str_size};
  } else if (*data_pointer == str16) {
    data_pointer++;
    std::size_t str_size = 0;
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      str_size += uint16_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::string{data_pointer, data_pointer + str_size};
  } else if (*data_pointer == str8) {
    data_pointer++;
    std::size_t str_size = 0;
    for (auto i = sizeof(uint8_t); i > 0; --i) {
      str_size += uint8_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::string{data_pointer, data_pointer + str_size};
  } else {
    std::size_t str_size = *data_pointer & 0b00011111;
    data_pointer++;
    value = std::string{data_pointer, data_pointer + str_size};
    data_pointer += str_size;
  }
}

template<>
void Unpacker::unpack_type(std::vector<uint8_t> &value) {
  if (*data_pointer == bin32) {
    data_pointer++;
    std::size_t bin_size = 0;
    for (auto i = sizeof(uint32_t); i > 0; --i) {
      bin_size += uint32_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::vector<uint8_t>{data_pointer, data_pointer + bin_size};
  } else if (*data_pointer == bin16) {
    data_pointer++;
    std::size_t bin_size = 0;
    for (auto i = sizeof(uint16_t); i > 0; --i) {
      bin_size += uint16_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::vector<uint8_t>{data_pointer, data_pointer + bin_size};
  } else {
    data_pointer++;
    std::size_t bin_size = 0;
    for (auto i = sizeof(uint8_t); i > 0; --i) {
      bin_size += uint8_t(*data_pointer++) << 8 * (i - 1);
    }
    value = std::vector<uint8_t>{data_pointer, data_pointer + bin_size};
  }
}

template<class PackableObject>
std::vector<uint8_t> pack(PackableObject &&obj) {
  auto packer = Packer{};
  obj.pack(packer);
  return packer.vector();
}

template<class UnpackableObject>
UnpackableObject unpack(uint8_t *data_start) {
  auto obj = UnpackableObject{};
  auto unpacker = Unpacker(data_start);
  obj.pack(unpacker);
  return obj;
}
}

#endif //CPPACK_PACKER_HPP
