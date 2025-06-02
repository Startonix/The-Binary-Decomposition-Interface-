// File: bdi/runtime/ExecutionContext.cpp
 #include "ExecutionContext.hpp"
 namespace bdi::runtime {
 void ExecutionContext::setPortValue(const PortRef& port, TypedPayload value) {
    port_values_[port] = std::move(value);
 }
 void ExecutionContext::setPortValue(NodeID node_id, PortIndex port_idx, TypedPayload value) {
    setPortValue({node_id, port_idx}, std::move(value));
 }
 std::optional<TypedPayload> ExecutionContext::getPortValue(const PortRef& port) const {
    auto it = port_values_.find(port);
    if (it != port_values_.end()) {
        return it->second;
    }
    return std::nullopt; // Value not found
 }
 std::optional<TypedPayload> ExecutionContext::getPortValue(NodeID node_id, PortIndex port_idx) const {
     return getPortValue({node_id, port_idx});
 }
 void ExecutionContext::clear() {
    port_values_.clear();
    // call_stack_.clear();
 }
 // --- Call Stack Methods (Stubs) --
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
    call_stack_.clear(); // Clear stack on context reset
 }

 } // namespace bdi::runtime
