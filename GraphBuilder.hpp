// File: bdi/frontend/api/GraphBuilder.hpp
 #ifndef BDI_FRONTEND_API_GRAPHBUILDER_HPP
 #define BDI_FRONTEND_API_GRAPHBUILDER_HPP
 #include "../../core/graph/BDIGraph.hpp"
 #include "../../core/graph/BDINode.hpp"
 #include "../../core/types/BDITypes.hpp"
 #include "../../meta/MetadataStore.hpp" // Include MetadataStore
 #include <memory>
 #include <string>
 #include <any> // For optional metadata variant
 namespace bdi::frontend::api {
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::BDINode;
 using bdi::core::graph::NodeID;
 using bdi::core::graph::PortIndex;
 using bdi::core::graph::PortRef;
 using bdi::core::graph::BDIOperationType;
 using bdi::core::types::BDIType;
 using bdi::core::payload::TypedPayload;
 using namespace bdi::meta;
 // Provides a convenient API for programmatically constructing BDI graphs.
 class GraphBuilder {
 public:
    // Constructor now takes MetadataStore reference
    GraphBuilder(MetadataStore& metadata_store, const std::string& graph_name = "built_graph");
    // Create a new node
    // Add node, optionally providing initial metadata
    NodeID addNode(BDIOperationType op, const std::string& debug_name = "", std::optional<MetadataVariant> initial_metadata = std::nullopt);
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
    // Set or update metadata for a specific node
    bool setNodeMetadata(NodeID node_id, MetadataVariant metadata);
    // Get metadata handle (optional)
    std::optional<MetadataHandle> getNodeMetadataHandle(NodeID node_id);
    std::unique_ptr<BDIGraph> finalizeGraph();
    // Get a reference to the graph being built (use with caution regarding ownership)
    BDIGraph& getGraph();
    const BDIGraph& getGraph() const;
private:
    MetadataStore& metadata_store_; // Reference to external store
    std::unique_ptr<BDIGraph> graph_;
    // Helper to get mutable node pointer
    BDINode* getNodeMutable(NodeID node_id);
 };
 } // namespace bdi::frontend::api
 #endif // BDI_FRONTEND_API_GRAPHBUILDER_HPP
