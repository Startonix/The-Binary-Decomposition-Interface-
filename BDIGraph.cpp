// File: bdi/core/graph/BDIGraph.cpp
 #include "BDIGraph.hpp"
 #include "../types/BinaryEncoding.hpp" // Include encoders/decoders
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
 // --- Helper Functions for Stream I/O using BinaryEncoding --
template<typename T>
 bool write_encoded(std::ostream& os, T value) {
    BinaryData buffer;
    // Find the right encode function based on type T
    using namespace bdi::core::types;
         if constexpr (std::is_same_v<T, bool>)      { encode_bool(buffer, value); }
         else if constexpr (std::is_same_v<T, int8_t>)    { encode_i8(buffer, value); }
         else if constexpr (std::is_same_v<T, uint8_t>)   { encode_u8(buffer, value); }
         else if constexpr (std::is_same_v<T, int16_t>)   { encode_i16(buffer, value); }
         else if constexpr (std::is_same_v<T, uint16_t>)  { encode_u16(buffer, value); }
         else if constexpr (std::is_same_v<T, int32_t>)   { encode_i32(buffer, value); }
         else if constexpr (std::is_same_v<T, uint32_t>)  { encode_u32(buffer, value); }
         else if constexpr (std::is_same_v<T, int64_t>)   { encode_i64(buffer, value); }
         else if constexpr (std::is_same_v<T, uint64_t>)  { encode_u64(buffer, value); }
         // Add others: float, double, uintptr_t, BDIType, BDIOperationType (need encode functions)
         else if constexpr (std::is_enum_v<T>)           { encode_u64(buffer, static_cast<uint64_t>(value)); } // Assuming enums fit in u64 for simplicity
         else { /* Error or unsupported type */ return false;}
    os.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return os.good();
 }
 template<typename T>
 bool read_decoded(std::istream& is, T& out_value) {
    BinaryData buffer(sizeof(T)); // Read enough bytes for the type
    is.read(reinterpret_cast<char*>(buffer.data()), sizeof(T));
    if (!is) return false;
    size_t offset = 0;
    bool success = false;
     using namespace bdi::core::types;
         if constexpr (std::is_same_v<T, bool>)      { success = decode_bool(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, int8_t>)    { success = decode_i8(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, uint8_t>)   { success = decode_u8(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, int16_t>)   { success = decode_i16(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, uint16_t>)  { success = decode_u16(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, int32_t>)   { success = decode_i32(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, uint32_t>)  { success = decode_u32(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, int64_t>)   { success = decode_i64(buffer, offset, out_value); }
         else if constexpr (std::is_same_v<T, uint64_t>)  { success = decode_u64(buffer, offset, out_value); }
          // Add others: float, double, uintptr_t, BDIType, BDIOperationType
         else if constexpr (std::is_enum_v<T>)           { uint64_t temp_val; success = decode_u64(buffer, offset, temp_val); if(success) out_value =
 static_cast<T>(temp_val); }
         else { success = false; }
    return success;
 }
 // --- BDIGraph Methods (Updated Serialization) --
bool BDIGraph::serialize(std::ostream& os) const {
    using namespace bdi::core::types;
    const uint32_t MAGIC_NUMBER = 0xBADBEEF1; // Changed magic number
    const uint16_t VERSION = 2; // Incremented version
    // Use write_encoded helper for basic types
    if (!write_encoded(os, MAGIC_NUMBER)) return false;
    if (!write_encoded(os, VERSION)) return false;
    uint32_t name_len = static_cast<uint32_t>(name_.length());
    if (!write_encoded(os, name_len)) return false;
    os.write(name_.c_str(), name_len); // Write string directly
    uint64_t node_count = nodes_.size();
    if (!write_encoded(os, node_count)) return false;
    for (const auto& pair : nodes_) {
        const BDINode& node = *(pair.second);
        // Write Node ID, OpType, MetadataHandle, RegionID
        if (!write_encoded(os, node.id)) return false;
        if (!write_encoded(os, static_cast<uint16_t>(node.operation))) return false; // Assuming OpType fits uint16
        if (!write_encoded(os, node.metadata_handle)) return false;
        if (!write_encoded(os, node.region_id)) return false;
        // Write Payload (Type, Size, Data)
        if (!write_encoded(os, static_cast<uint8_t>(node.payload.type))) return false; // Assuming Type fits uint8
        uint64_t payload_size = node.payload.data.size();
        if (!write_encoded(os, payload_size)) return false;
        if (payload_size > 0) {
            os.write(reinterpret_cast<const char*>(node.payload.data.data()), payload_size);
        }
        // Write Data Inputs (Count, List of PortRefs)
        uint32_t data_inputs_count = static_cast<uint32_t>(node.data_inputs.size());
        if (!write_encoded(os, data_inputs_count)) return false;
        for (const auto& port_ref : node.data_inputs) {
             if (!write_encoded(os, port_ref.node_id)) return false;
             if (!write_encoded(os, port_ref.port_index)) return false;
        }
        // Write Data Outputs (Count, List of PortInfos)
        uint32_t data_outputs_count = static_cast<uint32_t>(node.data_outputs.size());
        if (!write_encoded(os, data_outputs_count)) return false;
        for (const auto& port_info : node.data_outputs) {
            if (!write_encoded(os, static_cast<uint8_t>(port_info.type))) return false;
            uint32_t port_name_len = static_cast<uint32_t>(port_info.name.length());
            if (!write_encoded(os, port_name_len)) return false;
            os.write(port_info.name.c_str(), port_name_len);
        }
        // Write Control Inputs/Outputs (Count, List of NodeIDs)
        auto write_nodeid_vector = [&](const std::vector<NodeID>& vec) {
            uint32_t count = static_cast<uint32_t>(vec.size());
            if (!write_encoded(os, count)) return false;
            for (const NodeID& id : vec) {
                if (!write_encoded(os, id)) return false;
            }
            return true;
        };
        if (!write_nodeid_vector(node.control_inputs)) return false;
        if (!write_nodeid_vector(node.control_outputs)) return false;
    }
    // TODO: Serialize MetadataStore content if needed?
    return os.good();
 }
 std::unique_ptr<BDIGraph> BDIGraph::deserialize(std::istream& is) {
     using namespace bdi::core::types;
     const uint32_t EXPECTED_MAGIC_NUMBER = 0xBADBEEF1;
     const uint16_t SUPPORTED_VERSION = 2;
     uint32_t magic_number;
     uint16_t version;
     if (!read_decoded(is, magic_number) || !read_decoded(is, version)) return nullptr;
     if (magic_number != EXPECTED_MAGIC_NUMBER || version != SUPPORTED_VERSION) {
        std::cerr << "Error: Invalid magic number or unsupported version during deserialization (V2)." << std::endl;
        return nullptr;
     }
     uint32_t name_len;
     if (!read_decoded(is, name_len)) return nullptr;
     std::string graph_name(name_len, '\0');
     is.read(&graph_name[0], name_len);
     if (!is) return nullptr;
     auto graph = std::make_unique<BDIGraph>(graph_name);
     uint64_t node_count;
     if (!read_decoded(is, node_count)) return nullptr;
     uint64_t max_id_seen = 0;
     for (uint64_t i = 0; i < node_count; ++i) {
         NodeID node_id;
         uint16_t op_type_raw;
         MetadataHandle meta_handle;
         RegionID region_id_val; // Changed name to avoid conflict
         if (!read_decoded(is, node_id)) return nullptr;
         if (!read_decoded(is, op_type_raw)) return nullptr;
         if (!read_decoded(is, meta_handle)) return nullptr;
         if (!read_decoded(is, region_id_val)) return nullptr;
         max_id_seen = std::max(max_id_seen, node_id);
         auto node = std::make_unique<BDINode>(node_id, static_cast<BDIOperationType>(op_type_raw));
         node->metadata_handle = meta_handle;
         node->region_id = region_id_val;
         uint8_t payload_type_raw;
         uint64_t payload_size;
         if (!read_decoded(is, payload_type_raw)) return nullptr;
         if (!read_decoded(is, payload_size)) return nullptr;
         node->payload.type = static_cast<BDIType>(payload_type_raw);
         if (payload_size > 0) {
             node->payload.data.resize(payload_size);
             is.read(reinterpret_cast<char*>(node->payload.data.data()), payload_size);
             if (!is) return nullptr;
         }
         // Read Data Inputs
         uint32_t data_inputs_count;
         if (!read_decoded(is, data_inputs_count)) return nullptr;
         node->data_inputs.resize(data_inputs_count);
         for (uint32_t j = 0; j < data_inputs_count; ++j) {
              if (!read_decoded(is, node->data_inputs[j].node_id)) return nullptr;
              if (!read_decoded(is, node->data_inputs[j].port_index)) return nullptr;
         }
         // Read Data Outputs
         uint32_t data_outputs_count;
         if (!read_decoded(is, data_outputs_count)) return nullptr;
         node->data_outputs.resize(data_outputs_count);
         for (uint32_t j = 0; j < data_outputs_count; ++j) {
             uint8_t port_type_raw;
             uint32_t port_name_len;
             if (!read_decoded(is, port_type_raw)) return nullptr;
             if (!read_decoded(is, port_name_len)) return nullptr;
             node->data_outputs[j].type = static_cast<BDIType>(port_type_raw);
             node->data_outputs[j].name.resize(port_name_len);
             is.read(&node->data_outputs[j].name[0], port_name_len);
             if (!is) return nullptr;
         }
         // Read Control Inputs/Outputs
          auto read_nodeid_vector = [&](std::vector<NodeID>& vec) {
             uint32_t count;
             if (!read_decoded(is, count)) return false;
             vec.resize(count);
             for (uint32_t k = 0; k < count; ++k) {
                 if (!read_decoded(is, vec[k])) return false;
             }
             return true;
         };
         if (!read_nodeid_vector(node->control_inputs)) return nullptr;
         if (!read_nodeid_vector(node->control_outputs)) return nullptr;
         graph->nodes_[node_id] = std::move(node);
     }
     graph->next_node_id_ = max_id_seen + 1;
     if (!is) return nullptr;
     return graph;
 }
 // ... rest of BDIGraph.cpp ...
 } // namespace bdi::core::graph
