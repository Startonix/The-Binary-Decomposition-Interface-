// File: bdi/core/graph/BDINode.hpp
 #ifndef BDI_CORE_GRAPH_BDINODE_HPP
 #define BDI_CORE_GRAPH_BDINODE_HPP
 #include "../types/BDITypes.hpp"
 #include "../payload/TypedPayload.hpp"
 #include "OperationTypes.hpp"
 #include <cstdint>
 #include <vector>
 #include <string>
 #include <map> // Or unordered_map if performance critical and hashing is fine
 namespace bdi::core::graph {
 using bdi::core::types::BDIType;
 using bdi::core::payload::TypedPayload;
 // Using uint64_t for IDs for simplicity. Could be wrapped in a struct later.
 using NodeID = uint64_t;
 using PortIndex = uint32_t; // Index within input/output port lists
 using MetadataHandle = uint64_t; // Handle into the MetadataStore
 using RegionID = uint64_t; // Identifier for MemoryRegion
 // Represents a reference to a specific output port of another node
 struct PortRef {
    NodeID node_id = 0;
    PortIndex port_index = 0;
    bool operator==(const PortRef&) const = default; // For use in maps/sets
 };
 // Describes an output port of a node
 struct PortInfo {
    BDIType type = BDIType::UNKNOWN;
    std::string name = ""; // Optional symbolic name for debugging/introspection
    PortInfo(BDIType t = BDIType::UNKNOWN, std::string n = "") : type(t), name(std::move(n)) {}
 };
 // The core structure representing a node in the BDI computation graph
 struct BDINode {
    NodeID id = 0;
    BDIOperationType operation = BDIOperationType::META_NOP;
    // Data Inputs: Specifies which node output ports provide data to this node
    // The index in this vector corresponds to the logical input number for the operation
    std::vector<PortRef> data_inputs;
    // Data Outputs: Describes the data produced by this node
    // Other nodes refer to these via {this->id, output_index}
    std::vector<PortInfo> data_outputs;
    // Control Flow Inputs: Nodes that can transfer control *to* this node
    // Typically used for merge points, loop headers, function entries
    std::vector<NodeID> control_inputs;
    // Control Flow Outputs: Nodes where control can transfer *from* this node
    // Order might matter (e.g., for conditional branches: [true_target, false_target])
    // Could be map<ConditionValue, NodeID> for switch-like behavior
    std::vector<NodeID> control_outputs;
    // Immediate data or configuration used directly by the operation
    TypedPayload payload;
    // Handle to associated metadata (semantics, proofs, hints) in MetadataStore
    MetadataHandle metadata_handle = 0;
    // Logical memory/compute region assignment
    RegionID region_id = 0;
    // --- Methods --
    BDINode(NodeID node_id = 0, BDIOperationType op = BDIOperationType::META_NOP)
        : id(node_id), operation(op) {}
    // Helper to get expected input type (using a convention or op definition)
    BDIType getExpectedInputType(PortIndex input_idx) const {
        // TODO: Implement logic based on 'operation' type
        // This might require a separate definition table for operations
        return BDIType::UNKNOWN; // Placeholder
    }
     // Helper to get output type
    BDIType getOutputType(PortIndex output_idx) const {
         if (output_idx < data_outputs.size()) {
            return data_outputs[output_idx].type;
         }
         return BDIType::UNKNOWN;
    }
    // Basic validation
    bool validatePorts(const class BDIGraph& graph) const; // Declared here, defined later maybe in .cpp
 };
 } // namespace bdi::core::graph
 #endif // BDI_CORE_GRAPH_BDINODE_HPP
