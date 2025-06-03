// File: bdi/core/types/BinaryEncoding.cpp
 #include "BinaryEncoding.hpp"
 #include <cstring> // For memcpy
 #include <stdexcept> // For errors potentially
 #include <algorithm> // For std::reverse
 #include <bit> // For std::endian
 namespace bdi::core::types {
 // --- Host Endianness Detection --
constexpr bool HOST_IS_LITTLE_ENDIAN = (std::endian::native == std::endian::little);
 // --- Helper: Reverse bytes if needed --
template <typename T>
 void ensure_target_endian(T& value) requires std::is_integral_v<T> || std::is_floating_point_v<T>
 {
    if constexpr (HOST_IS_LITTLE_ENDIAN != TARGET_IS_LITTLE_ENDIAN) {
        auto* bytes = reinterpret_cast<std::byte*>(&value);
        std::reverse(bytes, bytes + sizeof(T));
    }
    // If host and target endianness match, do nothing.
 }
 // --- Helper: Append bytes to buffer --
template <typename T>
 void append_bytes(BinaryData& buffer, T value) {
    ensure_target_endian(value); // Ensure correct byte order before appending
    const std::byte* start = reinterpret_cast<const std::byte*>(&value);
    buffer.insert(buffer.end(), start, start + sizeof(T));
 }
 // --- Helper: Read bytes from buffer --
template <typename T>
 bool read_bytes(const BinaryData& buffer, size_t& offset, T& out_value) {
    if (offset + sizeof(T) > buffer.size()) {
        return false; // Not enough data
    }
    std::memcpy(&out_value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    ensure_target_endian(out_value); // Convert to host endianness after reading
    return true;
 }
// --- Encoding Implementations --
void encode_bool(BinaryData& buffer, bool value) { encode_u8(buffer, static_cast<uint8_t>(value)); }
 void encode_i8(BinaryData& buffer, int8_t value) { append_bytes(buffer, value); }
 void encode_u8(BinaryData& buffer, uint8_t value) { append_bytes(buffer, value); }
 void encode_i16(BinaryData& buffer, int16_t value) { append_bytes(buffer, value); }
 void encode_u16(BinaryData& buffer, uint16_t value) { append_bytes(buffer, value); }
 void encode_i32(BinaryData& buffer, int32_t value) { append_bytes(buffer, value); }
 void encode_u32(BinaryData& buffer, uint32_t value) { append_bytes(buffer, value); }
 void encode_i64(BinaryData& buffer, int64_t value) { append_bytes(buffer, value); }
 void encode_u64(BinaryData& buffer, uint64_t value) { append_bytes(buffer, value); }
 void encode_f32(BinaryData& buffer, float value) { append_bytes(buffer, value); } // Assumes IEEE 754 host
 void encode_f64(BinaryData& buffer, double value) { append_bytes(buffer, value); } // Assumes IEEE 754 host
 void encode_ptr(BinaryData& buffer, uintptr_t value) { append_bytes(buffer, value); } // Size depends on architecture
 // --- Decoding Implementations --
bool decode_bool(const BinaryData& buffer, size_t& offset, bool& out_value) {
 uint8_t int_val;
 if (!read_bytes(buffer, offset, int_val)) return false;
 out_value = (int_val != 0);
 return true;
 }
 bool decode_i8(const BinaryData& buffer, size_t& offset, int8_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_u8(const BinaryData& buffer, size_t& offset, uint8_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_i16(const BinaryData& buffer, size_t& offset, int16_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_u16(const BinaryData& buffer, size_t& offset, uint16_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_i32(const BinaryData& buffer, size_t& offset, int32_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_u32(const BinaryData& buffer, size_t& offset, uint32_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_i64(const BinaryData& buffer, size_t& offset, int64_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_u64(const BinaryData& buffer, size_t& offset, uint64_t& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_f32(const BinaryData& buffer, size_t& offset, float& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_f64(const BinaryData& buffer, size_t& offset, double& out_value) { return read_bytes(buffer, offset, out_value); }
 bool decode_ptr(const BinaryData& buffer, size_t& offset, uintptr_t& out_value) { return read_bytes(buffer, offset, out_value); }
 } // namespace bdi::core::types
