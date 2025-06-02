 // File: bdi/core/payload/TypedPayload.hpp 
 #ifndef BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
 #define BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
 #include "../types/BDITypes.hpp"
 #include "../types/BinaryEncoding.hpp" // Placeholder
 #include <vector>
 #include <cstddef>
 #include <stdexcept>
 #include <concepts>
 #include <bit> // For std::bit_cast
 #include <cstring> // For memcpy fallback
 #include <array> // For std::bit_cast target
 #include <type_traits> // For std::is_same_v
 namespace bdi::core::payload {
 using bdi::core::types::BDIType;
 using bdi::core::types::BinaryData;
 using bdi::core::types::getBdiTypeSize;
 // --- Helper to map C++ types to BDIType --
template <typename T>
 struct MapCppTypeToBdiType {
 static constexpr BDIType value = BDIType::UNKNOWN; // Default
 };
 // Specializations for common types
 #define MAP_TYPE(CppType, BdiTypeEnum) \
 template <> struct MapCppTypeToBdiType<CppType> { static constexpr BDIType value = BdiTypeEnum; }
 MAP_TYPE(void, BDIType::VOID);
 MAP_TYPE(bool, BDIType::BOOL);
 MAP_TYPE(int8_t, BDIType::INT8);
 MAP_TYPE(uint8_t, BDIType::UINT8);
 MAP_TYPE(int16_t, BDIType::INT16);
 MAP_TYPE(uint16_t, BDIType::UINT16);
 MAP_TYPE(int32_t, BDIType::INT32);
 MAP_TYPE(uint32_t, BDIType::UINT32);
 MAP_TYPE(int64_t, BDIType::INT64);
 MAP_TYPE(uint64_t, BDIType::UINT64);
 MAP_TYPE(float, BDIType::FLOAT32);
 MAP_TYPE(double, BDIType::FLOAT64);
 // Add mappings for pointers, NodeID (uint64_t), RegionID (uint64_t) etc. if needed directly
 MAP_TYPE(uintptr_t, BDIType::POINTER); // Example mapping for POINTER
 MAP_TYPE(bdi::core::graph::NodeID, BDIType::NODE_ID); // Assuming NodeID is uint64_t
 MAP_TYPE(bdi::core::graph::RegionID, BDIType::REGION_ID); // Assuming RegionID is uint64_t
 #undef MAP_TYPE
 // --- TypedPayload Structure --
struct TypedPayload {
    BDIType type = BDIType::UNKNOWN;
    BinaryData data;
    TypedPayload() = default;
    TypedPayload(BDIType t, BinaryData d) : type(t), data(std::move(d)) {}
    bool isValid() const {
        size_t expectedSize = getBdiTypeSize(type);
        return type != BDIType::UNKNOWN && (expectedSize == 0 || data.size() == expectedSize);
        // Note: Need better handling for variable size types later (VECTOR, STRUCT etc.)
    }
    // Safe data access with type checking
    template <typename T>
    T getAs() const {
        constexpr BDIType expectedBdiType = MapCppTypeToBdiType<T>::value;
        static_assert(expectedBdiType != BDIType::UNKNOWN, "No BDIType mapping for C++ type T in getAs");
        if (type != expectedBdiType) {
            // Option 1: Throw immediately
             throw std::runtime_error("TypedPayload type mismatch: Expected " +
                 std::string(bdi::core::types::bdiTypeToString(expectedBdiType)) + ", got " +
                 std::string(bdi::core::types::bdiTypeToString(type)));
            // Option 2: Try conversion? (More complex, needs TypeSystem interaction)
            // Option 3: Return optional<T>?
        }
        if (sizeof(T) != data.size()) {
             throw std::runtime_error("TypedPayload size mismatch for getAs<T>");
        }
        if (data.empty() && sizeof(T) > 0) { // Allow empty for void?
             throw std::runtime_error("TypedPayload data is empty for getAs<T>");
        }
        if constexpr (sizeof(T) == 0) { // Handle void/empty types if necessary
             return; // Or some unit type representation?
        }
        // Use std::bit_cast if available and safe
        #if __cpp_lib_bit_cast >= 201806L && defined(__cpp_lib_is_layout_compatible) // Check for layout compatibility guarantees
            // Requires C++20 and layout compatibility checks for full safety if types differ slightly
            // For identical types or trivially copyable types, it should work
             if constexpr (std::is_trivially_copyable_v<T>) {
                // Reinterpret byte array as array of correct size for bit_cast
                return std::bit_cast<T>(*reinterpret_cast<const std::array<std::byte, sizeof(T)>*>(data.data()));
             } else {
                 // Fallback or error if not trivially copyable? Depends on requirements.
                 // Using memcpy as fallback:
                 T value;
                 std::memcpy(&value, data.data(), sizeof(T));
                 return value;
             }
        #else
             // Fallback using memcpy for older standards or missing features
             T value;
 std::memcpy(&value, data.data(), sizeof(T));
             return value;
        #endif
    }
     // Factory function with type mapping
     template <typename T>
     static TypedPayload createFrom(const T& value) {
         constexpr BDIType payloadType = MapCppTypeToBdiType<T>::value;
         if constexpr (payloadType == BDIType::UNKNOWN) {
             throw std::runtime_error("Cannot map C++ type to BDIType in createFrom");
         }
         BinaryData data(sizeof(T));
         // Use memcpy for portability, assuming host representation matches desired binary layout for now
         // TODO: Integrate with BinaryEncoding for proper endianness/format control
         std::memcpy(data.data(), &value, sizeof(T));
         return TypedPayload(payloadType, std::move(data));
     }
      // Special case for void/empty payload
      static TypedPayload createVoid() {
          return TypedPayload(BDIType::VOID, {});
      }
 };
 } // namespace bdi::core::payload
 #endif // BDI_CORE_PAYLOAD_TYPEDPAYLOAD_HPP
