// File: bdi/core/types/TypeSystem.cpp
 #include "TypeSystem.hpp"
 namespace bdi::core::types {
 bool TypeSystem::isInteger(BDIType type) {
    switch (type) {
        case BDIType::INT8: case BDIType::INT16: case BDIType::INT32: case BDIType::INT64:
        case BDIType::UINT8: case BDIType::UINT16: case BDIType::UINT32: case BDIType::UINT64:
            return true;
        default:
            return false;
    }
 }
 bool TypeSystem::isFloatingPoint(BDIType type) {
     switch (type) {
        case BDIType::FLOAT16: case BDIType::FLOAT32: case BDIType::FLOAT64:
            return true;
        default:
            return false;
    }
 }
 bool TypeSystem::isNumeric(BDIType type) {
    return isInteger(type) || isFloatingPoint(type);
 }
 bool TypeSystem::isSigned(BDIType type) {
     switch (type) {
        case BDIType::INT8: case BDIType::INT16: case BDIType::INT32: case BDIType::INT64:
        case BDIType::FLOAT16: case BDIType::FLOAT32: case BDIType::FLOAT64:
            return true;
default:
            return false;
    }
 }
 bool TypeSystem::areCompatible(BDIType type1, BDIType type2) {
    // For now, strict compatibility means identical type
    // Could be expanded later if needed (e.g., treating UINT_PTR and POINTER as compatible)
    return type1 == type2;
 }
 bool TypeSystem::canImplicitlyConvert(BDIType from_type, BDIType to_type) {
    if (from_type == to_type) return true;
    // --- Integer Promotions --
    // Smaller to larger of same signedness
    if ((from_type == BDIType::INT8 && (to_type == BDIType::INT16 || to_type == BDIType::INT32 || to_type == BDIType::INT64)) ||
        (from_type == BDIType::INT16 && (to_type == BDIType::INT32 || to_type == BDIType::INT64)) ||
        (from_type == BDIType::INT32 && to_type == BDIType::INT64)) {
        return true;
    }
     if ((from_type == BDIType::UINT8 && (to_type == BDIType::UINT16 || to_type == BDIType::UINT32 || to_type == BDIType::UINT64)) ||
        (from_type == BDIType::UINT16 && (to_type == BDIType::UINT32 || to_type == BDIType::UINT64)) ||
        (from_type == BDIType::UINT32 && to_type == BDIType::UINT64)) {
        return true;
    }
    // Unsigned to signed (if target is larger) - Be careful with value ranges
    // if (from_type == BDIType::UINT8 && (to_type == BDIType::INT16 || to_type == BDIType::INT32 || to_type == BDIType::INT64)) return true;
    // if (from_type == BDIType::UINT16 && (to_type == BDIType::INT32 || to_type == BDIType::INT64)) return true;
    // if (from_type == BDIType::UINT32 && to_type == BDIType::INT64) return true;
    // --- Float Promotions --
    if ((from_type == BDIType::FLOAT16 && (to_type == BDIType::FLOAT32 || to_type == BDIType::FLOAT64)) ||
        (from_type == BDIType::FLOAT32 && to_type == BDIType::FLOAT64)) {
        return true;
    }
    // --- Integer to Float --
    if (isInteger(from_type) && isFloatingPoint(to_type)) {
        // Generally safe, potential precision loss for large integers to float32
        return true;
    }
    // --- BOOL to Integer --
    if (from_type == BDIType::BOOL && isInteger(to_type)) {
        return true; // true -> 1, false -> 0
    }
    // Add more rules as needed (e.g., pointer conversions?)
    return false;
 }
 BDIType TypeSystem::getPromotedType(BDIType type1, BDIType type2) {
    // Handle identical types
    if (type1 == type2 && isNumeric(type1)) return type1;
    bool t1_int = isInteger(type1);
    bool t2_int = isInteger(type2);
    bool t1_fp = isFloatingPoint(type1);
    bool t2_fp = isFloatingPoint(type2);
    // If one is float, promote to the larger float type
    if (t1_fp || t2_fp) {
        if (type1 == BDIType::FLOAT64 || type2 == BDIType::FLOAT64) return BDIType::FLOAT64;
        if (type1 == BDIType::FLOAT32 || type2 == BDIType::FLOAT32) return BDIType::FLOAT32;
        if (type1 == BDIType::FLOAT16 || type2 == BDIType::FLOAT16) return BDIType::FLOAT16; // Assuming float16 exists
    }
    // If both are integers, promote according to standard C/C++ rules (simplified)
    if (t1_int && t2_int) {
 size_t size1 = getBdiTypeSize(type1);
        size_t size2 = getBdiTypeSize(type2);
        bool signed1 = isSigned(type1);
        bool signed2 = isSigned(type2);
        // Promote to largest size
        size_t max_size = std::max(size1, size2);
        // If signedness differs and unsigned is >= signed size, promote to unsigned
        if (signed1 != signed2) {
            if (!signed1 && size1 >= size2) return type1; // type1 is unsigned and >= type2
            if (!signed2 && size2 >= size1) return type2; // type2 is unsigned and >= type1
        }
        // Otherwise, promote to signed if either input is signed, using max size
        bool result_signed = signed1 || signed2;
        // Find matching type based on size and signedness
        if (max_size == 8) return result_signed ? BDIType::INT64 : BDIType::UINT64;
        if (max_size == 4) return result_signed ? BDIType::INT32 : BDIType::UINT32;
        if (max_size == 2) return result_signed ? BDIType::INT16 : BDIType::UINT16;
        if (max_size == 1) return result_signed ? BDIType::INT8 : BDIType::UINT8;
    }
    // Invalid combination for numeric promotion
    return BDIType::UNKNOWN;
 }
 } // namespace bdi::core::types
