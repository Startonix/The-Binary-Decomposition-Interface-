 // File: bdi/core/types/BinaryEncoding.hpp 
 #ifndef BDI_CORE_TYPES_BINARYENCODING_HPP
 #define BDI_CORE_TYPES_BINARYENCODING_HPP
 #include "BDITypes.hpp"
 #include <vector>
 #include <cstddef>
 #include <cstdint>
 #include <concepts> // For requires clause
 namespace bdi::core::types {
 using BinaryData = std::vector<std::byte>;
 // Target Endianness for serialization (can be configured)
 #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
 constexpr bool TARGET_IS_LITTLE_ENDIAN = true;
 #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
 constexpr bool TARGET_IS_LITTLE_ENDIAN = false;
 #else
 #warning "Target endianness not detected, assuming little-endian for BDI serialization."
 constexpr bool TARGET_IS_LITTLE_ENDIAN = true; // Default assumption
 #endif
 // --- Encoding Functions --
 void encode_bool(BinaryData& buffer, bool value);
 void encode_i8(BinaryData& buffer, int8_t value);
 void encode_u8(BinaryData& buffer, uint8_t value
 void encode_i16(BinaryData& buffer, int16_t value);
 void encode_u16(BinaryData& buffer, uint16_t value);
 void encode_i32(BinaryData& buffer, int32_t value);
 void encode_u32(BinaryData& buffer, uint32_t value);
 void encode_i64(BinaryData& buffer, int64_t value);
 void encode_u64(BinaryData& buffer, uint64_t value);
 void encode_f32(BinaryData& buffer, float value);
 void encode_f64(BinaryData& buffer, double value);
 void encode_ptr(BinaryData& buffer, uintptr_t value);
 // --- Decoding Functions --
 // Return bool success, value written to out parameter
 bool decode_bool(const BinaryData& buffer, size_t& offset, bool& out_value);
 bool decode_i8(const BinaryData& buffer, size_t& offset, int8_t& out_value);
 bool decode_u8(const BinaryData& buffer, size_t& offset, uint8_t& out_value);
 bool decode_i16(const BinaryData& buffer, size_t& offset, int16_t& out_value);
 bool decode_u16(const BinaryData& buffer, size_t& offset, uint16_t& out_value);
 bool decode_i32(const BinaryData& buffer, size_t& offset, int32_t& out_value);
 bool decode_u32(const BinaryData& buffer, size_t& offset, uint32_t& out_value);
 bool decode_i64(const BinaryData& buffer, size_t& offset, int64_t& out_value);
 bool decode_u64(const BinaryData& buffer, size_t& offset, uint64_t& out_value);
 bool decode_f32(const BinaryData& buffer, size_t& offset, float& out_value);
 bool decode_f64(const BinaryData& buffer, size_t& offset, double& out_value);
 bool decode_ptr(const BinaryData& buffer, size_t& offset, uintptr_t& out_value);
 } // namespace bdi::core::types
 #endif // BDI_CORE_TYPES_BINARYENCODING_HPP
