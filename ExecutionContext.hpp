 // File: bdi/runtime/ExecutionContext.hpp
 #ifndef BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #define BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #include "../core/graph/BDINode.hpp"       // For PortRef, NodeID
 #include "../core/payload/TypedPayload.hpp" // For TypedPayload
 #include "BDIValueVariant.hpp" // Use the new variant type
 #include <unordered_map>
 #include <optional>
 #include <vector> // For call stack
 namespace bdi::runtime {
 using bdi::core::graph::NodeID;
 using bdi::core::graph::PortRef;
 using bdi::core::payload::TypedPayload;
 // Hash function for PortRef to use it in unordered_map
 struct PortRefHash {
    std::size_t operator()(const PortRef& pr) const noexcept {
        // Simple hash combination - consider better hashing for performance
        std::size_t h1 = std::hash<NodeID>{}(pr.node_id);
        std::size_t h2 = std::hash<PortIndex>{}(pr.port_index);
        return h1 ^ (h2 << 1); // Combine hashes
    }
 };
 class ExecutionContext {
 public:
    ExecutionContext() = default;
 // Store the output value for a specific port
    void setPortValue(const PortRef& port, BDIValueVariant value); // Takes variant
    void setPortValue(NodeID node_id, PortIndex port_idx, BDIValueVariant value);
    // Retrieve the value variant for a specific port
    std::optional<BDIValueVariant> getPortValue(const PortRef& port) const; // Returns variant
    std::optional<BDIValueVariant> getPortValue(NodeID node_id, PortIndex port_idx) const;
    // --- Conversion --
    // Convert from TypedPayload (binary) to BDIValueVariant (runtime value)
    // Returns std::monostate in variant on error
    static BDIValueVariant payloadToVariant(const TypedPayload& payload);
    // Convert from BDIValueVariant (runtime value) back to TypedPayload (binary)
    // Returns TypedPayload with UNKNOWN type on error
    static TypedPayload variantToPayload(const BDIValueVariant& value);
    // --- Call Stack --
    void pushCall(NodeID return_node_id);
    std::optional<NodeID> popCall();
    bool isCallStackEmpty() const;
    void clear();
 private:
    std::unordered_map<PortRef, BDIValueVariant, PortRefHash> port_values_; // Stores variants
    std::vector<NodeID> call_stack_;
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_EXECUTIONCONTEXT_HPP
