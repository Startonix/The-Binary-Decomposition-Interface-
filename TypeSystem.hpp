 // File: bdi/core/types/TypeSystem.hpp
 #ifndef BDI_CORE_TYPES_TYPESYSTEM_HPP
 #define BDI_CORE_TYPES_TYPESYSTEM_HPP
 #include "BDITypes.hpp"
 #include <numeric> // For std::common_type 
 namespace bdi::core::types {
 class TypeSystem {
 public:
 // Basic type compatibility check (can types be used interchangeably?)
    static bool areCompatible(BDIType type1, BDIType type2) {
        // Basic implementation: types are compatible if they are identical
        // TODO: Expand with rules for implicit conversions (e.g., INT32 -> INT64)
        // TODO: Handle compatibility for pointers/references if needed
        return type1 == type2;
    }
    // Check if an implicit conversion is allowed
    // Check if an implicit conversion is generally considered safe/standard
    static bool canImplicitlyConvert(BDIType from_type, BDIType to_type) {
        if (from_type == to_type) return true;
        // Example: Widening integer conversions
        if ((from_type == BDIType::INT32 && to_type == BDIType::INT64) ||
            (from_type == BDIType::UINT32 && to_type == BDIType::UINT64) ||
            (from_type == BDIType::INT16 && (to_type == BDIType::INT32 || to_type == BDIType::INT64)) // etc.
           ) {
            return true;
        }
        // Example: Float conversions
        if (from_type == BDIType::FLOAT32 && to_type == BDIType::FLOAT64) {
            return true;
        }
        // Add more rules as needed...
        return false;
    }
    // TODO: Add methods for handling composite types (structs, vectors) if needed
    // static BDIType resolveVectorElementType(BDIType vectorType);
    // static StructLayout getStructLayout(BDIType structType); // Requires type registry
 };
 } // namespace bdi::core::types
 #endif // BDI_CORE_TYPES_TYPESYSTEM_HPP
 // File: bdi/core/payload/TypedPayload.hpp
 #ifndef BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
 #define BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
 #include "../types/BDITypes.hpp"
 #include "../types/BinaryEncoding.hpp"
 #include <vector>
 #include <cstddef> // For std::byte
 #include <stdexcept>
 #include <concepts> // For C++20 concepts (optional, can use SFINAE/templates)
 #include <bit> // For std::bit_cast (C++20)
 namespace bdi::core::payload {
 using bdi::core::types::BDIType;
 using bdi::core::types::BinaryData;
 using bdi::core::types::getBdiTypeSize;
 // Represents a block of binary data associated with a specific BDI type.
 // Used for node payloads (immediate values, configuration).
 struct TypedPayload {
    BDIType type = BDIType::UNKNOWN;
    BinaryData data;
    TypedPayload() = default;
    TypedPayload(BDIType t, BinaryData d) : type(t), data(std::move(d)) {}
    // Basic validation
    bool isValid() const {
        size_t expectedSize = getBdiTypeSize(type);
        // Allow size 0 for VOID, UNKNOWN, etc. Handle variable sizes later.
        return type != BDIType::UNKNOWN && (expectedSize == 0 || data.size() == expectedSize);
    }
    // Template for safe data access (requires C++20 std::bit_cast)
    // Throws if type mismatch or size mismatch occurs
 template <typename T>
    T getAs() const {
        // TODO: Add a static mapping from C++ type T to BDIType for checking
        // constexpr BDIType expectedBdiType = MapCppTypeToBdiType<T>::value;
        // if (type != expectedBdiType) {
        //     throw std::runtime_error("TypedPayload type mismatch");
        // }
        if (sizeof(T) != data.size()) {
             throw std::runtime_error("TypedPayload size mismatch for getAs<T>");
        }
        if (data.empty()) {
             throw std::runtime_error("TypedPayload data is empty for getAs<T>");
        }
        // Requires C++20. For C++17 or earlier, use memcpy carefully.
        // Assumes trivial copyability and correct alignment.
        #if __cpp_lib_bit_cast >= 201806L
            return std::bit_cast<T>(*reinterpret_cast<const std::array<std::byte, sizeof(T)>*>(data.data()));
        #else
             // Fallback using memcpy (less safe regarding type rules)
             T value;
             if (data.size() == sizeof(T)) {
                 std::memcpy(&value, data.data(), sizeof(T));
                 return value;
             } else {
                throw std::runtime_error("TypedPayload size mismatch (memcpy fallback)");
             }
        #endif
    }
     // Factory function example (needs corresponding T -> BDIType mapping)
     template <typename T>
     static TypedPayload createFrom(const T& value) {
         // TODO: Determine BDIType from T
         // BDIType payloadType = MapCppTypeToBdiType<T>::value;
         BDIType payloadType = BDIType::UNKNOWN; // Placeholder - MUST BE IMPLEMENTED
         if constexpr (std::is_same_v<T, int32_t>) payloadType = BDIType::INT32;
         else if constexpr (std::is_same_v<T, float>) payloadType = BDIType::FLOAT32;
         // ... add more mappings ...
         if (payloadType == BDIType::UNKNOWN) {
             throw std::runtime_error("Cannot map C++ type to BDIType in createFrom");
         }
         BinaryData data(sizeof(T));
         std::memcpy(data.data(), &value, sizeof(T));
         return TypedPayload(payloadType, std::move(data));
     }
 };
 } // namespace bdi::core::payload
 #endif // BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
