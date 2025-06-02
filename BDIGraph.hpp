// File: bdi/core/graph/BDIGraph.hpp
 #ifndef BDI_CORE_GRAPH_BDIGRAPH_HPP
 #define BDI_CORE_GRAPH_BDIGRAPH_HPP
 #include "BDINode.hpp"
 #include <unordered_map>
 #include <vector>
 #include <optional>
 #include <string>
 #include <memory> // For std::unique_ptr
 namespace bdi::core::graph {
 class BDIGraph {
 public:
    BDIGraph(std::string graph_name = "unnamed_bdi_graph")
        : name_(std::move(graph_name)), next_node_id_(1) {} // Start IDs from 1 (0 reserved?)
    // --- Graph Modification --
    // Add a new node, takes ownership if unique_ptr provided
    // Returns the assigned NodeID
    NodeID addNode(std::unique_ptr<BDINode> node);
    NodeID addNode(BDIOperationType op = BDIOperationType::META_NOP); // Creates node internally
    // Remove a node (potentially complex due to edge cleanup)
    bool removeNode(NodeID node_id);
    // Add data dependency edge: output 'from_port_idx' of 'from_node_id' -> input 'to_input_idx' of 'to_node_id'
    bool connectData(NodeID from_node_id, PortIndex from_port_idx, NodeID to_node_id, PortIndex to_input_idx);
    // Add control flow edge: from 'from_node_id' -> to 'to_node_id' (appends to output/input lists)
    bool connectControl(NodeID from_node_id, NodeID to_node_id);
    // TODO: Add methods for conditional control flow, removing edges
    // --- Graph Query --
    std::optional<std::reference_wrapper<BDINode>> getNode(NodeID node_id);
    std::optional<std::reference_wrapper<const BDINode>> getNode(NodeID node_id) const;
    size_t getNodeCount() const { return nodes_.size(); }
    const std::string& getName() const { return name_; }
    // Get nodes providing data input to a specific input port of a node
    std::vector<PortRef> getDataSourcesFor(NodeID node_id, PortIndex input_idx) const;
    // Get nodes consuming data output from a specific output port of a node
    std::vector<PortRef> getDataConsumersFor(NodeID node_id, PortIndex output_idx) const;
    // Get control flow predecessors/successors
    std::vector<NodeID> getControlPredecessors(NodeID node_id) const;
    std::vector<NodeID> getControlSuccessors(NodeID node_id) const;
    // --- Validation --
    // Perform comprehensive validation checks (types, connections, cycles if needed)
    bool validateGraph() const;
    // --- Iteration --
    // Provide iterators to walk through nodes (const and non-const)
    auto begin() { return nodes_.begin(); }
    auto end() { return nodes_.end(); }
    auto begin() const { return nodes_.cbegin(); }
    auto end() const { return nodes_.cend(); }
    auto cbegin() const { return nodes_.cbegin(); }
    auto cend() const { return nodes_.cend(); }
    // --- Serialization --
    // TODO: bool serialize(std::ostream& os) const;
    // TODO: static std::unique_ptr<BDIGraph> deserialize(std::istream& is);
 private:
    std::string name_;
    std::unordered_map<NodeID, std::unique_ptr<BDINode>> nodes_; // Using unique_ptr for ownership
    NodeID next_node_id_;
    // Helper to get mutable node pointer
    BDINode* getNodeMutable(NodeID node_id);
 };
 // Implementation of BDINode::validatePorts needs BDIGraph definition
 inline bool BDINode::validatePorts(const BDIGraph& graph) const {
    // Example validation: check if nodes referenced in data_inputs exist
    for (const auto& port_ref : data_inputs) {
        if (!graph.getNode(port_ref.node_id)) {
            return false; // Referenced node doesn't exist
        }
        // TODO: Check if the source node actually has the referenced output port
        // TODO: Check type compatibility between source output and this input
    }
    // TODO: Add more checks for control flow etc.
    return true;
 }
 } // namespace bdi::core::graph
 #endif // BDI_CORE_GRAPH_BDIGRAPH_HPP
