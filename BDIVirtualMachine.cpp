// File: bdi/runtime/BDIVirtualMachine.cpp
 #include "BDIVirtualMachine.hpp"
 #include "../core/graph/BDINode.hpp" // Need full BDINode definition
 #include <iostream> // For stubs/debug output
 #include <stdexcept> // For errors
 namespace bdi::runtime {
 BDIVirtualMachine::BDIVirtualMachine() : current_node_id_(0) {
    // TODO: Initialize MemoryManager, Scheduler, etc.
    std::cout << "BDIVirtualMachine: Initialized (Stub)." << std::endl;
 }
 bool BDIVirtualMachine::execute(BDIGraph& graph, NodeID entry_node_id) {
    std::cout << "BDIVirtualMachine: Starting execution at Node " << entry_node_id << std::endl;
    current_node_id_ = entry_node_id;
    // Basic execution loop limit to prevent infinite loops in stub
    int execution_step_limit = 1000;
    int steps = 0;
    while (current_node_id_ != 0 && steps < execution_step_limit) { // Assuming 0 is invalid/halt ID
        if (!fetchDecodeExecuteCycle(graph)) {
            std::cerr << "BDIVirtualMachine: Execution cycle failed or halted." << std::endl;
            return false; // Halted or error occurred
        }
        steps++;
    }
    if (steps >= execution_step_limit) {
         std::cerr << "BDIVirtualMachine: Execution step limit reached." << std::endl;
         return false;
    }
    std::cout << "BDIVirtualMachine: Execution finished." << std::endl;
    return true; // Successfully halted (current_node_id_ became 0?)
 }
 bool BDIVirtualMachine::fetchDecodeExecuteCycle(BDIGraph& graph) {
    auto node_opt = graph.getNode(current_node_id_);
    if (!node_opt) {
        std::cerr << "VM Error: Current node ID " << current_node_id_ << " not found in graph." << std::endl;
        current_node_id_ = 0; // Halt on error
        return false;
    }
    BDINode& current_node = node_opt.value().get();
     std::cout << "VM Step: Executing Node " << current_node.id << " Op: " << static_cast<int>(current_node.operation) << std::endl;
    if (!executeNode(current_node)) {
         std::cerr << "VM Error: Execution failed for Node " << current_node.id << std::endl;
         current_node_id_ = 0; // Halt on error
         return false;
    }
    // If execution succeeded, determine the next node
    NodeID next_id = determineNextNode(current_node);
    current_node_id_ = next_id;
    return current_node_id_ != 0; // Continue if next node ID is valid
 }
 // !!! --- STUB IMPLEMENTATION --- !!!
 // This needs proper implementation with VM context (registers/stack/temps)
 // and interaction with MemoryManager
 bool BDIVirtualMachine::executeNode(BDINode& node) {
    switch (node.operation) {
        case core::graph::BDIOperationType::META_NOP:
             std::cout << "  Op: NOP" << std::endl;
// Do nothing
            break;
        case core::graph::BDIOperationType::META_START:
             std::cout << "  Op: START" << std::endl;
             // Initialize context if needed
             break;
        case core::graph::BDIOperationType::META_END:
             std::cout << "  Op: END" << std::endl;
             // Signal halt / return value?
             return true; // Special case? Or let determineNextNode handle halt?
        case core::graph::BDIOperationType::ARITH_ADD:
             std::cout << "  Op: ADD (Stub)" << std::endl;
             // STUB: Need to:
             // 1. Get input PortRefs from node.data_inputs
             // 2. Resolve PortRefs to actual values (from VM context/memory/previous outputs)
             // 3. Check types
             // 4. Perform addition
             // 5. Store result (in VM context/temp storage) for consumers
             break;
        case core::graph::BDIOperationType::MEM_LOAD:
            std::cout << "  Op: LOAD (Stub)" << std::endl;
            // STUB: Need to:
            // 1. Get address (from input PortRef or payload)
            // 2. Interact with MemoryManager to read data
            // 3. Store loaded value (in VM context)
            break;
        case core::graph::BDIOperationType::MEM_STORE:
             std::cout << "  Op: STORE (Stub)" << std::endl;
             // STUB: Need to:
             // 1. Get address (from input PortRef or payload)
             // 2. Get data to store (from input PortRef)
             // 3. Interact with MemoryManager to write data
             break;
        case core::graph::BDIOperationType::CTRL_JUMP:
             std::cout << "  Op: JUMP (Stub)" << std::endl;
             // Action is handled by determineNextNode
             break;
        case core::graph::BDIOperationType::CTRL_BRANCH_COND:
             std::cout << "  Op: BRANCH (Stub)" << std::endl;
             // STUB: Need to:
             // 1. Get condition value (from input PortRef)
             // 2. Action is handled by determineNextNode based on condition
             break;
        default:
            std::cerr << "  Op: UNIMPLEMENTED (" << static_cast<int>(node.operation) << ")" << std::endl;
            return false; // Unimplemented operation
    }
    return true; // Assume success for stubs
 }
 // !!! --- STUB IMPLEMENTATION --- !!!
 // Needs proper handling of branches, calls, returns, end nodes
 NodeID BDIVirtualMachine::determineNextNode(BDINode& node) {
    switch (node.operation) {
        case core::graph::BDIOperationType::CTRL_JUMP:
            if (!node.control_outputs.empty()) {
                 std::cout << "  Jump -> Node " << node.control_outputs[0] << std::endl;
                return node.control_outputs[0]; // Assume first output is jump target
            }
            break;
        case core::graph::BDIOperationType::CTRL_BRANCH_COND:
            // STUB: Assume condition is true for now
             std::cout << "  Branch (assuming true) -> Node " << (node.control_outputs.empty() ? 0 : node.control_outputs[0]) << std::endl;
            if (!node.control_outputs.empty()) {
// Convention: [0] = true target, [1] = false target
                // Need to get condition value from VM context!
                bool condition = true; // STUBBED
                if (condition && node.control_outputs.size() > 0) {
                    return node.control_outputs[0];
                } else if (!condition && node.control_outputs.size() > 1) {
                    return node.control_outputs[1];
                }
            }
            break; // Fall through if branch target missing
         case core::graph::BDIOperationType::META_END:
              std::cout << "  End Node -> Halting (0)" << std::endl;
              return 0; // Signal Halt
        // Default: Assume sequential execution if possible
        default:
            if (!node.control_outputs.empty()) {
                 // Assuming the first control output is the sequential path if multiple exist
                 std::cout << "  Sequential -> Node " << node.control_outputs[0] << std::endl;
                 return node.control_outputs[0];
            }
            break;
    }
    std::cerr << "VM Warning: Could not determine next node from Node " << node.id << ". Halting." << std::endl;
    return 0; // Halt if next node cannot be determined
 }
 } // namespace bdi::runtime
