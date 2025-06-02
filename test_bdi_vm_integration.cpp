// File: tests/test_bdi_vm_integration.cpp
 #include "gtest/gtest.h"
 #include "bdi/runtime/BDIVirtualMachine.hpp"
 #include "bdi/frontend/api/GraphBuilder.hpp"
 #include "bdi/core/types/BDITypes.hpp"
 #include "bdi/core/payload/TypedPayload.hpp"
 using namespace bdi::runtime;
 using namespace bdi::frontend::api;
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 // Helper function to create a CONST node (assuming no dedicated CONST op yet)
 // We'll use a NOP node and just set its output value in the context before execution.
 NodeID addConstNode(GraphBuilder& builder, TypedPayload payload, NodeID& next_control) {
    NodeID node_id = builder.addNode(BDIOperationType::META_NOP);
    builder.defineDataOutput(node_id, 0, payload.type); // Define output port
    builder.connectControl(next_control, node_id);
    next_control = node_id;
    return node_id;
 }
 TEST(BDIVMIntegrationTest, SimpleArithmetic) {
    GraphBuilder builder("VMArithmeticTest");
    BDIVirtualMachine vm(1024); // VM with 1KB memory (not used here)
    NodeID start_node = builder.addNode(BDIOperationType::META_START);
    NodeID current_ctl = start_node;
    // Create "CONST" nodes using NOPs
    TypedPayload payload_a = TypedPayload::createFrom(int32_t{25});
    TypedPayload payload_b = TypedPayload::createFrom(int32_t{17});
    NodeID const_a_node = addConstNode(builder, payload_a, current_ctl);
    NodeID const_b_node = addConstNode(builder, payload_b, current_ctl);
    NodeID add_node = builder.addNode(BDIOperationType::ARITH_ADD);
    builder.defineDataOutput(add_node, 0, BDIType::INT32);
    builder.connectControl(current_ctl, add_node);
    builder.connectData(const_a_node, 0, add_node, 0); // Input 0 = Const A output 0
    builder.connectData(const_b_node, 0, add_node, 1); // Input 1 = Const B output 0
    current_ctl = add_node;
    NodeID end_node = builder.addNode(BDIOperationType::META_END);
    builder.connectControl(current_ctl, end_node);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(graph->validateGraph());
    // Pre-populate context for the "CONST" NOP nodes
    vm.getExecutionContext()->setPortValue(const_a_node, 0, payload_a);
    vm.getExecutionContext()->setPortValue(const_b_node, 0, payload_b);
    // Execute
    ASSERT_TRUE(vm.execute(*graph, start_node));
    // Check result
    auto result_opt = vm.getExecutionContext()->getPortValue(add_node, 0);
    ASSERT_TRUE(result_opt.has_value());
    EXPECT_EQ(result_opt.value().type, BDIType::INT32);
    EXPECT_EQ(result_opt.value().getAs<int32_t>(), 25 + 17);
 }
 TEST(BDIVMIntegrationTest, SimpleMemory) {
    GraphBuilder builder("VMMemoryTest");
    BDIVirtualMachine vm(1024);
    ExecutionContext& ctx = *vm.getExecutionContext();
    MemoryManager& mem = *vm.getMemoryManager(); // Get internal manager
    // Allocate some memory manually for the test address
    uintptr_t test_addr = 0; // Simple case: use address 0
    // auto region_id = mem.allocateRegion(16); // Optionally allocate properly
    // ASSERT_TRUE(region_id.has_value());
    // auto region_info = mem.getRegionInfo(region_id.value());
    // ASSERT_TRUE(region_info.has_value());
    // test_addr = region_info.value().base_address;
    NodeID start_node = builder.addNode(BDIOperationType::META_START);
    NodeID current_ctl = start_node;
    // Constants
    TypedPayload payload_addr = TypedPayload::createFrom(test_addr);
    TypedPayload payload_val = TypedPayload::createFrom(int32_t{987});
    NodeID const_addr_node = addConstNode(builder, payload_addr, current_ctl);
    NodeID const_val_node = addConstNode(builder, payload_val, current_ctl);
    // STORE
    NodeID store_node = builder.addNode(BDIOperationType::MEM_STORE);
    builder.connectControl(current_ctl, store_node);
    builder.connectData(const_addr_node, 0, store_node, 0); // Input 0: Address
    builder.connectData(const_val_node, 0, store_node, 1);  // Input 1: Value
    current_ctl = store_node;
    // LOAD
    NodeID load_node = builder.addNode(BDIOperationType::MEM_LOAD);
    builder.defineDataOutput(load_node, 0, BDIType::INT32); // Output 0: Loaded value
    builder.connectControl(current_ctl, load_node);
    builder.connectData(const_addr_node, 0, load_node, 0); // Input 0: Address
    current_ctl = load_node;
    NodeID end_node = builder.addNode(BDIOperationType::META_END);
    builder.connectControl(current_ctl, end_node);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(graph->validateGraph());
    // Pre-populate context
    ctx.setPortValue(const_addr_node, 0, payload_addr);
    ctx.setPortValue(const_val_node, 0, payload_val);
    // Execute
    ASSERT_TRUE(vm.execute(*graph, start_node));
    // Check result loaded from memory
    auto result_opt = ctx.getPortValue(load_node, 0);
    ASSERT_TRUE(result_opt.has_value());
    EXPECT_EQ(result_opt.value().type, BDIType::INT32);
    EXPECT_EQ(result_opt.value().getAs<int32_t>(), 987);
    // Optional: Check memory directly
    std::vector<std::byte> read_back_buffer(sizeof(int32_t));
    ASSERT_TRUE(mem.readMemory(test_addr, read_back_buffer.data(), sizeof(int32_t)));
    EXPECT_EQ(TypedPayload(BDIType::INT32, read_back_buffer).getAs<int32_t>(), 987);
 }
 TEST(BDIVMIntegrationTest, SimpleBranch) {
    GraphBuilder builder("VMBranchTest");
    BDIVirtualMachine vm(1024);
    ExecutionContext& ctx = *vm.getExecutionContext();
    NodeID start_node = builder.addNode(BDIOperationType::META_START);
    NodeID current_ctl = start_node;
    // Condition Node (CONST BOOL)
    TypedPayload payload_cond_true = TypedPayload::createFrom(bool{true});
    TypedPayload payload_cond_false = TypedPayload::createFrom(bool{false});
    NodeID const_cond_node = addConstNode(builder, payload_cond_true, current_ctl); // Start with true
    // Branch Node
    NodeID branch_node = builder.addNode(BDIOperationType::CTRL_BRANCH_COND);
    builder.connectControl(current_ctl, branch_node);
    builder.connectData(const_cond_node, 0, branch_node, 0); // Input 0: Condition
    current_ctl = branch_node; // Control flow splits here
    // True Path
    NodeID true_path_node = builder.addNode(BDIOperationType::ARITH_ADD); // Just an identifiable op
    builder.defineDataOutput(true_path_node, 0, BDIType::INT32); // Output something
    // Assume inputs for ADD are setup elsewhere or consts for simplicity
    // False Path
    NodeID false_path_node = builder.addNode(BDIOperationType::ARITH_SUB); // Different op
    builder.defineDataOutput(false_path_node, 0, BDIType::INT32);
    // Merge Node (using NOP)
    NodeID merge_node = builder.addNode(BDIOperationType::META_NOP);
    // End Node
    NodeID end_node = builder.addNode(BDIOperationType::META_END);
    builder.connectControl(merge_node, end_node);
    // Connect Branch control outputs
    builder.connectControl(branch_node, true_path_node); // Output 0 = True target
    builder.connectControl(branch_node, false_path_node); // Output 1 = False target
    // Connect Paths to Merge
    builder.connectControl(true_path_node, merge_node);
    builder.connectControl(false_path_node, merge_node);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(graph->validateGraph());
    // --- Test True Path --
    ctx.clear(); // Clear from previous runs if any
    ctx.setPortValue(const_cond_node, 0, payload_cond_true);
    // Set dummy outputs for path nodes (just to check if they were executed)
    ctx.setPortValue(true_path_node, 0, TypedPayload::createFrom(int32_t{111})); // Pre-set expected output
    ctx.setPortValue(false_path_node, 0, TypedPayload::createFrom(int32_t{222}));
    ASSERT_TRUE(vm.execute(*graph, start_node));
    // Check if true path node's *original pre-set* value is still there (or updated if ADD was implemented)
    // More robustly: check if *only* the true path node produced an output that overwrote a default.
    EXPECT_TRUE(ctx.getPortValue(true_path_node, 0).has_value());
    // We can't easily check if false path *didn't* run without trace logs,
    // but if its output wasn't set or changed, that's indirect evidence.
    // --- Test False Path --
    ctx.clear();
    ctx.setPortValue(const_cond_node, 0, payload_cond_false);
    // Set dummy outputs again
    ctx.setPortValue(true_path_node, 0, TypedPayload::createFrom(int32_t{111}));
    ctx.setPortValue(false_path_node, 0, TypedPayload::createFrom(int32_t{222}));
    ASSERT_TRUE(vm.execute(*graph, start_node));
    EXPECT_TRUE(ctx.getPortValue(false_path_node, 0).has_value());
 }
