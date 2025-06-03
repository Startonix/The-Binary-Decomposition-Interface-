// File: bdi/frontend/dsl/ArithmeticMapper.cpp
 #include "ArithmeticMapper.hpp"
 #include "../../core/graph/OperationTypes.hpp"
 #include "../../core/types/BDITypes.hpp"
 #include "../../core/payload/TypedPayload.hpp"
 #include <stdexcept>
 namespace bdi::frontend::dsl {
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 core::graph::NodeID ArithmeticMapper::mapToGraph(const std::any& dsl_representation, GraphBuilder& builder) {
    // Assume the std::any holds a pointer to the root ArithmeticExpr
    const ArithmeticExpr* root_expr = nullptr;
    try {
        root_expr = std::any_cast<const ArithmeticExpr*>(dsl_representation);
    } catch (const std::bad_any_cast& e) {
         throw std::runtime_error("ArithmeticMapper requires std::any to hold const ArithmeticExpr*");
    }
    if (!root_expr) {
        throw std::runtime_error("ArithmeticMapper received null expression pointer");
    }
    // Need a starting control flow point - maybe builder should provide one?
    // For now, assume caller handles START/END nodes.
    NodeID dummy_start = 0; // Indicates no predecessor initially
    return mapExpression(root_expr, builder, dummy_start);
 }
 core::graph::NodeID ArithmeticMapper::mapExpression(const ArithmeticExpr* expr, GraphBuilder& builder, NodeID& current_control_node) {
    if (!expr) return 0; // Invalid expression node
    switch (expr->op) {
        case ArithOp::CONST_I32: {
            // Create a CONST node (using NOP for now)
            NodeID const_node = builder.addNode(BDIOperationType::META_NOP, "CONST_I32"); // Tag with name
            builder.setNodePayload(const_node, TypedPayload::createFrom(expr->value));
            builder.defineDataOutput(const_node, 0, BDIType::INT32);
            // Connect control flow if a predecessor exists
            if (current_control_node != 0) {
                 builder.connectControl(current_control_node, const_node);
            }
            current_control_node = const_node; // Update control flow point
            return const_node; // Return the ID of the node producing the constant
        }
        case ArithOp::ADD:
        case ArithOp::SUB:
        case ArithOp::MUL:
        case ArithOp::DIV: {
            // Recursively map left and right children
            NodeID lhs_node_id = mapExpression(expr->lhs.get(), builder, current_control_node);
             // Important: Control flow point might have been updated by LHS mapping
            NodeID lhs_control_node = current_control_node; // Save control point after LHS
            NodeID rhs_node_id = mapExpression(expr->rhs.get(), builder, current_control_node);
             // Control flow point might have been updated again by RHS mapping
            if (lhs_node_id == 0 || rhs_node_id == 0) {
                throw std::runtime_error("Failed to map child expression");
            }
            // Create the BDI operation node
            BDIOperationType bdi_op;
            switch (expr->op) {
                case ArithOp::ADD: bdi_op = BDIOperationType::ARITH_ADD; break;
                case ArithOp::SUB: bdi_op = BDIOperationType::ARITH_SUB; break;
                case ArithOp::MUL: bdi_op = BDIOperationType::ARITH_MUL; break;
                case ArithOp::DIV: bdi_op = BDIOperationType::ARITH_DIV; break;
                default: throw std::logic_error("Unexpected ArithOp"); // Should not happen
            }
            NodeID op_node = builder.addNode(bdi_op);
            builder.defineDataOutput(op_node, 0, BDIType::INT32); // Assume INT32 result
            // Connect control flow: Both children's control flow must precede the operation
            builder.connectControl(lhs_control_node, op_node); // Control from LHS branch
            builder.connectControl(current_control_node, op_node); // Control from RHS branch (current point)
            // Connect data flow
            builder.connectData(lhs_node_id, 0, op_node, 0); // LHS result -> Op input 0
            builder.connectData(rhs_node_id, 0, op_node, 1); // RHS result -> Op input 1
            current_control_node = op_node; // Update control flow point
            return op_node; // Return the ID of the node producing the result
        }
        default:
            throw std::runtime_error("Unknown ArithOp encountered");
    }
 }
 } // namespace bdi::frontend::dsl
