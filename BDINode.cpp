 // File: bdi/core/graph/BDINode.cpp 
 #include "BDINode.hpp"
 #include "BDIGraph.hpp"
 #include "TypeSystem.hpp"
 namespace bdi::core::graph {
 // ... (validatePorts remains the same) ...
 // --- Implementation for getExpectedInputType --
// NOTE: This provides a *hint* or a common case. Runtime type checking in the VM
 // based on actual connected inputs is more robust, especially for polymorphic ops.
 BDIType BDINode::getExpectedInputType(PortIndex input_idx) const {
    using Op = BDIOperationType;
    using Type = types::BDIType;
    switch (operation) {
        // Arithmetic (assuming common cases, needs refinement for variants)
        case Op::ARITH_ADD:
        case Op::ARITH_SUB:
        case Op::ARITH_MUL:
        case Op::ARITH_DIV:
        case Op::ARITH_MOD:
             if (input_idx < 2) return Type::UNKNOWN; // Placeholder: Expect numeric, specific type checked at runtime
             break;
        case Op::ARITH_NEG:
        case Op::ARITH_ABS:
             if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect numeric
             break;
        case Op::ARITH_INC:
        case Op::ARITH_DEC:
             if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect integer/pointer
             break;
        // Bitwise
        case Op::BIT_AND:
        case Op::BIT_OR:
        case Op::BIT_XOR:
             if (input_idx < 2) return Type::UNKNOWN; // Placeholder: Expect integer
             break;
        case Op::BIT_NOT:
             if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect integer
             break;
        case Op::BIT_SHL:
        case Op::BIT_SHR:
        case Op::BIT_ASHR:
        case Op::BIT_ROL:
        case Op::BIT_ROR:
             if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect integer (value)
             if (input_idx == 1) return Type::UNKNOWN; // Placeholder: Expect integer (shift amount)
             break;
        // Comparison
        case Op::CMP_EQ:
        case Op::CMP_NE:
        case Op::CMP_LT:
        case Op::CMP_LE:
        case Op::CMP_GT:
        case Op::CMP_GE:
             if (input_idx < 2) return Type::UNKNOWN; // Placeholder: Expect comparable types
             break;
        // Logical
        case Op::LOGIC_AND:
        case Op::LOGIC_OR:
        case Op::LOGIC_XOR:
             if (input_idx < 2) return Type::BOOL;
             break;
        case Op::LOGIC_NOT:
             if (input_idx == 0) return Type::BOOL;
             break;
        // Memory
        case Op::MEM_LOAD:
             if (input_idx == 0) return Type::POINTER; // Or MEM_REF
             break;
        case Op::MEM_STORE:
             if (input_idx == 0) return Type::POINTER; // Or MEM_REF (Address)
             if (input_idx == 1) return Type::UNKNOWN; // Value (any type)
             break;
        case Op::MEM_ALLOC:
             if (input_idx == 0) return Type::UINT64; // Assuming size is uint64
             break;
        // Control Flow
        case Op::CTRL_BRANCH_COND:
             if (input_idx == 0) return Type::BOOL;
             break;
        // Conversion (Example)
        case Op::CONV_INT_TO_FLOAT:
             if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect Integer
             break;
         case Op::CONV_FLOAT_TO_INT:
              if (input_idx == 0) return Type::UNKNOWN; // Placeholder: Expect Float
              break;
         // Default: No specific expectation or unimplemented
         default:
            break;
    }
    return Type::UNKNOWN;
 }
 } // namespace bdi::core::graph
