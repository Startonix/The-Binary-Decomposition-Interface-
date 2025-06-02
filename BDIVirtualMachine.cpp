// File: bdi/runtime/BDIVirtualMachine.cpp
 #include "BDIVirtualMachine.hpp"
 #include "ExecutionContext.hpp"
 #include "MemoryManager.hpp" // Include MemoryManager
 #include "../core/graph/BDINode.hpp"
 #include "../core/types/TypeSystem.hpp" // For type checks
 #include "../core/payload/TypedPayload.hpp" // For createFrom
 #include <iostream>
 #include <stdexcept>
 #include <variant> // For visitor pattern on TypedPayload (optional)
 namespace bdi::runtime {
 // Helper macro for getting typed value from context (replace with proper error handling)
 #define GET_INPUT_VALUE(ctx, port_ref, type, var_name) \
    auto var_name##_opt = ctx.getPortValue(port_ref); \
    if (!var_name##_opt) { \
        std::cerr << "VM Error: Input value not found for Node " << port_ref.node_id << " Port " << port_ref.port_index << std::endl; \
        return false; \
    } \
    /* TODO: Add type checking here against node.getExpectedInputType() */ \
    type var_name = var_name##_opt.value().getAs<type>(); \
 // Helper macro for setting output value
 #define SET_OUTPUT_VALUE(ctx, node_id, port_idx, value) \
    ctx.setPortValue(node_id, port_idx, core::payload::TypedPayload::createFrom(value));
 BDIVirtualMachine::BDIVirtualMachine(size_t memory_size)
    : current_node_id_(0),
      memory_manager_(std::make_unique<MemoryManager>(memory_size)), // Create Memory Manager
      execution_context_(std::make_unique<ExecutionContext>()) // Create Execution Context
 {
    if (!memory_manager_ || !execution_context_) {
        throw std::runtime_error("Failed to initialize VM components.");
    }
    std::cout << "BDIVirtualMachine: Initialized with MemoryManager and ExecutionContext." << std::endl;
 }
 // Destructor needed for unique_ptr to forward declared types
 BDIVirtualMachine::~BDIVirtualMachine() = default;
 bool BDIVirtualMachine::execute(BDIGraph& graph, NodeID entry_node_id) {
    std::cout << "BDIVirtualMachine: Starting execution at Node " << entry_node_id << std::endl;
    current_node_id_ = entry_node_id;
    execution_context_->clear(); // Reset context for new execution
    int execution_step_limit = 1000;
    int steps = 0;
    while (current_node_id_ != 0 && steps < execution_step_limit) {
        if (!fetchDecodeExecuteCycle(graph)) {
            std::cerr << "BDIVirtualMachine: Execution cycle failed or halted at Node " << current_node_id_ << "." << std::endl;
             current_node_id_ = 0; // Ensure halt on failure
            return false;
        }
        steps++;
         if (current_node_id_ == 0) { // Check if execution cycle signaled halt
            std::cout << "BDIVirtualMachine: Execution halted normally by META_END or control flow." << std::endl;
            break;
        }
    }
    if (steps >= execution_step_limit) {
         std::cerr << "BDIVirtualMachine: Execution step limit reached." << std::endl;
         return false;
    }
    std::cout << "BDIVirtualMachine: Execution finished." << std::endl;
    return current_node_id_ == 0; // Success if halted normally
 }
 bool BDIVirtualMachine::fetchDecodeExecuteCycle(BDIGraph& graph) {
    auto node_opt = graph.getNode(current_node_id_);
    if (!node_opt) {
        std::cerr << "VM Error: Current node ID " << current_node_id_ << " not found in graph." << std::endl;
        return false; // Halt on error
    }
t_node);
    current_node_id_ = next_id;
    return true; // Cycle completed (may have set current_node_id_ to 0 to signal halt)
 }
 // --- Execute Node Implementation (Expanded) --
bool BDIVirtualMachine::executeNode(BDINode& node) {
     using OpType = core::graph::BDIOperationType;
     using BDIType = core::types::BDIType;
    // Convenience reference
    ExecutionContext& ctx = *execution_context_;
    try { // Add basic exception handling around operations
        switch (node.operation) {
            // --- Meta Ops --
            case OpType::META_NOP: /* Do nothing */ break;
            case OpType::META_START: /* Context setup? */ break;
            case OpType::META_END: return true; // Handled by determineNextNode setting ID to 0
            // --- Arithmetic Ops (Example: INT32 Add) --
            case OpType::ARITH_ADD: {
                if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1) return false; // Basic validation
                // Assume INT32 for now - NEED PROPER TYPE HANDLING
                GET_INPUT_VALUE(ctx, node.data_inputs[0], int32_t, lhs);
                GET_INPUT_VALUE(ctx, node.data_inputs[1], int32_t, rhs);
                int32_t result = lhs + rhs;
                SET_OUTPUT_VALUE(ctx, node.id, 0, result);
                // std::cout << "  ADD " << lhs << " + " << rhs << " = " << result << std::endl;
                break;
            }
             // --- Add more arithmetic ops (SUB, MUL, DIV etc.) handling different types --
             // --- Need careful type checking based on node.getExpectedInputType / node.getOutputType --
            // --- Memory Ops --
            case OpType::MEM_LOAD: {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 // Input 0: Address (assume uintptr_t for now)
                 // Output 0: Loaded value (type from node.getOutputType(0))
                 GET_INPUT_VALUE(ctx, node.data_inputs[0], uintptr_t, address);
                 BDIType load_type = node.getOutputType(0);
                 size_t load_size = core::types::getBdiTypeSize(load_type);
                 if (load_size == 0) return false; // Cannot load zero-size type
                 std::vector<std::byte> buffer(load_size);
                 if (!memory_manager_->readMemory(address, buffer.data(), load_size)) {
                     std::cerr << "VM Error: Memory read failed at address " << address << std::endl;
                     return false;
                 }
                 ctx.setPortValue(node.id, 0, TypedPayload(load_type, std::move(buffer)));
                //  std::cout << "  LOAD from " << address << " (Size: " << load_size << ")" << std::endl;
                 break;
            }
             case OpType::MEM_STORE: {
                 if (node.data_inputs.size() != 2) return false; // Input 0: Address, Input 1: Value
                 GET_INPUT_VALUE(ctx, node.data_inputs[0], uintptr_t, address);
                 auto value_payload_opt = ctx.getPortValue(node.data_inputs[1]);
                 if (!value_payload_opt) return false; // Value not found
                 const auto& value_payload = value_payload_opt.value();
                 if (!memory_manager_->writeMemory(address, value_payload.data.data(), value_payload.data.size())) {
 std::cerr << "VM Error: Memory write failed at address " << address << std::endl;
                     return false;
                 }
                 // std::cout << "  STORE to " << address << " (Size: " << value_payload.data.size() << ")" << std::endl;
                 break;
            }
             case OpType::MEM_ALLOC: {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false; // Input 0: Size
                 GET_INPUT_VALUE(ctx, node.data_inputs[0], size_t, alloc_size); // Assuming size is passed as input
                 // TODO: Get read_only flag?
                 auto region_id_opt = memory_manager_->allocateRegion(alloc_size);
                 if (!region_id_opt) return false; // Allocation failed
                 auto region_info = memory_manager_->getRegionInfo(region_id_opt.value());
                 if (!region_info) return false; // Should not happen if alloc succeeded
                 // Output 0: Base address of allocated region
                 SET_OUTPUT_VALUE(ctx, node.id, 0, region_info.value().base_address);
                 // Output 1 (Optional convention): RegionID
                 // SET_OUTPUT_VALUE(ctx, node.id, 1, region_info.value().id);
                //  std::cout << "  ALLOC " << alloc_size << " bytes -> Addr " << region_info.value().base_address << std::endl;
                 break;
             }
            // --- Comparison Ops (Example: EQ_I32) --
             case OpType::CMP_EQ: {
                 if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1) return false;
                 // Assume INT32 for now - NEED PROPER TYPE HANDLING
                 GET_INPUT_VALUE(ctx, node.data_inputs[0], int32_t, lhs);
                 GET_INPUT_VALUE(ctx, node.data_inputs[1], int32_t, rhs);
                 bool result = (lhs == rhs);
                 SET_OUTPUT_VALUE(ctx, node.id, 0, result); // Output is BOOL
                //  std::cout << "  CMP_EQ " << lhs << " == " << rhs << " -> " << result << std::endl;
                 break;
             }
             // --- Add more comparison ops (NE, LT, GT, etc.) --
            // --- Control Flow Ops --
            // JUMP/BRANCH logic is primarily in determineNextNode, but BRANCH needs condition evaluation here.
            case OpType::CTRL_JUMP: /* Handled by determineNextNode */ break;
            case OpType::CTRL_BRANCH_COND: {
                if (node.data_inputs.size() != 1) return false; // Input 0: Condition (BOOL)
                // Condition value is retrieved in determineNextNode
                 break;
            }
            // --- Default --
            default:
                std::cerr << "VM Error: UNIMPLEMENTED Operation Type (" << static_cast<int>(node.operation) << ") for Node " << node.id << std::endl;
                return false; // Unimplemented operation
        }
    } catch (const std::exception& e) {
        std::cerr << "VM Exception during execution of Node " << node.id << ": " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "VM Unknown exception during execution of Node " << node.id << std::endl;
        return false;
    }
    return true; // Assume success if no error/exception
 }
 // --- Determine Next Node Implementation (Refined) --
NodeID BDIVirtualMachine::determineNextNode(BDINode& node) {
    ExecutionContext& ctx = *execution_context_;
    switch (node.operation) {
        case core::graph::BDIOperationType::CTRL_JUMP:
            if (!node.control_outputs.empty()) {
                return node.control_outputs[0]; // Unconditional jump target
            }
            break; // Fall through if target missing
        case core::graph::BDIOperationType::CTRL_BRANCH_COND: {
            if (node.data_inputs.empty() || node.control_outputs.size() < 1) { // Need at least condition input and one target
 std::cerr << "VM Error: Invalid BRANCH node " << node.id << " configuration." << std::endl;
                 break; // Fall through to halt
            }
            // Get condition value from context
            auto condition_payload_opt = ctx.getPortValue(node.data_inputs[0]);
            if (!condition_payload_opt || condition_payload_opt.value().type != core::types::BDIType::BOOL) {
                 std::cerr << "VM Error: BRANCH node " << node.id << " condition input is missing or not BOOL." << std::endl;
                 break; // Fall through to halt
            }
            bool condition = condition_payload_opt.value().getAs<bool>(); // Assumes BOOL is stored as bool C++ type
            NodeID target_node = 0; // Default to halt if target missing
            if (condition) { // True branch
                if (node.control_outputs.size() > 0) {
                    target_node = node.control_outputs[0]; // Convention: True target is first
                }
            } else { // False branch
                 if (node.control_outputs.size() > 1) {
                    target_node = node.control_outputs[1]; // Convention: False target is second
                 } else if (node.control_outputs.size() > 0) {
                    // If only one target, assume it's the fallthrough/merge point
                    target_node = node.control_outputs[0];
                 }
            }
            // std::cout << "  BRANCH Condition=" << condition << " -> Node " << target_node << std::endl;
            return target_node;
        }
         case core::graph::BDIOperationType::META_END:
              return 0; // Signal Halt
        // --- CALL / RETURN (Basic Sketch - NO STACK HANDLING YET) --
        // case core::graph::BDIOperationType::CTRL_CALL:
        //     if (!node.control_outputs.empty()) {
        //         // STUB: Need to push return address (e.g., next sequential node if exists)
        //         // NodeID return_addr = ...;
        //         // ctx.pushCall(return_addr);
        //         return node.control_outputs[0]; // Jump to function entry
        //     }
        //     break;
        // case core::graph::BDIOperationType::CTRL_RETURN: {
        //     // STUB: Need to pop return address
        //     // auto return_addr_opt = ctx.popCall();
        //     // return return_addr_opt.value_or(0); // Halt if stack empty
        //     return 0; // Simple halt for now
        // }
        // Default: Assume sequential execution if possible 
       default:
            if (!node.control_outputs.empty()) {
                 // Simple sequential flow: assumes first control output is the next instruction
                 return node.control_outputs[0];
            }
            break; // Fall through to halt if no control output specified
    }
    // If no specific control flow determined, assume halt or end of graph segment
    // std::cerr << "VM Warning: No control flow path determined from Node " << node.id << ". Halting." << std::endl;
    return 0; // Halt
 }
 } // namespace bdi::runtime
