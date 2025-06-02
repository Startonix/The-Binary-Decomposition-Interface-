// File: bdi/runtime/BDIVirtualMachine.hpp
 #ifndef BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #define BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
 #include "../core/graph/BDIGraph.hpp"
 #include <memory> // For std::shared_ptr or unique_ptr if VM owns graph
 namespace bdi::runtime {
 using bdi::core::graph::BDIGraph;
 using bdi::core::graph::NodeID;
 // Basic Stub for the BDI Virtual Machine / Interpreter
 class BDIVirtualMachine {
 public:
    BDIVirtualMachine(); // Constructor might take memory manager, scheduler etc.
    // Primary execution entry point
    // Takes graph by reference, doesn't assume ownership here.
    // Returns success/failure or final state info.
    bool execute(BDIGraph& graph, NodeID entry_node_id);
    // TODO: Add methods for stepping, debugging, state inspection, loading graphs
 private:
    // --- Internal VM State --
    NodeID current_node_id_;
    // TODO: Represent register file / execution context / stack
    // TODO: Interface to MemoryManager
    // TODO: Interface to RuntimeScheduler
    // TODO: Interface to TraceGenerator
    // --- Execution Loop Helpers --
    bool fetchDecodeExecuteCycle(BDIGraph& graph);
    bool executeNode(BDINode& node); // Dispatch based on node.operation
    NodeID determineNextNode(BDINode& node); // Follow control flow
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_BDIVIRTUALMACHINE_HPP
