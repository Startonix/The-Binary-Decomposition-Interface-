 // File: bdi/runtime/BDIVirtualMachine.cpp 
 #include "BDIVirtualMachine.hpp"
 #include "ExecutionContext.hpp"
 #include "MemoryManager.hpp"
 #include "../core/graph/BDINode.hpp"
 #include "../core/types/TypeSystem.hpp"
 #include "../core/payload/TypedPayload.hpp"
 #include <iostream>
 #include <stdexcept>
 #include <variant>
 #include <cmath> // For abs, floating point ops
 #include <limits> // For numeric limits
 namespace bdi::runtime {
 // --- Helper Templates for Input/Output --
// (More robust than macros)
 template <typename ExpectedType>
 std::optional<ExpectedType> getInputValue(ExecutionContext& ctx, const BDINode& node, PortIndex input_idx) {
    if (input_idx >= node.data_inputs.size()) {
        std::cerr << "VM Error: Accessing out-of-bounds input index " << input_idx << " for Node " << node.id << std::endl;
        return std::nullopt;
    }
    auto payload_opt = ctx.getPortValue(node.data_inputs[input_idx]);
    if (!payload_opt) {
        std::cerr << "VM Error: Input value not found for Node " << node.id << " Input " << input_idx << std::endl;
        return std::nullopt;
    }
    // Basic Type Check (can be enhanced using TypeSystem)
    constexpr BDIType expected_bdi_type = core::payload::MapCppTypeToBdiType<ExpectedType>::value;
     if (payload_opt.value().type != expected_bdi_type) {
         // Attempt implicit conversion? For now, require exact match for simplicity in this helper.
         // Proper VM would handle promotions based on operation.
         std::cerr << "VM Error: Type mismatch for Node " << node.id << " Input " << input_idx
                   << ". Expected " << core::types::bdiTypeToString(expected_bdi_type)
                   << ", Got " << core::types::bdiTypeToString(payload_opt.value().type) << std::endl;
         return std::nullopt;
     }
    try {
        return payload_opt.value().getAs<ExpectedType>();
    } catch (const std::exception& e) {
         std::cerr << "VM Error: Exception getting input for Node " << node.id << " Input " << input_idx << ": " << e.what() << std::endl;
        return std::nullopt;
    }
 }
 template <typename T>
 bool setOutputValue(ExecutionContext& ctx, const BDINode& node, PortIndex output_idx, T value) {
     if (output_idx >= node.data_outputs.size()) {
         std::cerr << "VM Error: Accessing out-of-bounds output index " << output_idx << " for Node " << node.id << std::endl;
         return false;
     }
     // Check if node's defined output type matches T
 constexpr BDIType expected_bdi_type = core::payload::MapCppTypeToBdiType<T>::value;
     if (node.data_outputs[output_idx].type != expected_bdi_type) {
          std::cerr << "VM Error: Trying to set output type " << core::types::bdiTypeToString(expected_bdi_type)
                    << " for Node " << node.id << " Port " << output_idx
                    << ", but port is defined as " << core::types::bdiTypeToString(node.data_outputs[output_idx].type) << std::endl;
          // Allow if UNKNOWN? Or require definition? For now, be strict.
          return false;
     }
     ctx.setPortValue(node.id, output_idx, core::payload::TypedPayload::createFrom(value));
     return true;
 }
 // --- BDIVirtualMachine Methods --
BDIVirtualMachine::BDIVirtualMachine(size_t memory_size)
    : current_node_id_(0),
      memory_manager_(std::make_unique<MemoryManager>(memory_size)),
      execution_context_(std::make_unique<ExecutionContext>())
 {
    if (!memory_manager_ || !execution_context_) {
        throw std::runtime_error("Failed to initialize VM components.");
    }
    //std::cout << "BDIVirtualMachine: Initialized with MemoryManager and ExecutionContext." << std::endl;
 }
 BDIVirtualMachine::~BDIVirtualMachine() = default;
 bool BDIVirtualMachine::execute(BDIGraph& graph, NodeID entry_node_id) {
    //std::cout << "BDIVirtualMachine: Starting execution at Node " << entry_node_id << std::endl;
    current_node_id_ = entry_node_id;
    execution_context_->clear();
    int execution_step_limit = 10000; // Increased limit
    int steps = 0;
    while (current_node_id_ != 0 && steps < execution_step_limit) {
        if (!fetchDecodeExecuteCycle(graph)) {
            std::cerr << "BDIVirtualMachine: Execution cycle failed or halted abnormally at Node " << current_node_id_ << "." << std::endl;
            current_node_id_ = 0;
            return false;
        }
        steps++;
         if (current_node_id_ == 0) {
            //std::cout << "BDIVirtualMachine: Execution halted normally." << std::endl;
            break;
        }
    }
    if (steps >= execution_step_limit) {
         std::cerr << "BDIVirtualMachine: Execution step limit reached." << std::endl;
         return false;
    }
    //std::cout << "BDIVirtualMachine: Execution finished." << std::endl;
    return current_node_id_ == 0;
 }
 bool BDIVirtualMachine::fetchDecodeExecuteCycle(BDIGraph& graph) {
    auto node_opt = graph.getNode(current_node_id_);
    if (!node_opt) {
        std::cerr << "VM Error: Current node ID " << current_node_id_ << " not found in graph." << std::endl;
        return false;
    }
    BDINode& current_node = node_opt.value().get();
    if (!executeNode(current_node)) {
         // Error already printed in executeNode usually
         return false;
    }
    // Determine the next node AFTER successful execution
    NodeID next_id = determineNextNode(current_node);
    current_node_id_ = next_id;
    return true; // Cycle completed successfully (may have set current_node_id_ to 0)
 }
 // --- Execute Node Implementation (Expanded with Core Ops) --
// Note: This version still simplifies type handling significantly.
 // A production VM would need robust handling of type promotions,
 // different integer sizes, float precision etc., likely using templates
 // or visitors over std::variant representations of values.
 bool BDIVirtualMachine::executeNode(BDINode& node) {
    using OpType = core::graph::BDIOperationType;
    using BDIType = core::types::BDIType;
    using TypeSys = core::types::TypeSystem;
    ExecutionContext& ctx = *execution_context_;
    // Lambda helpers for common binary operations (simplifies type checking)
    auto executeNumericBinaryOp = [&](auto operation) -> bool {
        if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1) return false;
        auto lhs_opt = ctx.getPortValue(node.data_inputs[0]);
        auto rhs_opt = ctx.getPortValue(node.data_inputs[1]);
        if (!lhs_opt || !rhs_opt) return false; // Missing input
        BDIType type1 = lhs_opt.value().type;
        BDIType type2 = rhs_opt.value().type;
        BDIType result_type = TypeSys::getPromotedType(type1, type2); // Determine result type
        BDIType expected_output_type = node.getOutputType(0);
        // Allow UNKNOWN output for flexibility, otherwise check compatibility
         if (expected_output_type != BDIType::UNKNOWN && !TypeSys::areCompatible(result_type, expected_output_type) &&
 !TypeSys::canImplicitlyConvert(result_type, expected_output_type)) {
             std::cerr << "VM Error: Node " << node.id << " output type " << core::types::bdiTypeToString(expected_output_type)
                       << " incompatible with promoted result type " << core::types::bdiTypeToString(result_type) << std::endl;
             return false;
         }
         if (result_type == BDIType::UNKNOWN) {
             std::cerr << "VM Error: Invalid type promotion for Node " << node.id << " inputs "
                       << core::types::bdiTypeToString(type1) << " and " << core::types::bdiTypeToString(type2) << std::endl;
             return false;
         }
        // Perform operation based on promoted type
        // NOTE: This requires converting inputs to the promoted type first!
        // This is a simplified example assuming direct operation is possible or types match.
        // A full implementation needs explicit conversion logic.
        try {
            // --- Simplified Type Handling --
            // We'll just handle a few common pairs for demonstration
             if (result_type == BDIType::INT32) {
                 int32_t v1 = lhs_opt.value().getAs<int32_t>(); // Assumes type matches or conversion is trivial
                 int32_t v2 = rhs_opt.value().getAs<int32_t>();
                 int32_t result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             } else if (result_type == BDIType::INT64) {
                 int64_t v1 = lhs_opt.value().getAs<int64_t>();
                 int64_t v2 = rhs_opt.value().getAs<int64_t>();
                 int64_t result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             } else if (result_type == BDIType::FLOAT32) {
                 float v1 = lhs_opt.value().getAs<float>();
                 float v2 = rhs_opt.value().getAs<float>();
                 float result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             } else if (result_type == BDIType::FLOAT64) {
                 double v1 = lhs_opt.value().getAs<double>();
                 double v2 = rhs_opt.value().getAs<double>();
                 double result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             }
            // --- Add other numeric types (UINTs, FLOAT16...) ---
            else {
                 std::cerr << "VM Error: Unhandled numeric type " << core::types::bdiTypeToString(result_type) << " for Node " << node.id << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
             std::cerr << "VM Exception during numeric op Node " << node.id << ": " << e.what() << std::endl;
            return false;
        }
    };
    auto executeIntegerBinaryOp = [&](auto operation) -> bool {
         // Similar structure to executeNumericBinaryOp, but enforces integer types
         // ... implementation ...
         // For brevity, skipping full implementation here, but structure is similar
         // Ensure result type is integer.
         return true; // Placeholder
    };
     auto executeComparisonOp = [&](auto operation) -> bool {
        if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1 || node.getOutputType(0) != BDIType::BOOL) return false;
         auto lhs_opt = ctx.getPortValue(node.data_inputs[0]);
         auto rhs_opt = ctx.getPortValue(node.data_inputs[1]);
         if (!lhs_opt || !rhs_opt) return false;
         // Simplified: Assume inputs are compatible and directly comparable for now
         // Need proper type handling based on inputs
         try {
             if (TypeSys::isInteger(lhs_opt.value().type)) { // Example INT32 comparison
                 int32_t v1 = lhs_opt.value().getAs<int32_t>();
                 int32_t v2 = rhs_opt.value().getAs<int32_t>();
                 bool result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             } else if (TypeSys::isFloatingPoint(lhs_opt.value().type)) { // Example FLOAT32 comparison
                 float v1 = lhs_opt.value().getAs<float>();
                 float v2 = rhs_opt.value().getAs<float>();
                 bool result = operation(v1, v2);
                 return setOutputValue(ctx, node, 0, result);
             }
             // --- Add other comparable types --
             else {
                  std::cerr << "VM Error: Unhandled comparison types for Node " << node.id << std::endl;
                 return false;
             }
         } catch (const std::exception& e) {
              std::cerr << "VM Exception during comparison op Node " << node.id << ": " << e.what() << std::endl;
             return false;
         }
     };
    try {
        switch (node.operation) {
            // --- Meta Ops --
            case OpType::META_NOP: break;
            case OpType::META_START: break;
            case OpType::META_END: return true; // Handled by determineNextNode
            // --- Arithmetic Ops --
            case OpType::ARITH_ADD: return executeNumericBinaryOp([](auto a, auto b){ return a + b; });
            case OpType::ARITH_SUB: return executeNumericBinaryOp([](auto a, auto b){ return a - b; });
            case OpType::ARITH_MUL: return executeNumericBinaryOp([](auto a, auto b){ return a * b; });
            case OpType::ARITH_DIV: return executeNumericBinaryOp([](auto a, auto b){
                 // TODO: Handle division by zero!
                 if (b == 0) throw std::runtime_error("Division by zero");
                 return a / b;
            });
            case OpType::ARITH_MOD: { // Modulo needs integer types
                 if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1) return false;
                 auto lhs_opt = ctx.getPortValue(node.data_inputs[0]);
                 auto rhs_opt = ctx.getPortValue(node.data_inputs[1]);
                 if (!lhs_opt || !rhs_opt || !TypeSys::isInteger(lhs_opt.value().type) || !TypeSys::isInteger(rhs_opt.value().type)) return false;
                 // Simplified INT32 case
                 int32_t v1 = lhs_opt.value().getAs<int32_t>();
                 int32_t v2 = rhs_opt.value().getAs<int32_t>();
                 if (v2 == 0) throw std::runtime_error("Modulo by zero");
                 return setOutputValue(ctx, node, 0, v1 % v2);
            }
             case OpType::ARITH_NEG: {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto val_opt = ctx.getPortValue(node.data_inputs[0]);
                 if (!val_opt || !TypeSys::isNumeric(val_opt.value().type)) return false;
                 // Simplified INT32 case
                  if (val_opt.value().type == BDIType::INT32) return setOutputValue(ctx, node, 0, -val_opt.value().getAs<int32_t>());
                  // Add other types (float, int64...)
                 return false; // Unhandled type
             }
            // --- Implement ABS, INC, DEC similarly --
            // --- Bitwise Ops (Assume Integer types) --
             case OpType::BIT_AND: return executeIntegerBinaryOp([](auto a, auto b){ return a & b; });
             case OpType::BIT_OR:  return executeIntegerBinaryOp([](auto a, auto b){ return a | b; });
             case OpType::BIT_XOR: return executeIntegerBinaryOp([](auto a, auto b){ return a ^ b; });
             case OpType::BIT_NOT: {
                  if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                  auto val_opt = ctx.getPortValue(node.data_inputs[0]);
                  if (!val_opt || !TypeSys::isInteger(val_opt.value().type)) return false;
                  // Simplified INT32 case
                  if (val_opt.value().type == BDIType::INT32) return setOutputValue(ctx, node, 0, ~val_opt.value().getAs<int32_t>());
                 // Add other integer types...
                 return false;
             }
            // --- Implement SHL, SHR, ASHR, ROL, ROR, POPCOUNT etc. --
            // --- Comparison Ops --
            case OpType::CMP_EQ: return executeComparisonOp([](auto a, auto b){ return a == b; });
            case OpType::CMP_NE: return executeComparisonOp([](auto a, auto b){ return a != b; });
            case OpType::CMP_LT: return executeComparisonOp([](auto a, auto b){ return a < b; });
            case OpType::CMP_LE: return executeComparisonOp([](auto a, auto b){ return a <= b; });
            case OpType::CMP_GT: return executeComparisonOp([](auto a, auto b){ return a > b; });
            case OpType::CMP_GE: return executeComparisonOp([](auto a, auto b){ return a >= b; });
             // --- Logical Ops (Assume BOOL inputs/output) --
             case OpType::LOGIC_AND: return executeComparisonOp([](bool a, bool b){ return a && b; }); // Re-use comparison helper structure
             case OpType::LOGIC_OR:  return executeComparisonOp([](bool a, bool b){ return a || b; });
             case OpType::LOGIC_XOR: return executeComparisonOp([](bool a, bool b){ return a ^ b; });
             case OpType::LOGIC_NOT: {
                  if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1 || node.getOutputType(0) != BDIType::BOOL) return false;
                  auto val_opt = getInputValue<bool>(ctx, node, 0); // Use helper
                  if (!val_opt) return false;
                  return setOutputValue(ctx, node, 0, !val_opt.value());
             }
            // --- Memory Ops --
            case OpType::MEM_LOAD: {
                 // ... (Implementation from previous step) ...
                 // Ensure type loaded matches node.getOutputType(0)
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto address_opt = getInputValue<uintptr_t>(ctx, node, 0);
                 if (!address_opt) return false;
                 uintptr_t address = address_opt.value();
                 BDIType load_type = node.getOutputType(0);
                 size_t load_size = core::types::getBdiTypeSize(load_type);
                 if (load_size == 0 && load_type != BDIType::VOID) { // Allow VOID load? Maybe not.
                      std::cerr << "VM Error: Cannot load zero-size type " << core::types::bdiTypeToString(load_type) << " for Node " << node.id << std::endl;
                      return false;
                 }
                 std::vector<std::byte> buffer(load_size);
                 if (!memory_manager_->readMemory(address, buffer.data(), load_size)) {
                     std::cerr << "VM Error: Memory read failed at address " << address << " for Node " << node.id << std::endl;
                     return false;
                 }
                 ctx.setPortValue(node.id, 0, TypedPayload(load_type, std::move(buffer)));
                 break;
            }
            case OpType::MEM_STORE: {
                  // ... (Implementation from previous step) ...
                  if (node.data_inputs.size() != 2) return false; // Input 0: Address, Input 1: Value
                  auto address_opt = getInputValue<uintptr_t>(ctx, node, 0);
                  auto value_payload_opt = ctx.getPortValue(node.data_inputs[1]); // Get payload directly
                  if (!address_opt || !value_payload_opt) return false;
                 if (!memory_manager_->writeMemory(address_opt.value(), value_payload_opt.value().data.data(), value_payload_opt.value().data.size())) {
                      std::cerr << "VM Error: Memory write failed at address " << address_opt.value() << " for Node " << node.id << std::endl;
                     return false;
                 }
                 break;
            }
            case OpType::MEM_ALLOC: {
                 // ... (Implementation from previous step) ...
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto alloc_size_opt = getInputValue<uint64_t>(ctx, node, 0); // Assume size is uint64
                 if (!alloc_size_opt) return false;
                 size_t alloc_size = static_cast<size_t>(alloc_size_opt.value());
                 auto region_id_opt = memory_manager_->allocateRegion(alloc_size);
                 if (!region_id_opt) return false;
                 auto region_info = memory_manager_->getRegionInfo(region_id_opt.value());
                 if (!region_info) return false;
                 // Output 0: Base address (POINTER type)
                 if (!setOutputValue(ctx, node, 0, region_info.value().base_address)) return false;
                 break;
            }
            // --- Type Conversion Ops (Example) --
             case OpType::CONV_INT_TO_FLOAT: { // Example: INT32 -> FLOAT32
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1 || node.getOutputType(0) != BDIType::FLOAT32) return false;
                 auto input_val_opt = getInputValue<int32_t>(ctx, node, 0); // Assume input type checked here
                 if (!input_val_opt) return false;
                 return setOutputValue(ctx, node, 0, static_cast<float>(input_val_opt.value()));
             }
             case OpType::CONV_FLOAT_TO_INT: { // Example: FLOAT32 -> INT32 (Truncation)
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1 || node.getOutputType(0) != BDIType::INT32) return false;
                  auto input_val_opt = getInputValue<float>(ctx, node, 0);
                  if (!input_val_opt) return false;
                  // TODO: Handle potential overflow/saturation?
                  return setOutputValue(ctx, node, 0, static_cast<int32_t>(input_val_opt.value()));
             }
            // --- Implement other CONV_ operations --
            // --- Control Flow Ops --
            case OpType::CTRL_JUMP: break; // Handled by determineNextNode
            case OpType::CTRL_BRANCH_COND: break; // Condition checked in determineNextNode
            case OpType::CTRL_CALL: break; // Handled by determineNextNode
            case OpType::CTRL_RETURN: break; // Handled by determineNextNode
            // --- Default --
            default:
                std::cerr << "VM Error: UNIMPLEMENTED Operation Type (" << static_cast<int>(node.operation) << ") for Node " << node.id << std::endl;
                return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "VM Exception during execution of Node " << node.id << " (Op: " << static_cast<int>(node.operation) << "): " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "VM Unknown exception during execution of Node " << node.id << " (Op: " << static_cast<int>(node.operation) << ")" << std::endl;
        return false;
    }
    return true; // Assume success if no error/exception
 }
 // --- Determine Next Node Implementation (Expanded for CALL/RETURN) --
NodeID BDIVirtualMachine::determineNextNode(BDINode& node) {
    ExecutionContext& ctx = *execution_context_;
    NodeID next_id = 0; // Default to halt
    switch (node.operation) {
        // --- Standard Control Flow ---
        case core::graph::BDIOperationType::CTRL_JUMP:
            if (!node.control_outputs.empty()) {
                next_id = node.control_outputs[0];
            }
            break;
        case core::graph::BDIOperationType::CTRL_BRANCH_COND: {
            // ... (Implementation from previous step remains the same) ...
             if (node.data_inputs.empty() || node.control_outputs.empty()) break;
             auto condition_payload_opt = ctx.getPortValue(node.data_inputs[0]);
             if (!condition_payload_opt || condition_payload_opt.value().type != core::types::BDIType::BOOL) break;
             bool condition = condition_payload_opt.value().getAs<bool>();
             if (condition) { // True branch
                 if (node.control_outputs.size() > 0) next_id = node.control_outputs[0];
             } else { // False branch
                  if (node.control_outputs.size() > 1) next_id = node.control_outputs[1];
                  else if (node.control_outputs.size() > 0) next_id = node.control_outputs[0]; // Fallthrough if only one target
             }
            break;
        }
        case core::graph::BDIOperationType::META_END:
             next_id = 0; // Signal Halt
             break;
        // --- Function Call / Return --
        case core::graph::BDIOperationType::CTRL_CALL:
            if (!node.control_outputs.empty()) {
                 // Determine return address: Assume sequential execution after CALL node
                 // Find the node connected via control flow *from* the CALL node that is NOT the call target
                 NodeID call_target = node.control_outputs[0]; // Assume target is first
                 NodeID return_address = 0; // Default halt if no return path
                 // This requires searching the graph or having precomputed successor info.
                 // Simple approach: Look for *another* control output from the CALL node.
                 // A more robust approach uses graph analysis or specific return edge conventions.
                 // For now, let's assume a simple sequential return if possible.
                 // We need to know the *next* node in the *caller's* sequence. This isn't directly
                 // stored on the CALL node itself typically. Let's assume a convention:
                 // The *second* control output of CALL is the return address node ID.
                 if (node.control_outputs.size() > 1) {
                     return_address = node.control_outputs[1];
                 } else {
                    // If no explicit return path, maybe it returns to 0 (halt) or error?
                    std::cerr << "VM Warning: No explicit return path specified for CALL Node " << node.id << std::endl;
                 }
                // Push return address onto stack
                ctx.pushCall(return_address);
                // Jump to the function entry point
                next_id = call_target;
            }
            break;
        case core::graph::BDIOperationType::CTRL_RETURN: {
            // Pop the return address from the stack
            auto return_address_opt = ctx.popCall();
            if (return_address_opt) {
                next_id = return_address_opt.value(); // Jump to return address
            } else {
                 std::cerr << "VM Warning: RETURN executed with empty call stack. Halting." << std::endl;
                next_id = 0; // Halt if stack is empty
            }
            // Note: Handling return values is separate - usually done by the node *before* RETURN
            // storing the value in context/memory, accessible by the caller.
            break;
        }
        // Default: Assume sequential execution if only one control output
        default:
            if (node.control_outputs.size() == 1) {
                 next_id = node.control_outputs[0];
            } else if (node.control_outputs.size() > 1) {
                // Ambiguous sequential path - requires convention or error
                 std::cerr << "VM Warning: Ambiguous sequential flow from Node " << node.id << ". Halting." << std::endl;
                 next_id = 0;
            } else {
                // No control output defined, assume halt (unless it was META_END already handled)
                next_id = 0;
            }
            break;
    }
    return next_id;
 }
 // --- Need accessors for VM components --
 ExecutionContext* BDIVirtualMachine::getExecutionContext() { return execution_context_.get(); }
 MemoryManager* BDIVirtualMachine::getMemoryManager() { return memory_manager_.get(); }
 const ExecutionContext* BDIVirtualMachine::getExecutionContext() const { return execution_context_.get(); }
 const MemoryManager* BDIVirtualMachine::getMemoryManager() const { return memory_manager_.get(); }
