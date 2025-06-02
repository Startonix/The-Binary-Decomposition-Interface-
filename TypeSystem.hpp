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
 // Determine the result type for a binary operation based on input types
    // Returns UNKNOWN if operation is invalid for the types or promotion fails
    static BDIType getPromotedType(BDIType type1, BDIType type2);
    // Check if a type is considered an integer type (signed or unsigned)
    static bool isInteger(BDIType type);
    // Check if a type is floating point
    static bool isFloatingPoint(BDIType type);
    // Check if a type is numeric (integer or float)
    static bool isNumeric(BDIType type);
    // Check if a type is signed
    static bool isSigned(BDIType type);
 };
 } // namespace bdi::core::types
 #endif // BDI_CORE_TYPES_TYPESYSTEM_HPP
