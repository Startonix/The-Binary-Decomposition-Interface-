// File: bdi/core/graph/BDIGraph.cpp
 #include "BDIGraph.hpp"
 #include <stdexcept>
 #include <vector>
 #include <algorithm>
 #include <fstream> // For serialization
 #include <iostream> // For serialization debugging
 namespace bdi::core::graph {
 // --- Graph Modification --
NodeID BDIGraph::addNode(std::unique_ptr<BDINode> node) {
    if (!node) {
        throw std::invalid_argument("Cannot add null node");
    }
    NodeID id = next_node_id_++;
    node->id = id;
    nodes_[id] = std::move(node);
    return id;
 }
 NodeID BDIGraph::addNode(BDIOperationType op) {
    NodeID id = next_node_id_++;
    auto node = std::make_unique<BDINode>(id, op);
    nodes_[id] = std::move(node);
    return id;
 }
 bool BDIGraph::removeNode(NodeID node_id) {
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return false; // Node doesn't exist
    }
    // Critical: Remove all references TO this node from others
    std::vector<NodeID> nodes_to_check;
    for(const auto& pair : nodes_) {
        nodes_to_check.push_back(pair.first);
    }
    for(NodeID current_id : nodes_to_check) {
        if (current_id == node_id) continue; // Skip self
        BDINode* current_node = getNodeMutable(current_id);
        if (!current_node) continue; // Should not happen if nodes_ map is consistent
        // Remove data input references
        std::erase_if(current_node->data_inputs,
                      [node_id](const PortRef& ref) { return ref.node_id == node_id; });
        // Remove control input references
        std::erase(current_node->control_inputs, node_id);
         // Remove control output references
        std::erase(current_node->control_outputs, node_id);
    }
    // Finally, remove the node itself
    nodes_.erase(it);
    return true;
 }
 bool BDIGraph::connectData(NodeID from_node_id, PortIndex from_port_idx, NodeID to_node_id, PortIndex to_input_idx) {
    BDINode* from_node = getNodeMutable(from_node_id);
    BDINode* to_node = getNodeMutable(to_node_id);
    if (!from_node || !to_node) {
        return false; // One or both nodes don't exist
    }
    // Validate port indices
    if (from_port_idx >= from_node->data_outputs.size()) {
        return false; // Source port index out of bounds
    }
    // Ensure the target input port index exists or can be created
    if (to_input_idx >= to_node->data_inputs.size()) {
        to_node->data_inputs.resize(to_input_idx + 1); // Resize if accessing a new input index
    } else if (to_node->data_inputs[to_input_idx].node_id != 0) {
         // Optional: Warn or error if overwriting an existing connection?
         // std::cerr << "Warning: Overwriting existing data connection to node "
         //           << to_node_id << " input " << to_input_idx << std::endl;
    }
    // TODO: Perform type compatibility check here before connecting?
    // BDIType from_type = from_node->getOutputType(from_port_idx);
    // BDIType to_type = to_node->getExpectedInputType(to_input_idx);
    // if (!types::TypeSystem::areCompatible(from_type, to_type) && !types::TypeSystem::canImplicitlyConvert(from_type, to_type)) {
    //     return false; // Type mismatch
    // }
    to_node->data_inputs[to_input_idx] = {from_node_id, from_port_idx};
    return true;
 }
 bool BDIGraph::connectControl(NodeID from_node_id, NodeID to_node_id) {
    BDINode* from_node = getNodeMutable(from_node_id);
    BDINode* to_node = getNodeMutable(to_node_id);
    if (!from_node || !to_node) {
        return false; // One or both nodes don't exist
    }
    // Avoid duplicate control edges (optional, depends on semantics)
    if (std::find(from_node->control_outputs.begin(), from_node->control_outputs.end(), to_node_id) == from_node->control_outputs.end()) {
       from_node->control_outputs.push_back(to_node_id);
    }
    if (std::find(to_node->control_inputs.begin(), to_node->control_inputs.end(), from_node_id) == to_node->control_inputs.end()) {
        to_node->control_inputs.push_back(from_node_id);
    }
    return true;
 }
 // --- Graph Query --
std::optional<std::reference_wrapper<BDINode>> BDIGraph::getNode(NodeID node_id) {
    auto it = nodes_.find(node_id);
    if (it != nodes_.end() && it->second) {
        return std::ref(*(it->second));
    }
    return std::nullopt;
 }
 std::optional<std::reference_wrapper<const BDINode>> BDIGraph::getNode(NodeID node_id) const {
    auto it = nodes_.find(node_id);
    if (it != nodes_.end() && it->second) {
        return std::cref(*(it->second));
    }
    return std::nullopt;
 }
// Helper to get mutable node pointer
 BDINode* BDIGraph::getNodeMutable(NodeID node_id) {
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        return it->second.get();
    }
    return nullptr;
 }
 // --- Validation --
bool BDIGraph::validateGraph() const {
    for (const auto& pair : nodes_) {
        if (!pair.second) return false; // Should not happen
        if (!pair.second->validatePorts(*this)) {
             std::cerr << "Validation failed for node " << pair.first << std::endl;
            return false;
        }
    }
    // Add global graph checks here (e.g., single START node?)
    return true;
 }
 // --- Simple Binary Serialization --
// NOTE: This is a *very* basic example. A robust implementation needs:
 // - Error handling (exceptions or error codes)
 // - Versioning
 // - Endianness handling
 // - Efficient encoding of variable-size data (e.g., length prefixes)
 // - Potentially handling pointers/references carefully if graph is loaded at different address
 bool BDIGraph::serialize(std::ostream& os) const {
    const uint32_t MAGIC_NUMBER = 0xDEADBEEF;
    const uint16_t VERSION = 1;
    // 1. Header
    os.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
    os.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));
    // 2. Graph Name
    uint32_t name_len = static_cast<uint32_t>(name_.length());
    os.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
    os.write(name_.c_str(), name_len);
    // 3. Node Count
    uint64_t node_count = nodes_.size();
    os.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    // 4. Nodes
    for (const auto& pair : nodes_) {
        const BDINode& node = *(pair.second);
        NodeID current_id = pair.first; // Same as node.id
        // Write Node ID (redundant if using map key, but good for validation)
        os.write(reinterpret_cast<const char*>(¤t_id), sizeof(current_id));
        // Write Operation Type
        os.write(reinterpret_cast<const char*>(&node.operation), sizeof(node.operation));
        // Write Payload (Type, Size, Data)
        os.write(reinterpret_cast<const char*>(&node.payload.type), sizeof(node.payload.type));
        uint64_t payload_size = node.payload.data.size();
        os.write(reinterpret_cast<const char*>(&payload_size), sizeof(payload_size));
        if (payload_size > 0) {
            os.write(reinterpret_cast<const char*>(node.payload.data.data()), payload_size);
        }
        // Write Data Inputs (Count, List of PortRefs)
        uint32_t data_inputs_count = static_cast<uint32_t>(node.data_inputs.size());
        os.write(reinterpret_cast<const char*>(&data_inputs_count), sizeof(data_inputs_count));
        for (const auto& port_ref : node.data_inputs) {
 os.write(reinterpret_cast<const char*>(&port_ref), sizeof(port_ref));
        }
        // Write Data Outputs (Count, List of PortInfos)
        uint32_t data_outputs_count = static_cast<uint32_t>(node.data_outputs.size());
        os.write(reinterpret_cast<const char*>(&data_outputs_count), sizeof(data_outputs_count));
        for (const auto& port_info : node.data_outputs) {
            os.write(reinterpret_cast<const char*>(&port_info.type), sizeof(port_info.type));
            uint32_t port_name_len = static_cast<uint32_t>(port_info.name.length());
            os.write(reinterpret_cast<const char*>(&port_name_len), sizeof(port_name_len));
            os.write(port_info.name.c_str(), port_name_len);
        }
        // Write Control Inputs (Count, List of NodeIDs)
        uint32_t control_inputs_count = static_cast<uint32_t>(node.control_inputs.size());
        os.write(reinterpret_cast<const char*>(&control_inputs_count), sizeof(control_inputs_count));
        os.write(reinterpret_cast<const char*>(node.control_inputs.data()), control_inputs_count * sizeof(NodeID));
        // Write Control Outputs (Count, List of NodeIDs)
        uint32_t control_outputs_count = static_cast<uint32_t>(node.control_outputs.size());
        os.write(reinterpret_cast<const char*>(&control_outputs_count), sizeof(control_outputs_count));
        os.write(reinterpret_cast<const char*>(node.control_outputs.data()), control_outputs_count * sizeof(NodeID));
        // Write Metadata Handle and Region ID
        os.write(reinterpret_cast<const char*>(&node.metadata_handle), sizeof(node.metadata_handle));
        os.write(reinterpret_cast<const char*>(&node.region_id), sizeof(node.region_id));
    }
    return os.good();
 }
 std::unique_ptr<BDIGraph> BDIGraph::deserialize(std::istream& is) {
     const uint32_t EXPECTED_MAGIC_NUMBER = 0xDEADBEEF;
     const uint16_t SUPPORTED_VERSION = 1;
    // 1. Header
    uint32_t magic_number;
    uint16_t version;
    is.read(reinterpret_cast<char*>(&magic_number), sizeof(magic_number));
    is.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!is || magic_number != EXPECTED_MAGIC_NUMBER || version != SUPPORTED_VERSION) {
        std::cerr << "Error: Invalid magic number or unsupported version during deserialization." << std::endl;
        return nullptr;
    }
    // 2. Graph Name
    uint32_t name_len;
    is.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    std::string graph_name(name_len, '\0');
    is.read(&graph_name[0], name_len);
    auto graph = std::make_unique<BDIGraph>(graph_name);
    // 3. Node Count
    uint64_t node_count;
    is.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    // 4. Nodes
    uint64_t max_id_seen = 0;
    for (uint64_t i = 0; i < node_count; ++i) {
        NodeID node_id;
        is.read(reinterpret_cast<char*>(&node_id), sizeof(node_id));
        max_id_seen = std::max(max_id_seen, node_id);
        auto node = std::make_unique<BDINode>(node_id);
        // Read Operation Type
        is.read(reinterpret_cast<char*>(&node->operation), sizeof(node->operation));
        // Read Payload
        is.read(reinterpret_cast<char*>(&node->payload.type), sizeof(node->payload.type));
        uint64_t payload_size;
        is.read(reinterpret_cast<char*>(&payload_size), sizeof(payload_size));
        if (payload_size > 0) {
            node->payload.data.resize(payload_size);
            is.read(reinterpret_cast<char*>(node->payload.data.data()), payload_size);
        }
        // Read Data Inputs
        uint32_t data_inputs_count;
        is.read(reinterpret_cast<char*>(&data_inputs_count), sizeof(data_inputs_count));
        node->data_inputs.resize(data_inputs_count);
        for (uint32_t j = 0; j < data_inputs_count; ++j) {
             is.read(reinterpret_cast<char*>(&node->data_inputs[j]), sizeof(PortRef));
        }
        // Read Data Outputs
        uint32_t data_outputs_count;
        is.read(reinterpret_cast<char*>(&data_outputs_count), sizeof(data_outputs_count));
        node->data_outputs.resize(data_outputs_count);
        for (uint32_t j = 0; j < data_outputs_count; ++j) {
            is.read(reinterpret_cast<char*>(&node->data_outputs[j].type), sizeof(BDIType));
            uint32_t port_name_len;
            is.read(reinterpret_cast<char*>(&port_name_len), sizeof(port_name_len));
            node->data_outputs[j].name.resize(port_name_len);
            is.read(&node->data_outputs[j].name[0], port_name_len);
        }
        // Read Control Inputs
        uint32_t control_inputs_count;
        is.read(reinterpret_cast<char*>(&control_inputs_count), sizeof(control_inputs_count));
        node->control_inputs.resize(control_inputs_count);
        is.read(reinterpret_cast<char*>(node->control_inputs.data()), control_inputs_count * sizeof(NodeID));
        // Read Control Outputs
        uint32_t control_outputs_count;
        is.read(reinterpret_cast<char*>(&control_outputs_count), sizeof(control_outputs_count));
        node->control_outputs.resize(control_outputs_count);
        is.read(reinterpret_cast<char*>(node->control_outputs.data()), control_outputs_count * sizeof(NodeID));
        // Read Metadata Handle and Region ID
        is.read(reinterpret_cast<char*>(&node->metadata_handle), sizeof(node->metadata_handle));
        is.read(reinterpret_cast<char*>(&node->region_id), sizeof(node->region_id));
        if (!is) {
             std::cerr << "Error: Stream error during node deserialization (node " << node_id << ")." << std::endl;
             return nullptr;
        }
        // Add the constructed node to the graph map
        graph->nodes_[node_id] = std::move(node);
    }
    // Restore the next_node_id counter
    graph->next_node_id_ = max_id_seen + 1;
    if (!is) {
         std::cerr << "Error: Stream error after reading nodes." << std::endl;
         return nullptr;
    }
    return graph;
 }
 } // namespace bdi::core::graph
