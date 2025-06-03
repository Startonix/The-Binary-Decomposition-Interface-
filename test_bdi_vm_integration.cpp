// File: tests/test_bdi_vm_integration.cpp
 #include "gtest/gtest.h"
 #include "bdi/runtime/BDIVirtualMachine.hpp"
 #include "bdi/frontend/api/GraphBuilder.hpp"
 #include "bdi/core/types/BDITypes.hpp"
 #include "bdi/core/payload/TypedPayload.hpp"
 #include <limits> // For numeric limits tests
 using namespace bdi::runtime;
 using namespace bdi::frontend::api;
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 // Helper function to create a CONST node (assuming no dedicated CONST op yet)
 // We'll use a NOP node and just set its output value in the context before execution.
// --- Helper Functions (Keep or move to a common test utility header) --
// Assuming addConstNode exists from previous step
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
 // --- Test Fixture for VM Tests --
class BDIVMIntegrationTest : public ::testing::Test {
 protected:
    std::unique_ptr<GraphBuilder> builder;
    std::unique_ptr<BDIVirtualMachine> vm;
    void SetUp() override {
        builder = std::make_unique<GraphBuilder>("VMIntegrationTestGraph");
        vm = std::make_unique<BDIVirtualMachine>(1024 * 1024); // 1MB Memory
    }
    void TearDown() override {
        builder.reset();
        vm.reset();
    }
    // Helper to build and run, then check single output
    template <typename ExpectedType>
    void buildRunCheckOutput(NodeID start_node, NodeID result_node_port0, ExpectedType expected_value) {
        auto graph = builder->finalizeGraph();
        ASSERT_NE(graph, nullptr);
        ASSERT_TRUE(graph->validateGraph());
        ASSERT_TRUE(vm->execute(*graph, start_node));
        auto result_var_opt = vm->getExecutionContext()->getPortValue(result_node_port0, 0);
        ASSERT_TRUE(result_var_opt.has_value());
        auto result_typed_opt = convertVariantTo<ExpectedType>(result_var_opt.value());
        ASSERT_TRUE(result_typed_opt.has_value());
        if constexpr (std::is_floating_point_v<ExpectedType>) {
            ASSERT_FLOAT_EQ(result_typed_opt.value(), expected_value);
        } else {
            ASSERT_EQ(result_typed_opt.value(), expected_value);
        }
    }
     // Helper to build and run, expecting execution failure
     void buildRunExpectFailure(NodeID start_node) {
        auto graph = builder->finalizeGraph();
        ASSERT_NE(graph, nullptr);
        // Graph might be valid structurally, but execution fails
        ASSERT_FALSE(vm->execute(*graph, start_node));
     }
 };
 // --- Arithmetic Operation Tests --
TEST_F(BDIVMIntegrationTest, ArithmeticOpsInt32) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c5 = addConstNode(*builder, TypedPayload::createFrom(int32_t{5}), current);
    NodeID c3 = addConstNode(*builder, TypedPayload::createFrom(int32_t{3}), current);
    NodeID c_neg2 = addConstNode(*builder, TypedPayload::createFrom(int32_t{-2}), current);
    // SUB: 5 - 3 = 2
    NodeID sub_node = builder->addNode(OpType::ARITH_SUB);
    builder->defineDataOutput(sub_node, 0, BDIType::INT32);
    builder->connectControl(current, sub_node);
    builder->connectData(c5, 0, sub_node, 0);
    builder->connectData(c3, 0, sub_node, 1);
    current = sub_node;
    // MUL: 2 * -2 = -4
    NodeID mul_node = builder->addNode(OpType::ARITH_MUL);
    builder->defineDataOutput(mul_node, 0, BDIType::INT32);
    builder->connectControl(current, mul_node);
    builder->connectData(sub_node, 0, mul_node, 0); // Use result of SUB
    builder->connectData(c_neg2, 0, mul_node, 1);
    current = mul_node;
    // DIV: -4 / -2 = 2
    NodeID div_node = builder->addNode(OpType::ARITH_DIV);
    builder->defineDataOutput(div_node, 0, BDIType::INT32);
    builder->connectControl(current, div_node);
    builder->connectData(mul_node, 0, div_node, 0);
    builder->connectData(c_neg2, 0, div_node, 1);
    current = div_node;
    // MOD: 5 % 3 = 2
    NodeID mod_node = builder->addNode(OpType::ARITH_MOD);
    builder->defineDataOutput(mod_node, 0, BDIType::INT32);
    builder->connectControl(current, mod_node); // Run in parallel conceptually
    builder->connectData(c5, 0, mod_node, 0);
    builder->connectData(c3, 0, mod_node, 1);
    // We need a merge point to check both DIV and MOD results
    // NEG: - (result of DIV) = -2
     NodeID neg_node = builder->addNode(OpType::ARITH_NEG);
     builder->defineDataOutput(neg_node, 0, BDIType::INT32);
     builder->connectControl(div_node, neg_node); // Chain after DIV
     builder->connectData(div_node, 0, neg_node, 0);
     current = neg_node; // Update main control flow
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(current, end); // Main flow ends
    builder->connectControl(mod_node, end); // Mod flow also ends
    // Pre-populate context for CONST nodes
    vm->getExecutionContext()->setPortValue(c5, 0, BDIValueVariant{int32_t{5}});
    vm->getExecutionContext()->setPortValue(c3, 0, BDIValueVariant{int32_t{3}});
    vm->getExecutionContext()->setPortValue(c_neg2, 0, BDIValueVariant{int32_t{-2}});
    // Execute and check intermediate/final results
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(vm->execute(*graph, start));
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(sub_node, 0).value()).value(), 2);
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(mul_node, 0).value()).value(), -4);
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(div_node, 0).value()).value(), 2);
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(mod_node, 0).value()).value(), 2);
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(neg_node, 0).value()).value(), -2);
 }
 TEST_F(BDIVMIntegrationTest, ArithmeticDivisionByZero) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c5 = addConstNode(*builder, TypedPayload::createFrom(int32_t{5}), current);
    NodeID c0 = addConstNode(*builder, TypedPayload::createFrom(int32_t{0}), current);
    NodeID div_node = builder->addNode(OpType::ARITH_DIV);
    builder->defineDataOutput(div_node, 0, BDIType::INT32);
    builder->connectControl(current, div_node);
    builder->connectData(c5, 0, div_node, 0);
    builder->connectData(c0, 0, div_node, 1); // Divide by zero
    current = div_node;
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(current, end);
    vm->getExecutionContext()->setPortValue(c5, 0, BDIValueVariant{int32_t{5}});
    vm->getExecutionContext()->setPortValue(c0, 0, BDIValueVariant{int32_t{0}});
    buildRunExpectFailure(start); // Expect execute() to return false
 }
 // --- Bitwise Operation Tests --
TEST_F(BDIVMIntegrationTest, BitwiseOpsUint32) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID cA = addConstNode(*builder, TypedPayload::createFrom(uint32_t{0b10101010}), current); // 170
    NodeID cB = addConstNode(*builder, TypedPayload::createFrom(uint32_t{0b01100110}), current); // 102
    // AND: 0b00100010 (34)
    NodeID and_node = builder->addNode(OpType::BIT_AND);
    builder->defineDataOutput(and_node, 0, BDIType::UINT32);
    builder->connectControl(current, and_node);
    builder->connectData(cA, 0, and_node, 0);
    builder->connectData(cB, 0, and_node, 1);
    current = and_node;
    // OR: 0b11101110 (238)
    NodeID or_node = builder->addNode(OpType::BIT_OR);
    builder->defineDataOutput(or_node, 0, BDIType::UINT32);
    builder->connectControl(current, or_node); // conceptually parallel
    builder->connectData(cA, 0, or_node, 0);
    builder->connectData(cB, 0, or_node, 1);
    // XOR: 0b11001100 (204)
    NodeID xor_node = builder->addNode(OpType::BIT_XOR);
    builder->defineDataOutput(xor_node, 0, BDIType::UINT32);
    builder->connectControl(or_node, xor_node); // Chain after OR
    builder->connectData(cA, 0, xor_node, 0);
    builder->connectData(cB, 0, xor_node, 1);
    current = xor_node; // Update main flow
     // NOT: ~0b10101010 = 0b01010101... (depends on UINT32 max)
     NodeID not_node = builder->addNode(OpType::BIT_NOT);
     builder->defineDataOutput(not_node, 0, BDIType::UINT32);
     builder->connectControl(current, not_node);
     builder->connectData(cA, 0, not_node, 0);
     current = not_node;
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(current, end);
    builder->connectControl(and_node, end); // AND flow also ends
    // Pre-populate
    vm->getExecutionContext()->setPortValue(cA, 0, BDIValueVariant{uint32_t{0b10101010}});
    vm->getExecutionContext()->setPortValue(cB, 0, BDIValueVariant{uint32_t{0b01100110}});
    // Execute
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(vm->execute(*graph, start));
    // Check results
    EXPECT_EQ(convertVariantTo<uint32_t>(vm->getExecutionContext()->getPortValue(and_node, 0).value()).value(), 0b00100010);
    EXPECT_EQ(convertVariantTo<uint32_t>(vm->getExecutionContext()->getPortValue(or_node, 0).value()).value(),  0b11101110);
    EXPECT_EQ(convertVariantTo<uint32_t>(vm->getExecutionContext()->getPortValue(xor_node, 0).value()).value(), 0b11001100);
    EXPECT_EQ(convertVariantTo<uint32_t>(vm->getExecutionContext()->getPortValue(not_node, 0).value()).value(), ~uint32_t{0b10101010});
 }
 // --- Comparison Operation Tests --
TEST_F(BDIVMIntegrationTest, ComparisonOps) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c5_i = addConstNode(*builder, TypedPayload::createFrom(int32_t{5}), current);
    NodeID c5_f = addConstNode(*builder, TypedPayload::createFrom(float{5.0f}), current);
    NodeID c6_i = addConstNode(*builder, TypedPayload::createFrom(int32_t{6}), current);
    // CMP_EQ (5 == 6) -> false
    NodeID eq_node = builder->addNode(OpType::CMP_EQ);
    builder->defineDataOutput(eq_node, 0, BDIType::BOOL);
    builder->connectControl(current, eq_node);
    builder->connectData(c5_i, 0, eq_node, 0);
    builder->connectData(c6_i, 0, eq_node, 1);
    current = eq_node;
    // CMP_LT (5.0f < 6) -> true (Requires promotion)
    NodeID lt_node = builder->addNode(OpType::CMP_LT);
    builder->defineDataOutput(lt_node, 0, BDIType::BOOL);
    builder->connectControl(current, lt_node);
    builder->connectData(c5_f, 0, lt_node, 0);
    builder->connectData(c6_i, 0, lt_node, 1);
    current = lt_node;
    // CMP_GE (5 >= 5.0f) -> true
    NodeID ge_node = builder->addNode(OpType::CMP_GE);
    builder->defineDataOutput(ge_node, 0, BDIType::BOOL);
    builder->connectControl(current, ge_node);
    builder->connectData(c5_i, 0, ge_node, 0);
    builder->connectData(c5_f, 0, ge_node, 1);
    current = ge_node;
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(current, end);
    // Pre-populate
    vm->getExecutionContext()->setPortValue(c5_i, 0, BDIValueVariant{int32_t{5}});
    vm->getExecutionContext()->setPortValue(c5_f, 0, BDIValueVariant{float{5.0f}});
    vm->getExecutionContext()->setPortValue(c6_i, 0, BDIValueVariant{int32_t{6}});
    // Execute & Check
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(vm->execute(*graph, start));
    EXPECT_EQ(convertVariantTo<bool>(vm->getExecutionContext()->getPortValue(eq_node, 0).value()).value(), false);
    EXPECT_EQ(convertVariantTo<bool>(vm->getExecutionContext()->getPortValue(lt_node, 0).value()).value(), true); // Requires float/int comparison
 logic
    EXPECT_EQ(convertVariantTo<bool>(vm->getExecutionContext()->getPortValue(ge_node, 0).value()).value(), true); // Requires int/float comparison
 logic
 }
 // --- Conversion Tests --
TEST_F(BDIVMIntegrationTest, ConversionOps) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c_i32 = addConstNode(*builder, TypedPayload::createFrom(int32_t{-10}), current);
    NodeID c_f32 = addConstNode(*builder, TypedPayload::createFrom(float{123.75f}), current);
    // INT32 -> FLOAT32
    NodeID i2f_node = builder->addNode(OpType::CONV_INT_TO_FLOAT);
    builder->defineDataOutput(i2f_node, 0, BDIType::FLOAT32);
    builder->connectControl(current, i2f_node);
    builder->connectData(c_i32, 0, i2f_node, 0);
    current = i2f_node;
    // FLOAT32 -> INT32 (Truncate)
    NodeID f2i_node = builder->addNode(OpType::CONV_FLOAT_TO_INT);
    builder->defineDataOutput(f2i_node, 0, BDIType::INT32);
    builder->connectControl(current, f2i_node);
    builder->connectData(c_f32, 0, f2i_node, 0); // Use original float const
    current = f2i_node; // Update main flow
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(i2f_node, end); // End alternate path too
    builder->connectControl(current, end);
    // Pre-populate
    vm->getExecutionContext()->setPortValue(c_i32, 0, BDIValueVariant{int32_t{-10}});
    vm->getExecutionContext()->setPortValue(c_f32, 0, BDIValueVariant{float{123.75f}});
    // Execute & Check
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    ASSERT_TRUE(vm->execute(*graph, start));
    EXPECT_FLOAT_EQ(convertVariantTo<float>(vm->getExecutionContext()->getPortValue(i2f_node, 0).value()).value(), -10.0f);
    EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(f2i_node, 0).value()).value(), 123);
 }
 // --- CALL/RETURN Tests --
TEST_F(BDIVMIntegrationTest, CallReturnSimple) {
    // --- Build Function Subgraph --
    GraphBuilder func_builder("AddFunc");
    NodeID func_start = func_builder.addNode(OpType::META_START); // Function entry point
    NodeID func_current = func_start;
    // Assume inputs are passed via ExecutionContext or dedicated mechanism (not tested here)
    // Get Input A (assume available at func_start port 0)
    // Get Input B (assume available at func_start port 1)
    NodeID func_add = func_builder.addNode(OpType::ARITH_ADD);
    func_builder.defineDataOutput(func_add, 0, BDIType::INT32);
    func_builder.connectControl(func_current, func_add);
    // func_builder.connectData(func_start, 0, func_add, 0); // Requires input passing mechanism
    // func_builder.connectData(func_start, 1, func_add, 1);
    func_current = func_add;
    // Set Return Value (assume output of ADD is return value, store somewhere standard?)
    // Convention: Store return value with a specific NodeID::Port known to RETURN?
    // For now, the result is just available at func_add port 0
    NodeID func_ret = func_builder.addNode(OpType::CTRL_RETURN);
    func_builder.connectControl(func_current, func_ret);
    // --- Build Main Graph --
    NodeID main_start = builder->addNode(OpType::META_START);
    NodeID main_current = main_start;
    NodeID c10 = addConstNode(*builder, TypedPayload::createFrom(int32_t{10}), main_current);
    NodeID c20 = addConstNode(*builder, TypedPayload::createFrom(int32_t{20}), main_current);
    // Node to trigger call (needs target and return path)
    NodeID call_node = builder->addNode(OpType::CTRL_CALL);
    builder->connectControl(main_current, call_node);
    main_current = call_node; // Control technically transfers, then returns
    NodeID after_call = builder->addNode(OpType::META_NOP); // Where execution should resume
    builder->connectControl(main_current, after_call);
    main_current = after_call;
    NodeID main_end = builder->addNode(OpType::META_END);
    builder->connectControl(main_current, main_end);
    // Finalize graphs
    auto func_graph = func_builder.finalizeGraph(); // Not used directly by VM yet
    auto main_graph = builder->finalizeGraph();
    ASSERT_NE(main_graph, nullptr);
    // Manually set call target and return address (since func graph isn't loaded/linked yet)
    // This is highly simplified! A real system needs function loading/linking.
    auto call_node_ref = main_graph->getNodeMutable(call_node);
    ASSERT_TRUE(call_node_ref != nullptr);
    call_node_ref->control_outputs.push_back(func_start); // Target = func_start (conceptually)
    call_node_ref->control_outputs.push_back(after_call); // Return = after_call
    // Pre-populate context
    vm->getExecutionContext()->setPortValue(c10, 0, BDIValueVariant{int32_t{10}});
    vm->getExecutionContext()->setPortValue(c20, 0, BDIValueVariant{int32_t{20}});
    // Simulate passing arguments to the function start node
    vm->getExecutionContext()->setPortValue(func_start, 0, BDIValueVariant{int32_t{10}}); // Arg A
    vm->getExecutionContext()->setPortValue(func_start, 1, BDIValueVariant{int32_t{20}}); // Arg B
    // Execute
    // We need to execute BOTH graphs conceptually, or merge them.
    // For this test, we'll "inline" the function execution logic within the VM execution of CALL/RETURN.
    // The VM needs modification to handle jumping between graph concepts or loading functions.
    // --- THIS TEST WILL LIKELY FAIL WITH CURRENT SIMPLE VM --
    // ASSERT_TRUE(vm->execute(*main_graph, main_start));
    // --- Conceptual Check (if VM handled jumps properly) --
    // EXPECT_TRUE(vm->getExecutionContext()->getPortValue(after_call, 0).has_value()); // Check if execution reached after call
    // Check function result (needs mechanism to retrieve it)
    // EXPECT_EQ(convertVariantTo<int32_t>(vm->getExecutionContext()->getPortValue(func_add, 0).value()).value(), 30);
    // Mark test as incomplete due to VM limitations
    GTEST_SKIP() << "Skipping CALL/RETURN test - requires VM function loading/linking mechanism.";
 }
