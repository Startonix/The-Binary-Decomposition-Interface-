// File: bdi/optimizer/OptimizationEngine.hpp
 #ifndef BDI_OPTIMIZER_OPTIMIZATIONENGINE_HPP
 #define BDI_OPTIMIZER_OPTIMIZATIONENGINE_HPP
 #include "OptimizationPassBase.hpp"
 #include "../core/graph/BDIGraph.hpp"
 #include <vector>
 #include <memory> // For unique_ptr
 namespace bdi::optimizer {
 class OptimizationEngine {
 public:
    OptimizationEngine() = default;
    // Add an optimization pass to the pipeline
    void addPass(std::unique_ptr<OptimizationPassBase> pass);
    // Run all registered passes on the graph until no more changes occur (optional)
    // or for a fixed number of iterations. Returns true if any changes were made.
    bool run(BDIGraph& graph, int max_iterations = 10);
 private:
    std::vector<std::unique_ptr<OptimizationPassBase>> passes_;
 };
 } // namespace bdi::optimizer
 #endif // BDI_OPTIMIZER_OPTIMIZATIONENGINE_HPP
