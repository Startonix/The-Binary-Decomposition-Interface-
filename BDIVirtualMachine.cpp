// File: bdi/runtime/BDIVirtualMachine.cpp 
 #include "BDIVirtualMachine.hpp"
 #include "ExecutionContext.hpp"
 #include "MemoryManager.hpp"
 #include "../core/graph/BDINode.hpp"
 #include "../core/types/TypeSystem.hpp"
 #include "../core/payload/TypedPayload.hpp"
 #include "BDIValueVariant.hpp" // Include the variant
 #include <iostream>
 #include <stdexcept>
 #include <variant>
 #include <cmath>
 #include <limits>
 #include <functional> // For std::function
 namespace bdi::runtime {
 // --- Type Conversion Helper --
 // Converts a value in a variant to the target C++ type, handling promotions/truncations.
 // Returns std::nullopt on failure.
 template <typename TargetType>
 std::optional<TargetType> convertVariantTo(const BDIValueVariant& value_var) {
    using Type = core::types::BDIType;
    constexpr Type target_bdi_type = core::payload::MapCppTypeToBdiType<TargetType>::value;
    TargetType result;
    bool success = false;
    std::visit([&](auto&& arg) {
        using SourceCppType = std::decay_t<decltype(arg)>;
        constexpr Type source_bdi_type = core::payload::MapCppTypeToBdiType<SourceCppType>::value;
        if constexpr (std::is_same_v<SourceCppType, TargetType>) {
            result = arg; // Direct match
            success = true;
        } else if constexpr (std::is_convertible_v<SourceCppType, TargetType>) {
             // Check if BDI allows this implicit conversion (more strict than C++)
             if (core::types::TypeSystem::canImplicitlyConvert(source_bdi_type, target_bdi_type)) {
                // Perform standard C++ conversion
                // Warning: Potential precision loss or overflow not explicitly checked here!
                result = static_cast<TargetType>(arg);
                success = true;
             }
             // Else: Conversion not allowed by BDI rules
        }
        // Else: Types are not convertible
    }, value_var);
    if (success) {
        return result;
    } else {
        std::cerr << "VM Error: Cannot convert variant holding " << core::types::bdiTypeToString(getBDIType(value_var))
                  << " to target type " << core::types::bdiTypeToString(target_bdi_type) << std::endl;
        return std::nullopt;
    }
 }
 // --- Helper to get typed input value from context using variant --
template <typename ExpectedType>
 std::optional<ExpectedType> getInputValueTyped(ExecutionContext& ctx, const BDINode& node, PortIndex input_idx) {
     if (input_idx >= node.data_inputs.size()) return std::nullopt; // Bounds check
     auto value_var_opt = ctx.getPortValue(node.data_inputs[input_idx]);
     if (!value_var_opt) return std::nullopt; // Value not found
     return convertVariantTo<ExpectedType>(value_var_opt.value());
 }
 // --- Helper to set output value variant --
bool setOutputValueVariant(ExecutionContext& ctx, const BDINode& node, PortIndex output_idx, BDIValueVariant value_var) {
     if (output_idx >= node.data_outputs.size()) return false;
     // Optional: Check if variant type matches node's declared output type
     BDIType output_def_type = node.data_outputs[output_idx].type;
     BDIType value_actual_type = getBDIType(value_var);
     if (output_def_type != BDIType::UNKNOWN && !core::types::TypeSystem::areCompatible(output_def_type, value_actual_type) &&
 !core::types::TypeSystem::canImplicitlyConvert(value_actual_type, output_def_type)) {
         std::cerr << "VM Error: Output type mismatch for Node " << node.id << " Port " << output_idx
                   << ". Declared: " << core::types::bdiTypeToString(output_def_type)
                   << ", Actual: " << core::types::bdiTypeToString(value_actual_type) << std::endl;
         // If implicit conversion is allowed, should we convert before setting? For now, require match or explicit conversion node.
         return false;
     }
     ctx.setPortValue(node.id, output_idx, std::move(value_var));
     return true;
 }
 // --- BDIVirtualMachine Methods --
BDIVirtualMachine::BDIVirtualMachine(MetadataStore& meta_store, size_t memory_size) // Added MetadataStore
    : current_node_id_(0),
      metadata_store_(&meta_store), // Store reference
      memory_manager_(std::make_unique<MemoryManager>(memory_size)),
      execution_context_(std::make_unique<ExecutionContext>())
 {
    if (!memory_manager_ || !execution_context_ || !metadata_store_) {
        throw std::runtime_error("Failed to initialize VM components.");
    }
 }
 BDIVirtualMachine::~BDIVirtualMachine() = default;
 bool BDIVirtualMachine::execute(BDIGraph& graph, NodeID entry_node_id) { /* ... (no change needed) ... */ }
 bool BDIVirtualMachine::fetchDecodeExecuteCycle(BDIGraph& graph) { /* ... (no change needed) ... */ }
 // --- Execute Node Implementation (Refactored for Variant & Core Ops) --
 bool BDIVirtualMachine::executeNode(BDINode& node) {
    using OpType = core::graph::BDIOperationType;
    using BDIType = core::types::BDIType;
    using TypeSys = core::types::TypeSystem;
    ExecutionContext& ctx = *execution_context_;
  case OpType::META_ASSERT: {
                 if (node.data_inputs.size() != 1) return false;
                 auto condition_opt = getInputValueTyped<bool>(ctx, node, 0);
                 if (!condition_opt) {
                      std::cerr << "VM Error: ASSERT Node " << node.id << " condition input missing or invalid." << std::endl;
                     return false;
                 }
                 if (!condition_opt.value()) {
                     // Assertion failed!
                     std::cerr << "VM ASSERTION FAILED: Node " << node.id << "." << std::endl;
                     // Retrieve associated semantic tag for more info?
                      const MetadataVariant* meta = metadata_store_->getMetadata(node.metadata_handle); // Need access to MetadataStore
                     if (meta && std::holds_alternative<SemanticTag>(*meta)) {
                          std::cerr << "  Description: " << std::get<SemanticTag>(*meta).description << std::endl;
                     }
                     return false; // Halt execution on failed assertion
                 }
                 // Else: Assertion passed, continue
                 break;
            }
            case OpType::META_VERIFY_PROOF: {
                 std::cout << "  Op: VERIFY_PROOF (Stub) for Node " << node.id << std::endl;
                 // STUB:
                 // 1. Get metadata handle: node.metadata_handle
                 // 2. Access MetadataStore: metadata_store_->getMetadata(handle)
                 // 3. Check if variant holds a ProofTag
                 // 4. If yes, get the hash/proof data
                 // 5. Call external ProofVerifier (to be implemented)
                 // 6. Return false if verification fails
                 break;
            }
    // --- Visitor Pattern for Binary Operations --
    // Define a visitor struct to handle operations based on promoted types
    struct BinaryOpVisitor {
        BDIValueVariant& result; // Store result directly in the variant
        const BDIValueVariant& rhs;
        // Define operator() for each type combination you want to handle
        template <typename T1> // T1 is type of LHS
        void operator()(const T1& lhs_val) {
            std::visit([&](auto&& rhs_val) { // Visit RHS
                using T2 = std::decay_t<decltype(rhs_val)>;
                // Determine promoted type (simplified example)
                 constexpr BDIType bdi_t1 = core::payload::MapCppTypeToBdiType<T1>::value;
                 constexpr BDIType bdi_t2 = core::payload::MapCppTypeToBdiType<T2>::value;
                 BDIType promoted_bdi = TypeSys::getPromotedType(bdi_t1, bdi_t2);
                 if (promoted_bdi == BDIType::UNKNOWN) {
                     result = std::monostate{}; // Indicate error
                     return;
                 }
                 // Perform operation based on promoted type
                 // Requires converting lhs_val and rhs_val to promoted type!
                 // --- THIS IS THE COMPLEX PART --
                 // Example for INT32 promotion (needs full conversion logic)
                 if (promoted_bdi == BDIType::INT32) {
                     auto lhs_p = convertVariantTo<int32_t>(BDIValueVariant{lhs_val}); // Convert LHS
                     auto rhs_p = convertVariantTo<int32_t>(rhs);                    // Convert RHS
                     if (lhs_p && rhs_p) {
                         // Apply the actual operation lambda passed to executeBinaryOpHelper
                         // result = operation_lambda(*lhs_p, *rhs_p); // Need to pass lambda in
                         result = (*lhs_p) + (*rhs_p); // Hardcoded ADD for now
                     } else { result = std::monostate{}; } // Conversion failed
                 }
                  // --- Add cases for INT64, FLOAT32, FLOAT64 etc. promotion --
                 else {
                    // std::cerr << "BinaryOpVisitor: Unhandled promotion type." << std::endl;
                    result = std::monostate{}; // Error: Unhandled promotion
                 }
            }, rhs); // End visit RHS
        } // End operator()
        // Handle monostate (error case)
         void operator()(const std::monostate&) { result = std::monostate{}; }
    };
    // Helper function to execute binary ops using the visitor
    // Takes operation name for error messages and the operation lambda
    // Returns bool success
    auto executeBinaryOpHelper =
        [&](const std::string& opName, auto operation_lambda) -> bool {
        if (node.data_inputs.size() != 2 || node.data_outputs.size() != 1) {
            std::cerr << "VM Error: Incorrect number of ports for " << opName << " Node " << node.id << std::endl;
            return false;
        }
        auto lhs_var_opt = ctx.getPortValue(node.data_inputs[0]);
        auto rhs_var_opt = ctx.getPortValue(node.data_inputs[1]);
        if (!lhs_var_opt || !rhs_var_opt) {
            std::cerr << "VM Error: Missing inputs for " << opName << " Node " << node.id << std::endl;
          return false;
        }
        BDIValueVariant result_var = std::monostate{}; // Start with error state
        // --- Use visitor to perform operation based on actual variant types --
        // NOTE: The visitor needs the specific operation lambda to work properly.
        // This structure is complex. Simpler approach below for now.
        // BinaryOpVisitor visitor{result_var, rhs_var_opt.value()};
        // std::visit(visitor, lhs_var_opt.value());
        // --- Simpler (less robust type handling) approach for now --
        BDIType type1 = getBDIType(lhs_var_opt.value());
        BDIType type2 = getBDIType(rhs_var_opt.value());
        BDIType promoted_type = TypeSys::getPromotedType(type1, type2);
        if(promoted_type == BDIType::UNKNOWN) {
            std::cerr << "VM Error: Cannot promote types for " << opName << " Node " << node.id << std::endl;
            return false;
        }
        try {
            if (promoted_type == BDIType::INT32) {
                auto v1 = convertVariantTo<int32_t>(lhs_var_opt.value());
                auto v2 = convertVariantTo<int32_t>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            } else if (promoted_type == BDIType::INT64) {
                auto v1 = convertVariantTo<int64_t>(lhs_var_opt.value());
                auto v2 = convertVariantTo<int64_t>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            } else if (promoted_type == BDIType::UINT32) {
                auto v1 = convertVariantTo<uint32_t>(lhs_var_opt.value());
                auto v2 = convertVariantTo<uint32_t>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            } else if (promoted_type == BDIType::UINT64) {
                auto v1 = convertVariantTo<uint64_t>(lhs_var_opt.value());
                auto v2 = convertVariantTo<uint64_t>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            } else if (promoted_type == BDIType::FLOAT32) {
                auto v1 = convertVariantTo<float>(lhs_var_opt.value());
                auto v2 = convertVariantTo<float>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            } else if (promoted_type == BDIType::FLOAT64) {
                auto v1 = convertVariantTo<double>(lhs_var_opt.value());
                auto v2 = convertVariantTo<double>(rhs_var_opt.value());
                if (v1 && v2) result_var = operation_lambda(*v1, *v2); else return false;
            }
            // --- Add BOOL logic if needed --
            else {
                 std::cerr << "VM Error: Unhandled promoted type " << core::types::bdiTypeToString(promoted_type) << " in " << opName << " Node " <<
 node.id << std::endl;
                 return false;
            }
        } catch (const std::exception& e) {
             std::cerr << "VM Exception during " << opName << " Node " << node.id << ": " << e.what() << std::endl;
            return false;
        }
        // Check for error state in result_var before setting output
        if (std::holds_alternative<std::monostate>(result_var)) {
             std::cerr << "VM Error: Operation failed for " << opName << " Node " << node.id << std::endl;
            return false;
        }
        return setOutputValueVariant(ctx, node, 0, result_var);
    };
     // --- Helper for Unary Operations --
     auto executeUnaryOpHelper =
         [&](const std::string& opName, auto operation_lambda) -> bool {
         if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
         auto val_opt = ctx.getPortValue(node.data_inputs[0]);
         if (!val_opt) return false;
         BDIValueVariant result_var = std::monostate{};
         BDIType input_type = getBDIType(val_opt.value());
          // Determine result type based on operation and input type
         // BDIType result_type = ...; // e.g., NEG keeps type, NOT is BOOL->BOOL?
         try {
              // Simplified again - apply lambda directly based on variant type
              std::visit([&](auto&& arg) {
                  using T = std::decay_t<decltype(arg)>;
                   if constexpr (!std::is_same_v<T, std::monostate>) {
                       // Check if operation is valid for this type T
                       // Apply operation
                        result_var = operation_lambda(arg);
                   }
              }, val_opt.value());
         } catch (const std::exception& e) {
             std::cerr << "VM Exception during " << opName << " Node " << node.id << ": " << e.what() << std::endl;
             return false;
         }
          if (std::holds_alternative<std::monostate>(result_var)) return false; // Operation failed or wasn't applicable
          return setOutputValueVariant(ctx, node, 0, result_var);
     };
    // --- Execute Node Switch Statement --
    try {
        switch (node.operation) {
            // --- Meta Ops --
            case OpType::META_NOP: break;
            case OpType::META_START: break;
            case OpType::META_END: return true;
            // --- Arithmetic Ops --
            case OpType::ARITH_ADD: return executeBinaryOpHelper("ADD", [](auto a, auto b){ return a + b; });
            case OpType::ARITH_SUB: return executeBinaryOpHelper("SUB", [](auto a, auto b){ return a - b; });
            case OpType::ARITH_MUL: return executeBinaryOpHelper("MUL", [](auto a, auto b){ return a * b; });
            case OpType::ARITH_DIV: return executeBinaryOpHelper("DIV", [](auto a, auto b){
                 if (b == static_cast<decltype(b)>(0)) throw std::runtime_error("Division by zero");
                 return a / b; // Integer division truncates, float is standard
            });
            case OpType::ARITH_MOD: // Requires integer logic separate from helper maybe
                 return executeBinaryOpHelper("MOD", [](auto a, auto b) -> decltype(a) {
                     if constexpr (std::is_integral_v<decltype(a)> && std::is_integral_v<decltype(b)>) {
                        if (b == 0) throw std::runtime_error("Modulo by zero");
                        return a % b;
                     } else {
                        throw std::runtime_error("MOD requires integer types"); // Should be caught by type promotion ideally
                        return {}; // Return default value for type
                     }
                 });
             case OpType::ARITH_NEG: return executeUnaryOpHelper("NEG", [](auto a){ return -a; });
             case OpType::ARITH_ABS: return executeUnaryOpHelper("ABS", [](auto a){ using std::abs; return abs(a); });
            // INC, DEC need careful handling for pointers vs numbers
            // --- Bitwise Ops (Assume Integer) --
            // Note: These lambdas might fail if non-integer types get through promotion (needs fixing)
             case OpType::BIT_AND: return executeBinaryOpHelper("AND", [](auto a, auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a & b;
 else throw std::runtime_error("BIT_AND requires integers"); });
             case OpType::BIT_OR:  return executeBinaryOpHelper("OR", [](auto a, auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a | b; else
 throw std::runtime_error("BIT_OR requires integers"); });
             case OpType::BIT_XOR: return executeBinaryOpHelper("XOR", [](auto a, auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a ^ b; else
 throw std::runtime_error("BIT_XOR requires integers"); });
             case OpType::BIT_NOT: return executeUnaryOpHelper("NOT", [](auto a){ if constexpr (std::is_integral_v<decltype(a)>) return ~a; else throw
 std::runtime_error("BIT_NOT requires integers"); });
             // --- SHL, SHR etc. require second operand type check --
            // --- Comparison Ops --
            case OpType::CMP_EQ: return executeBinaryOpHelper("CMP_EQ", [](auto a, auto b){ return bool(a == b); });
            case OpType::CMP_NE: return executeBinaryOpHelper("CMP_NE", [](auto a, auto b){ return bool(a != b); });
            case OpType::CMP_LT: return executeBinaryOpHelper("CMP_LT", [](auto a, auto b){ return bool(a < b); });
            case OpType::CMP_LE: return executeBinaryOpHelper("CMP_LE", [](auto a, auto b){ return bool(a <= b); });
            case OpType::CMP_GT: return executeBinaryOpHelper("CMP_GT", [](auto a, auto b){ return bool(a > b); });
            case OpType::CMP_GE: return executeBinaryOpHelper("CMP_GE", [](auto a, auto b){ return bool(a >= b); });
            // --- Logical Ops --
            case OpType::LOGIC_AND: return executeBinaryOpHelper("LOGIC_AND", [](bool a, bool b){ return a && b; });
            case OpType::LOGIC_OR:  return executeBinaryOpHelper("LOGIC_OR", [](bool a, bool b){ return a || b; });
            case OpType::LOGIC_XOR: return executeBinaryOpHelper("LOGIC_XOR", [](bool a, bool b){ return a ^ b; });
            case OpType::LOGIC_NOT: return executeUnaryOpHelper("LOGIC_NOT", [](bool a){ return !a; });
            // --- Memory Ops --
            case OpType::MEM_LOAD: {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto address_opt = getInputValueTyped<uintptr_t>(ctx, node, 0);
                 if (!address_opt) return false;
                 BDIType load_type = node.getOutputType(0);
                 size_t load_size = core::types::getBdiTypeSize(load_type);
                 if (load_size == 0 && load_type != BDIType::VOID) return false;
                 TypedPayload loaded_payload(load_type, BinaryData(load_size)); // Create payload structure
                 if (!memory_manager_->readMemory(address_opt.value(), loaded_payload.data.data(), load_size)) {
                     return false;
                 }
                 BDIValueVariant loaded_value = ExecutionContext::payloadToVariant(loaded_payload); // Convert binary to variant
                 if (std::holds_alternative<std::monostate>(loaded_value) && load_type != BDIType::VOID) return false; // Conversion failed
                 return setOutputValueVariant(ctx, node, 0, loaded_value); // Store variant in context
            }
            case OpType::MEM_STORE: {
                 if (node.data_inputs.size() != 2) return false;
                 auto address_opt = getInputValueTyped<uintptr_t>(ctx, node, 0);
                 auto value_var_opt = ctx.getPortValue(node.data_inputs[1]); // Get variant directly
                 if (!address_opt || !value_var_opt) return false;
                 TypedPayload payload_to_store = ExecutionContext::variantToPayload(value_var_opt.value()); // Convert variant to binary
                 if (payload_to_store.type == BDIType::UNKNOWN) return false; // Conversion failed
                 if (!memory_manager_->writeMemory(address_opt.value(), payload_to_store.data.data(), payload_to_store.data.size())) {
                     return false;
                 }
                 break; // Store has no output
            }
            case OpType::MEM_ALLOC: {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto alloc_size_opt = getInputValueTyped<uint64_t>(ctx, node, 0);
                 if (!alloc_size_opt) return false;
                 size_t alloc_size = static_cast<size_t>(alloc_size_opt.value());
                 auto region_id_opt = memory_manager_->allocateRegion(alloc_size);
                 if (!region_id_opt) return false;
                 auto region_info = memory_manager_->getRegionInfo(region_id_opt.value());
                 if (!region_info) return false;
                 // Output 0: Base address (POINTER/uintptr_t)
                 return setOutputValueVariant(ctx, node, 0, BDIValueVariant{region_info.value().base_address});
            }
            // --- Type Conversion Ops --
             case OpType::CONV_INT_TO_FLOAT: // Example: Any Integer -> FLOAT32
             case OpType::CONV_FLOAT_TO_INT: // Example: Any Float -> INT32 (Truncate)
             case OpType::CONV_EXTEND_SIGN: // Example: INT16 -> INT32
             case OpType::CONV_EXTEND_ZERO: // Example: UINT16 -> UINT32
             case OpType::CONV_TRUNC:       // Example: INT64 -> INT32
             case OpType::CONV_BITCAST:     // Reinterpret bits
             {
                 if (node.data_inputs.size() != 1 || node.data_outputs.size() != 1) return false;
                 auto input_val_opt = ctx.getPortValue(node.data_inputs[0]);
                 if (!input_val_opt) return false;
                 BDIType target_type = node.getOutputType(0);
                 BDIValueVariant result_var = std::monostate{};
                 // Use convertVariantTo to perform the conversion based on target type
                 if (target_type == BDIType::FLOAT32) {
                     auto res = convertVariantTo<float>(input_val_opt.value());
                     if (res) result_var = *res;
                 } else if (target_type == BDIType::INT32) {
                      auto res = convertVariantTo<int32_t>(input_val_opt.value());
                     if (res) result_var = *res;
                 }
                 // --- Add cases for all target types specified by CONV operations --
                 // Need specific logic for BITCAST, EXTEND, TRUNC etc.
                 else {
                      std::cerr << "VM Error: Unhandled CONV target type " << core::types::bdiTypeToString(target_type) << " for Node " << node.id <<
 std::endl;
                     return false;
                 }
                 if (std::holds_alternative<std::monostate>(result_var)) return false; // Conversion failed
                 return setOutputValueVariant(ctx, node, 0, result_var);
             }
            // --- Control Flow Ops --
            // Execution logic done, control flow handled by determineNextNode
            case OpType::CTRL_JUMP:
            case OpType::CTRL_BRANCH_COND:
            case OpType::CTRL_CALL:
            case OpType::CTRL_RETURN:
                break;
            // --- Default --
            default:
                std::cerr << "VM Error: UNIMPLEMENTED/UNKNOWN Operation Type (" << static_cast<int>(node.operation) << ") for Node " << node.id
 << std::endl;
                return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "VM Exception during execution of Node " << node.id << " (Op: " << static_cast<int>(node.operation) << "): " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "VM Unknown exception during execution of Node " << node.id << " (Op: " << static_cast<int>(node.operation) << ")" << std::endl;
        return false;
    }
    return true; // Assume success if no error/exception and not explicitly failed
 }
 // determineNextNode remains largely the same as before, handling control flow logic
 } // namespace bdi::runtime
