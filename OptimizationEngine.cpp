// File: bdi/optimizer/OptimizationEngine.cpp
 #include "OptimizationEngine.hpp"
 #include <iostream>
 namespace bdi::optimizer {
 void OptimizationEngine::addPass(std::unique_ptr<OptimizationPassBase> pass) {
    if (pass) {
        passes_.push_back(std::move(pass));
    }
 }
 bool OptimizationEngine::run(BDIGraph& graph, int max_iterations) {
    bool changed_overall = false;
    bool changed_in_iteration = false;
    int iteration = 0;
    if (passes_.empty()) {
        return false;
    }
    std::cout << "OptimizationEngine: Running passes..." << std::endl;
    do {
        changed_in_iteration = false;
        iteration++;
        std::cout << "  Iteration " << iteration << "..." << std::endl;
        for (const auto& pass : passes_) {
            std::cout << "    Running Pass: " << pass->getName() << "..." << std::endl;
            bool pass_changed = pass->run(graph);
            if (pass_changed) {
                std::cout << "      Graph modified by " << pass->getName() << "." << std::endl;
                changed_in_iteration = true;
                changed_overall = true;
                 // Optional: Re-validate graph after modification?
                 // if (!graph.validateGraph()) {
                 //     std::cerr << "      ERROR: Graph invalid after pass " << pass->getName() << "!" << std::endl;
                 //     return changed_overall; // Abort?
                 // }
            }
         if (!changed_in_iteration) {
             std::cout << "  No changes in iteration " << iteration << ". Optimization stable." << std::endl;
        }
    } while (changed_in_iteration && iteration < max_iterations);
    if (iteration >= max_iterations && changed_in_iteration) {
         std::cout << "OptimizationEngine: Warning - Max iterations reached, optimizations might not be stable." << std::endl;
    }
     std::cout << "OptimizationEngine: Finished." << std::endl;
    return changed_overall;
 }
 } // namespace bdi::optimizer
