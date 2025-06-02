// File: bdi/frontend/api/GraphBuilder.hpp
 #ifndef BDI_FRONTEND_API_GRAPHBUILDER_HPP
 #define BDI_FRONTEND_API_GRAPHBUILDER_HPP
 #include "../../core/graph/BDIGraph.hpp"
 #include "../../core/graph/BDINode.hpp"
 #include "../../core/types/BDITypes.hpp"
 #include <memory>
 #include <string>
 namespace bdi::frontend::api {
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::BDINode;
 using bdi::core::graph::NodeID;
 using bdi::core::graph::PortIndex;
 using bdi::core::graph::PortRef;
 using bdi::core::graph::BDIOperationType;
 using bdi::core::types::BDIType;
 using bdi::core::payload::TypedPayload;
 // Provides a convenient API for programmatically constructing BDI graphs.
 class GraphBuilder {
 public:
    GraphBuilder(const std::string& graph_name = "built_graph");
    // Create a new node
    NodeID addNode(BDIOperationType op, const std::string& debug_name = ""); // Name might go in metadata
    // Set immediate payload for a node
    bool setNodePayload(NodeID node_id, TypedPayload payload);
    // Define an output port for a node
    bool defineDataOutput(NodeID node_id, PortIndex output_idx, BDIType type, const std::string& name = "");
    // Connect data flow: from_node::from_port -> to_node::to_input
    bool connectData(NodeID from_node_id, PortIndex from_port_idx, NodeID to_node_id, PortIndex to_input_idx);
    // Connect control flow: from_node -> to_node
    bool connectControl(NodeID from_node_id, NodeID to_node_id);
    // TODO: Add methods for metadata, region assignment, etc.
    // Finalize and retrieve the built graph
    // Transfers ownership of the graph to the caller
    std::unique_ptr<BDIGraph> finalizeGraph();
    // Get a reference to the graph being built (use with caution regarding ownership)
    BDIGraph& getGraph();
    const BDIGraph& getGraph() const;
private:
    std::unique_ptr<BDIGraph> graph_;
    // Helper to get mutable node pointer
    BDINode* getNodeMutable(NodeID node_id);
 };
 } // namespace bdi::frontend::api
 #endif // BDI_FRONTEND_API_GRAPHBUILDER_HPP
