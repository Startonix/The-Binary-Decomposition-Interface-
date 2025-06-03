// File: bdi/optimizer/passes/ConstantFolding.hpp
 #ifndef BDI_OPTIMIZER_PASSES_CONSTANTFOLDING_HPP
 #define BDI_OPTIMIZER_PASSES_CONSTANTFOLDING_HPP
 #include "../OptimizationPassBase.hpp"
 #include "../../runtime/BDIValueVariant.hpp" // Need variant for evaluation
 #include <unordered_map>
 #include <optional>
 namespace bdi::optimizer {
 using bdi::runtime::BDIValueVariant;
 using bdi::core::graph::NodeID;
 using bdi::core::graph::PortRef;
 // Simple constant folding pass
 class ConstantFolding : public OptimizationPassBase {
 public:
    ConstantFolding() : OptimizationPassBase("ConstantFolding") {}
    void visitGraph(BDIGraph& graph) override;
 private:
    // Structure to hold constant values found during traversal
    std::unordered_map<PortRef, BDIValueVariant, bdi::runtime::PortRefHash> constant_values_;
    BDIGraph* current_graph_ = nullptr; // Need graph access for rewiring
    // Attempts to evaluate a node if all its inputs are constant
    std::optional<BDIValueVariant> evaluateConstantNode(BDINode& node);
    // Tries to replace a node with a constant result
    void replaceNodeWithConstant(BDINode& node_to_replace, const BDIValueVariant& constant_result);
    // Helper to check if a node represents a constant value we can use
    std::optional<BDIValueVariant> getConstantValueFromNode(NodeID node_id);
 };
 } // namespace bdi::optimizer
 #endif // BDI_OPTIMIZER_PASSES_CONSTANTFOLDING_HPP
