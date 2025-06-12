// File: bdi/frontend/api/GraphBuilder.cpp
 #include "GraphBuilder.hpp"
 #include <stdexcept> // For std::runtime_error
 namespace bdi::frontend::api {
 // Constructor takes MetadataStore reference
 GraphBuilder::GraphBuilder(MetadataStore& metadata_store, const std::string& graph_name)
 : metadata_store_(metadata_store) { // Store reference
    graph_ = std::make_unique<BDIGraph>(graph_name);
    if (!graph_) {
        throw std::runtime_error("Failed to allocate BDIGraph in GraphBuilder constructor");
    }
 }
 NodeID GraphBuilder::addNode(BDIOperationType op, const std::string& debug_name, std::optional<MetadataVariant> initial_metadata) {
    // TODO: Incorporate debug_name into metadata when MetadataStore is available
    if (!graph_) {
        throw std::runtime_error("GraphBuilder has no valid graph (perhaps after finalize?)");
    }
     NodeID node_id = graph_->addNode(op);
    BDINode* node = getNodeMutable(node_id);
    if (node) {
         // Add metadata if provided, otherwise default (monostate)
         MetadataVariant meta_to_add = initial_metadata.value_or(std::monostate{});
         // Add semantic tag if debug name provided
         if (!debug_name.empty() && std::holds_alternative<std::monostate>(meta_to_add)) {
             meta_to_add = SemanticTag{"", debug_name}; // Add name as description
         } else if (!debug_name.empty() && std::holds_alternative<SemanticTag>(meta_to_add)) {
             std::get<SemanticTag>(meta_to_add).description = debug_name; // Overwrite/set description
         }
         node->metadata_handle = metadata_store_.addMetadata(meta_to_add);
    }
    return node_id;
 }
    return graph_->addNode(op);
 }
// Set/Update Metadata
 bool GraphBuilder::setNodeMetadata(NodeID node_id, MetadataVariant metadata) {
     BDINode* node = getNodeMutable(node_id);
     if (node) {
        // Check if node already has metadata assigned
        if (node->metadata_handle != 0) {
            // Update existing metadata entry
            return metadata_store_.updateMetadata(node->metadata_handle, std::move(metadata));
        } else {
            // Add new metadata and assign handle
            node->metadata_handle = metadata_store_.addMetadata(std::move(metadata));
            return node->metadata_handle != 0;
        }
     }
     return false;
 }
 std::optional<MetadataHandle> GraphBuilder::getNodeMetadataHandle(NodeID node_id) {
     auto node_opt = graph_->getNode(node_id);
     if (node_opt) {
         return node_opt.value().get().metadata_handle;
     }
     return std::nullopt;
 }
 bool GraphBuilder::setNodePayload(NodeID node_id, TypedPayload payload) {
    BDINode* node = getNodeMutable(node_id);
    if (node) {
        node->payload = std::move(payload);
        return true;
    }
    return false;
 }
 bool GraphBuilder::defineDataOutput(NodeID node_id, PortIndex output_idx, BDIType type, const std::string& name) {
     BDINode* node = getNodeMutable(node_id);
    if (node) {
        if (output_idx >= node->data_outputs.size()) {
            node->data_outputs.resize(output_idx + 1);
        }
        node->data_outputs[output_idx] = PortInfo(type, name);
        return true;
    }
    return false;
 }
 bool GraphBuilder::connectData(NodeID from_node_id, PortIndex from_port_idx, NodeID to_node_id, PortIndex to_input_idx) {
    if (!graph_) {
        throw std::runtime_error("GraphBuilder has no valid graph (perhaps after finalize?)");
    }
    return graph_->connectData(from_node_id, from_port_idx, to_node_id, to_input_idx);
 }
 bool GraphBuilder::connectControl(NodeID from_node_id, NodeID to_node_id) {
    if (!graph_) {
        throw std::runtime_error("GraphBuilder has no valid graph (perhaps after finalize?)");
    }
    return graph_->connectControl(from_node_id, to_node_id);
 }
 std::unique_ptr<BDIGraph> GraphBuilder::finalizeGraph() {
    if (!graph_) {
         throw std::runtime_error("GraphBuilder cannot finalize - graph already finalized or invalid.");
    }
    // Perform final validation before handing over ownership?
    if (!graph_->validateGraph()) {
        std::cerr << "Warning: Finalizing graph with validation errors." << std::endl;
        // Depending on policy, could throw or return nullptr
    }
    return std::move(graph_); // Transfer ownership
 }
 BDIGraph& GraphBuilder::getGraph() {
    if (!graph_) {
        throw std::runtime_error("GraphBuilder has no valid graph (perhaps after finalize?)");
    }
    return *graph_;
 }
 const BDIGraph& GraphBuilder::getGraph() const {
     if (!graph_) {
        throw std::runtime_error("GraphBuilder has no valid graph (perhaps after finalize?)");
 }
    return *graph_;
 }
 // Helper to get mutable node pointer
 BDINode* GraphBuilder::getNodeMutable(NodeID node_id) {
    if (!graph_) {
        return nullptr;
    }
    auto it = graph_->nodes_.find(node_id); // Accessing private member - OK if this is a friend or part of graph
    if (it != graph_->nodes_.end()) {
        return it->second.get();
    }
    return nullptr;
 }
 } // namespace bdi::frontend::api
 // File: bdi/meta/MetadataStore.hpp
 #ifndef BDI_META_METADATASTORE_HPP
 #define BDI_META_METADATASTORE_HPP
 #include <cstdint>
 #include <string>
 #include <vector>
 #include <unordered_map>
 #include <variant>
 #include <atomic>
 #include <memory> // For std::shared_ptr if metadata can be shared
 namespace bdi::meta {
 using MetadataHandle = uint64_t;
 // --- Metadata Entry Structures --
struct SemanticTag {
    std::string dsl_source_ref; // e.g., "MyDSL:FunctionX:Line42"
    std::string description;
    // Add provenance: creation time, author tool/agent ID
 };
 struct ProofTag {
    enum class ProofSystem { NONE, INTERNAL_HASH, LEAN_HASH, COQ_HASH } system = ProofSystem::NONE;
    std::vector<std::byte> proof_data_hash;
 };
 struct HardwareHints {
    enum class CacheLocality { HINT_NONE, HINT_L1, HINT_L2, HINT_L3 } cache_hint = CacheLocality::HINT_NONE;
    uint32_t preferred_compute_unit_id = 0; // e.g., Core 5, GPU SM 2
    bool requires_simd_alignment = false;
    // Add more hints: latency class, vector width preference etc.
 };
 struct EntropyInfo {
    double estimated_shannon_entropy = 0.0;
    uint64_t estimated_kolmogorov_complexity = 0; // Approximate
 };
 struct AttentionInfo {
    float attention_score = 0.0f; // Normalized?
 };
 // Use std::variant to hold different metadata types associated with a single handle
 using MetadataVariant = std::variant<
    std::monostate, // Represents no specific metadata or empty state
    SemanticTag,
    ProofTag,
    HardwareHints,
    EntropyInfo,
    AttentionInfo
    // Add other metadata types here
 >;
 // Class to manage metadata entries
 class MetadataStore {
 public:
    MetadataStore() : next_handle_(1) {} // Start handles from 1
    // Add new metadata, returns a handle
    MetadataHandle addMetadata(MetadataVariant metadata);
    // Retrieve metadata by handle (returns pointer, ownership remains here)
    const MetadataVariant* getMetadata(MetadataHandle handle) const;
    MetadataVariant* getMetadataMutable(MetadataHandle handle); // Use with care
    // Update existing metadata
    bool updateMetadata(MetadataHandle handle, MetadataVariant metadata);
    // Remove metadata (consider reference counting if shared)
    bool removeMetadata(MetadataHandle handle);
 private:
    std::unordered_map<MetadataHandle, MetadataVariant> store_;
    std::atomic<MetadataHandle> next_handle_; // Simple atomic counter for handle generation
 };
 } // namespace bdi::meta
 #endif // BDI_META_METADATASTORE_HPP
