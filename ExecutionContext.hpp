 // File: bdi/runtime/ExecutionContext.hpp
 #ifndef BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #define BDI_RUNTIME_EXECUTIONCONTEXT_HPP
 #include "../core/graph/BDINode.hpp"       // For PortRef, NodeID
 #include "../core/payload/TypedPayload.hpp" // For TypedPayload
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
    void setPortValue(const PortRef& port, TypedPayload value);
    void setPortValue(NodeID node_id, PortIndex port_idx, TypedPayload value);
    // Retrieve the value for a specific port
    std::optional<TypedPayload> getPortValue(const PortRef& port) const;
    std::optional<TypedPayload> getPortValue(NodeID node_id, PortIndex port_idx) const;
    // --- Call Stack (Basic Implementation) --
    // void pushCall(NodeID return_node_id);
    // std::optional<NodeID> popCall();
    // bool isCallStackEmpty() const;
    // Clear context (e.g., for starting a new execution)
    void clear();
 private:
    // Stores the output values of executed nodes' ports
    std::unordered_map<PortRef, TypedPayload, PortRefHash> port_values_;
    // Basic call stack for function calls/returns
    // std::vector<NodeID> call_stack_;
    // Note: Full call stack implementation requires handling arguments, local variables etc.
    // Deferring full implementation for now.
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_EXECUTIONCONTEXT_HPP
