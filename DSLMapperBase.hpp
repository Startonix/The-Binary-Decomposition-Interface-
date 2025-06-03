 // File: bdi/frontend/dsl/DSLMapperBase.hpp
 #ifndef BDI_FRONTEND_DSL_DSLMAPPERSBASE_HPP
 #define BDI_FRONTEND_DSL_DSLMAPPERSBASE_HPP
 #include "../api/GraphBuilder.hpp"
 #include <any> // Or define a common DSL input structure base
 namespace bdi::frontend::dsl {
 using bdi::frontend::api::GraphBuilder;
 class DSLMapperBase {
 public:
    virtual ~DSLMapperBase() = default;
    // Pure virtual function to map a DSL representation to BDI graph nodes
    // Input 'dsl_representation' might be a specific struct, AST node, etc.
    // Returns the NodeID of the final output node of the mapped subgraph, or 0 on error.
    virtual core::graph::NodeID mapToGraph(const std::any& dsl_representation, GraphBuilder& builder) = 0;
 };
 } // namespace bdi::frontend::dsl
 #endif // BDI_FRONTEND_DSL_DSLMAPPERSBASE_HPP-------------------------------------------------------------------------------------------------
// File: bdi/frontend/dsl/ArithmeticMapper.hpp
 #ifndef BDI_FRONTEND_DSL_ARITHMETICMAPPER_HPP
 #define BDI_FRONTEND_DSL_ARITHMETICMAPPER_HPP
 #include "DSLMapperBase.hpp"
 #include <string>
 #include <memory> // For unique_ptr in expression tree
 namespace bdi::frontend::dsl {
 // --- Simple Arithmetic Expression Structure --
enum class ArithOp { ADD, SUB, MUL, DIV, CONST_I32 };
 struct ArithmeticExpr {
    ArithOp op;
    int32_t value = 0; // Used for CONST_I32
    std::unique_ptr<ArithmeticExpr> lhs = nullptr;
    std::unique_ptr<ArithmeticExpr> rhs = nullptr;
    // Convenience constructors
    ArithmeticExpr(ArithOp o, std::unique_ptr<ArithmeticExpr> l, std::unique_ptr<ArithmeticExpr> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    ArithmeticExpr(int32_t val) : op(ArithOp::CONST_I32), value(val) {}
 };
 // --- Mapper Implementation --
class ArithmeticMapper : public DSLMapperBase {
public:
    // Maps an ArithmeticExpr tree to BDI graph nodes
    core::graph::NodeID mapToGraph(const std::any& dsl_representation, GraphBuilder& builder) override;
 private:
     // Recursive helper function
     core::graph::NodeID mapExpression(const ArithmeticExpr* expr, GraphBuilder& builder, core::graph::NodeID& current_control_node);
 };
 } // namespace bdi::frontend::dsl
 #endif // BDI_FRONTEND_DSL_ARITHMETICMAPPER_HPP
