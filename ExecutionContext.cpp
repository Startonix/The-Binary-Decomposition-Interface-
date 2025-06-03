// File: bdi/runtime/ExecutionContext.cpp 
 #include "ExecutionContext.hpp"
 #include "../core/payload/TypedPayload.hpp"
 #include "../core/types/BinaryEncoding.hpp" // Include the encoding functions
 #include <stdexcept> // For type conversion errors
 #include <bit>       // For std::bit_cast
 #include <cstring>   // For memcpy
 #include <array>
 namespace bdi::runtime {
 // --- Value Storage --
void ExecutionContext::setPortValue(const PortRef& port, BDIValueVariant value) {
    port_values_[port] = std::move(value);
 }
 void ExecutionContext::setPortValue(NodeID node_id, PortIndex port_idx, BDIValueVariant value) {
    setPortValue({node_id, port_idx}, std::move(value));
 }
 std::optional<BDIValueVariant> ExecutionContext::getPortValue(const PortRef& port) const {
    auto it = port_values_.find(port);
    if (it != port_values_.end()) {
        return it->second;
    }
    return std::nullopt;
 }
 std::optional<BDIValueVariant> ExecutionContext::getPortValue(NodeID node_id, PortIndex port_idx) const {
     return getPortValue({node_id, port_idx});
 }
 // --- Conversion Helpers --
BDIValueVariant ExecutionContext::payloadToVariant(const TypedPayload& payload) {
    using Type = core::types::BDIType;
    try {
        switch (payload.type) {
            case Type::VOID:    return std::monostate{};
            case Type::BOOL:    return payload.getAs<bool>(); // Use safe getAs
            case Type::INT8:    return payload.getAs<int8_t>();
            case Type::UINT8:   return payload.getAs<uint8_t>();
            case Type::INT16:   return payload.getAs<int16_t>();
            case Type::UINT16:  return payload.getAs<uint16_t>();
            case Type::INT32:   return payload.getAs<int32_t>();
            case Type::UINT32:  return payload.getAs<uint32_t>();
            case Type::INT64:   return payload.getAs<int64_t>();
            case Type::UINT64:  return payload.getAs<uint64_t>();
            case Type::FLOAT32: return payload.getAs<float>();
            case Type::FLOAT64: return payload.getAs<double>();
            case Type::POINTER: // Fallthrough
            case Type::MEM_REF: // Fallthrough - treat as uintptr_t for now
            case Type::FUNC_PTR: return payload.getAs<uintptr_t>();
            // Add cases for NODE_ID, REGION_ID if needed, mapping to uint64_t probably
            // case Type::NODE_ID: return payload.getAs<uint64_t>();
            default:
                 // std::cerr << "Warning: payloadToVariant: Unsupported payload type " << core::types::bdiTypeToString(payload.type) << std::endl;
                 return std::monostate{}; // Return empty on error/unsupported
        }
    } catch (const std::exception& e) {
         std::cerr << "Error converting payload to variant: " << e.what() << std::endl;
  return std::monostate{};
    }
 }
 TypedPayload ExecutionContext::variantToPayload(const BDIValueVariant& value) {
     TypedPayload result;
     result.type = getBDIType(value); // Get type from variant content
     std::visit([&](auto&& arg) {
         using T = std::decay_t<decltype(arg)>;
         if constexpr (!std::is_same_v<T, std::monostate>) {
             result.data.resize(sizeof(T));
             // TODO: Integrate BinaryEncoding here for proper serialization
             std::memcpy(result.data.data(), &arg, sizeof(T));
         }
         // else: VOID type, empty data is correct
     }, value);
     return result; // Might have UNKNOWN type if variant held monostate unexpectedly
 }
 // --- Call Stack Methods --
void ExecutionContext::pushCall(NodeID return_node_id) {
    call_stack_.push_back(return_node_id);
 }
 std::optional<NodeID> ExecutionContext::popCall() {
    if (call_stack_.empty()) {
        return std::nullopt;
    }
    NodeID return_id = call_stack_.back();
    call_stack_.pop_back();
    return return_id;
 }
 bool ExecutionContext::isCallStackEmpty() const {
    return call_stack_.empty();
 }
 void ExecutionContext::clear() {
    port_values_.clear();
    call_stack_.clear();
 }
 } // namespace bdi::runtime
 BDIValueVariant ExecutionContext::payloadToVariant(const TypedPayload& payload) {
    using Type = core::types::BDIType;
    using namespace bdi::core::types; // For encoding functions
    size_t offset = 0;
    const BinaryData& buffer = payload.data;
    try {
        switch (payload.type) {
            // Use decode functions, checking return value
            case Type::VOID:    return std::monostate{};
            case Type::BOOL:    { bool v; return decode_bool(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::INT8:    { int8_t v; return decode_i8(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::UINT8:   { uint8_t v; return decode_u8(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::INT16:   { int16_t v; return decode_i16(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::UINT16:  { uint16_t v; return decode_u16(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::INT32:   { int32_t v; return decode_i32(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::UINT32:  { uint32_t v; return decode_u32(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::INT64:   { int64_t v; return decode_i64(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::UINT64:  { uint64_t v; return decode_u64(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::FLOAT32: { float v; return decode_f32(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::FLOAT64: { double v; return decode_f64(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            case Type::POINTER: // Fallthrough
            case Type::MEM_REF: // Fallthrough
            case Type::FUNC_PTR: { uintptr_t v; return decode_ptr(buffer, offset, v) ? BDIValueVariant{v} : std::monostate{}; }
            default:
                 return std::monostate{};
        }
    } catch (const std::exception& e) {
         std::cerr << "Error converting payload to variant: " << e.what() << std::endl;
         return std::monostate{};
    }
 }
 TypedPayload ExecutionContext::variantToPayload(const BDIValueVariant& value) {
     TypedPayload result;
     result.type = getBDIType(value);
     BinaryData buffer; // Use local buffer for encoding
     using namespace bdi::core::types;
     std::visit([&](auto&& arg) {
         using T = std::decay_t<decltype(arg)>;
         // Call appropriate encode function
         if constexpr (std::is_same_v<T, std::monostate>) { /* No data for VOID */ }
         else if constexpr (std::is_same_v<T, bool>)      { encode_bool(buffer, arg); }
         else if constexpr (std::is_same_v<T, int8_t>)    { encode_i8(buffer, arg); }
         else if constexpr (std::is_same_v<T, uint8_t>)   { encode_u8(buffer, arg); }
         else if constexpr (std::is_same_v<T, int16_t>)   { encode_i16(buffer, arg); }
         else if constexpr (std::is_same_v<T, uint16_t>)  { encode_u16(buffer, arg); }
         else if constexpr (std::is_same_v<T, int32_t>)   { encode_i32(buffer, arg); }
         else if constexpr (std::is_same_v<T, uint32_t>)  { encode_u32(buffer, arg); }
         else if constexpr (std::is_same_v<T, int64_t>)   { encode_i64(buffer, arg); }
         else if constexpr (std::is_same_v<T, uint64_t>)  { encode_u64(buffer, arg); }
         else if constexpr (std::is_same_v<T, float>)     { encode_f32(buffer, arg); }
         else if constexpr (std::is_same_v<T, double>)    { encode_f64(buffer, arg); }
         else if constexpr (std::is_same_v<T, uintptr_t>) { encode_ptr(buffer, arg); }
         // Add other types if needed
     }, value);
     result.data = std::move(buffer);
     return result;
 }
 // ... rest of ExecutionContext.cpp ...
 } // namespace bdi::runtime
#endif // BDI_RUNTIME_EXECUTIONCONTEXT_CPP
