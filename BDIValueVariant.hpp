 // File: bdi/runtime/BDIValueVariant.hpp
 #ifndef BDI_RUNTIME_BDIVALUEVARIANT_HPP
 #define BDI_RUNTIME_BDIVALUEVARIANT_HPP
 #include "../core/types/BDITypes.hpp"
 #include <variant>
 #include <cstdint>
 #include <string> // For potential errors
 namespace bdi::runtime {
 using bdi::core::types::BDIType;
 using bdi::core::graph::NodeID; // Include if needed for specific value types
 using bdi::core::graph::RegionID;
 // Define the variant to hold runtime values explicitly typed
 using BDIValueVariant = std::variant<
    std::monostate, // Represents uninitialized or void
    bool,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, // Corresponds to FLOAT32
    double, // Corresponds to FLOAT64
    uintptr_t // Represents POINTER, MEM_REF, potentially FUNC_PTR
    // Add other potential direct value types: NodeID, RegionID?
 >;
 // Helper to get BDIType from a BDIValueVariant
 inline BDIType getBDIType(const BDIValueVariant& value) {
    using namespace bdi::core::types;
    BDIType result = BDIType::UNKNOWN;
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) { result = BDIType::VOID; }
        else if constexpr (std::is_same_v<T, bool>)      { result = BDIType::BOOL; }
        else if constexpr (std::is_same_v<T, int8_t>)    { result = BDIType::INT8; }
        else if constexpr (std::is_same_v<T, uint8_t>)   { result = BDIType::UINT8; }
        else if constexpr (std::is_same_v<T, int16_t>)   { result = BDIType::INT16; }
        else if constexpr (std::is_same_v<T, uint16_t>)  { result = BDIType::UINT16; }
        else if constexpr (std::is_same_v<T, int32_t>)   { result = BDIType::INT32; }
        else if constexpr (std::is_same_v<T, uint32_t>)  { result = BDIType::UINT32; }
        else if constexpr (std::is_same_v<T, int64_t>)   { result = BDIType::INT64; }
        else if constexpr (std::is_same_v<T, uint64_t>)  { result = BDIType::UINT64; }
        else if constexpr (std::is_same_v<T, float>)     { result = BDIType::FLOAT32; }
        else if constexpr (std::is_same_v<T, double>)    { result = BDIType::FLOAT64; }
        else if constexpr (std::is_same_v<T, uintptr_t>) { result = BDIType::POINTER; } // Default mapping for uintptr_t
        // Add mappings for other types if included in the variant
    }, value);
    return result;
 }
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_BDIVALUEVARIANT_HPP
