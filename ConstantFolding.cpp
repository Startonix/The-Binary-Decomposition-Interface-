// File: bdi/optimizer/passes/ConstantFolding.cpp
 #include "ConstantFolding.hpp"
 #include "../../core/payload/TypedPayload.hpp"
 #include "../../core/types/TypeSystem.hpp"
 #include "../../runtime/ExecutionContext.hpp" // For payload<->variant conversion
 #include <iostream>
 #include <variant>
 namespace bdi::optimizer {
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 using namespace bdi::runtime;
 // Helper to get constant value directly from a node if it's a constant provider
 std::optional<BDIValueVariant> ConstantFolding::getConstantValueFromNode(NodeID node_id) {
    if (!current_graph_) return std::nullopt;
    auto node_opt = current_graph_->getNode(node_id);
    if (!node_opt) return std::nullopt;
    const BDINode& node = node_opt.value();
    // Define what constitutes a "constant" node for this pass
    // Option 1: Specific META_CONST operation type (preferred)
    // if (node.operation == BDIOperationType::META_CONST) { ... }
    // Option 2: Use NOP nodes with payloads (as used in tests)
    if (node.operation == BDIOperationType::META_NOP && node.payload.isValid() && node.payload.type != BDIType::VOID) {
        return ExecutionContext::payloadToVariant(node.payload);
    }
    // Option 3: Check if the output value is already known constant from previous folding
    PortRef output_port = {node_id, 0}; // Assume single output port 0 for simplicity
    auto it = constant_values_.find(output_port);
    if (it != constant_values_.end()) {
        return it->second;
    }
    return std::nullopt;
 }
 // Attempts to evaluate a node if all its inputs are constant
 std::optional<BDIValueVariant> ConstantFolding::evaluateConstantNode(BDINode& node) {
    // Check if operation is suitable for folding
    bool foldable_op = false;
    switch (node.operation) {
        case BDIOperationType::ARITH_ADD: case BDIOperationType::ARITH_SUB: case BDIOperationType::ARITH_MUL:
        case BDIOperationType::ARITH_DIV: case BDIOperationType::ARITH_MOD: case BDIOperationType::ARITH_NEG:
        case BDIOperationType::BIT_AND: case BDIOperationType::BIT_OR: case BDIOperationType::BIT_XOR: case BDIOperationType::BIT_NOT:
        case BDIOperationType::CMP_EQ: case BDIOperationType::CMP_NE: case BDIOperationType::CMP_LT: case BDIOperationType::CMP_LE:
        case BDIOperationType::CMP_GT: case BDIOperationType::CMP_GE:
        case BDIOperationType::LOGIC_AND: case BDIOperationType::LOGIC_OR: case BDIOperationType::LOGIC_XOR: case
 BDIOperationType::LOGIC_NOT:
        // Add relevant CONV_ ops if inputs are constant
            foldable_op = true;
            break;
        default:
            foldable_op = false;
            break;
    }
    if (!foldable_op || node.data_outputs.empty()) {
        return std::nullopt; // Cannot fold this operation or no output to produce
    }
    // Gather constant input values
    std::vector<BDIValueVariant> inputs;
    for (const auto& input_ref : node.data_inputs) {
        // Check our known constants map first
        auto const_it = constant_values_.find(input_ref);
        if (const_it != constant_values_.end()) {
            inputs.push_back(const_it->second);
        } else {
            // If not known constant, try getting directly from source node (if it's simple const)
            auto direct_const = getConstantValueFromNode(input_ref.node_id);
            if (direct_const) {
                 inputs.push_back(direct_const.value());
                 // Optional: Cache this found constant value
                 constant_values_[input_ref] = direct_const.value();
            } else {
                return std::nullopt; // Input is not constant, cannot fold this node
            }
        }
    }
    // --- Perform the evaluation (Simplified - reusing VM logic concepts) --
    // This requires a standalone evaluation capability, similar to the VM's executeNode
    // but operating directly on BDIValueVariants. This is non-trivial.
    // For this example, we'll STUB the evaluation for ADD I32.
    BDIValueVariant result_var = std::monostate{};
    if (node.operation == BDIOperationType::ARITH_ADD && inputs.size() == 2) {
         auto v1 = convertVariantTo<int32_t>(inputs[0]);
         auto v2 = convertVariantTo<int32_t>(inputs[1]);
         if (v1 && v2) {
             result_var = v1.value() + v2.value();
         }
    }
    // --- Implement evaluation logic for other foldable operations --
    else {
        // std::cout << "  Folding eval not implemented for op " << static_cast<int>(node.operation) << std::endl;
        return std::nullopt; // Evaluation not implemented for this op
    }
    if (std::holds_alternative<std::monostate>(result_var)) {
        return std::nullopt; // Evaluation failed
    }
    return result_var;
 }
 // Replaces the node with a constant, rewiring consumers
 void ConstantFolding::replaceNodeWithConstant(BDINode& node_to_replace, const BDIValueVariant& constant_result) {
     if (!current_graph_) return;
     NodeID old_node_id = node_to_replace.id;
     std::cout << "  Folding node " << old_node_id << " to constant." << std::endl;
     // 1. Create Payload for the constant result
     TypedPayload constant_payload = ExecutionContext::variantToPayload(constant_result);
     if (constant_payload.type == BDIType::UNKNOWN) {
          std::cerr << "    Error: Failed to create payload for constant result." << std::endl;
          return;
     }
     // 2. Create the new constant node (using NOP for now)
     NodeID new_const_node_id = current_graph_->addNode(BDIOperationType::META_NOP); // Or META_CONST if defined
     BDINode* new_const_node = current_graph_->getNodeMutable(new_const_node_id); // Need mutable access via graph
     if (!new_const_node) {
         std::cerr << "    Error: Failed to create new constant node." << std::endl;
         current_graph_->removeNode(new_const_node_id); // Clean up potentially added node
         return;
     }
     new_const_node->payload = constant_payload;
     // Define the output port matching the original node's output type
     // Assume single output port 0 for simplicity
     if (!node_to_replace.data_outputs.empty()) {
         new_const_node->data_outputs.push_back({constant_payload.type, node_to_replace.data_outputs[0].name + "_folded"});
     } else {
         // Handle case where original node had no output? Maybe error.
         std::cerr << "    Warning: Original node " << old_node_id << " had no output port defined." << std::endl;
          new_const_node->data_outputs.push_back({constant_payload.type, "_folded"});
     }
     // 3. Find all consumers of the original node's output(s) and rewire them
     // Iterate through *all* nodes in the graph to find consumers
     std::vector<NodeID> consumers_to_update;
     for (auto& pair : *current_graph_) { // Iterate using graph's iterator
         if (!pair.second) continue;
         BDINode& potential_consumer = *(pair.second);
         bool updated = false;
         for (PortRef& input_ref : potential_consumer.data_inputs) {
             // Check if this input comes from the node we are replacing
             // Assume output port 0 for simplicity
             if (input_ref.node_id == old_node_id && input_ref.port_index == 0) {
                 input_ref.node_id = new_const_node_id; // Rewire to the new const node
                 input_ref.port_index = 0; // Assuming new const node also uses output port 0
                 updated = true;
                 markGraphModified();
             }
         }
     // 4. Handle Control Flow - VERY Simplified
     // Connect control inputs of old node -> new const node
     // Connect new const node -> control outputs of old node
     // This preserves the linear flow *around* the folded operation.
     // More complex control flow requires careful handling.
      for (NodeID pred_id : node_to_replace.control_inputs) {
          current_graph_->connectControl(pred_id, new_const_node_id);
          // Also need to remove control output from pred_id pointing to old_node_id
          BDINode* pred_node = current_graph_->getNodeMutable(pred_id);
          if (pred_node) {
              std::erase(pred_node->control_outputs, old_node_id);
          }
      }
      for (NodeID succ_id : node_to_replace.control_outputs) {
          current_graph_->connectControl(new_const_node_id, succ_id);
           // Also need to remove control input from succ_id pointing to old_node_id
           BDINode* succ_node = current_graph_->getNodeMutable(succ_id);
           if (succ_node) {
                std::erase(succ_node->control_inputs, old_node_id);
           }
      }
     // 5. Remove the original node (or mark it for removal by a dead code pass)
     // Immediate removal can invalidate iterators if not careful.
     // For simplicity here, we just remove it. Be cautious in real implementation.
     std::cout << "    Removing original node " << old_node_id << std::endl;
     current_graph_->removeNode(old_node_id); // This also cleans up dangling edges TO old node
 }
 void ConstantFolding::visitGraph(BDIGraph& graph) {
    current_graph_ = &graph;
    constant_values_.clear();
    bool changed_in_pass;
    // Iterate multiple times until no more folding occurs in a pass
    // Or use a worklist approach for efficiency
    int max_passes = 10; // Limit iterations
    for (int pass = 0; pass < max_passes; ++pass) {
        changed_in_pass = false;
        std::vector<NodeID> nodes_to_process; // Process nodes in a stable order
        for(const auto& pair : graph) {
            nodes_to_process.push_back(pair.first);
        }
        // Optional: Sort nodes_to_process topologically if graph is DAG
        for (NodeID node_id : nodes_to_process) {
             auto node_ptr = graph.getNodeMutable(node_id); // Use graph's mutable access
             if (!node_ptr) continue; // Node might have been removed by previous folding
            BDINode& node = *node_ptr;
            // Skip nodes already processed or identified as constant providers
            if (node.operation == BDIOperationType::META_NOP && node.payload.isValid()) continue;
            // Try to evaluate the node
            std::optional<BDIValueVariant> result = evaluateConstantNode(node);
            if (result) {
                // Successfully folded! Store result and replace the node
                constant_values_[{node.id, 0}] = result.value(); // Cache constant result
                replaceNodeWithConstant(node, result.value());
                changed_in_pass = true;
                // Since we removed the node, the outer loop might have issues if using iterators directly.
                // Using a list of IDs helps here. We should ideally break/restart pass or use worklist.
                break; // Restart pass after modification for simplicity here
            }
        }
         if (!changed_in_pass) break; // No changes in this pass, exit loop
    }
   current_graph_ = nullptr; // Clear graph pointer
 }
 } // namespace bdi::optimizer
