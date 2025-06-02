 // File: tests/test_execution_context.cpp
 #include "gtest/gtest.h"
 #include "bdi/runtime/ExecutionContext.hpp" // Adjust path as needed
 #include "bdi/core/payload/TypedPayload.hpp" // For creating payloads
 using namespace bdi::runtime;
 using namespace bdi::core::graph;
 using namespace bdi::core::payload;
 using namespace bdi::core::types;
 TEST(ExecutionContextTest, SetAndGetValue) {
    ExecutionContext ctx;
    PortRef port1 = {10, 0};
    PortRef port2 = {10, 1};
    PortRef port3 = {20, 0};
    TypedPayload payload_int = TypedPayload::createFrom(int32_t{123});
    TypedPayload payload_float = TypedPayload::createFrom(float{45.6f});
    ctx.setPortValue(port1, payload_int);
    ctx.setPortValue(port2.node_id, port2.port_index, payload_float); // Alternative set method
    // Get existing values
    auto val1_opt = ctx.getPortValue(port1);
    ASSERT_TRUE(val1_opt.has_value());
    EXPECT_EQ(val1_opt.value().type, BDIType::INT32);
    EXPECT_EQ(val1_opt.value().getAs<int32_t>(), 123);
    auto val2_opt = ctx.getPortValue(port2.node_id, port2.port_index);
    ASSERT_TRUE(val2_opt.has_value());
    EXPECT_EQ(val2_opt.value().type, BDIType::FLOAT32);
    EXPECT_FLOAT_EQ(val2_opt.value().getAs<float>(), 45.6f);
    // Get non-existent value
    auto val3_opt = ctx.getPortValue(port3);
    EXPECT_FALSE(val3_opt.has_value());
 }
 TEST(ExecutionContextTest, OverwriteValue) {
    ExecutionContext ctx;
    PortRef port1 = {5, 0};
 ctx.setPortValue(port1, TypedPayload::createFrom(int32_t{10}));
    auto val1_opt = ctx.getPortValue(port1);
    ASSERT_TRUE(val1_opt.has_value());
    EXPECT_EQ(val1_opt.value().getAs<int32_t>(), 10);
    ctx.setPortValue(port1, TypedPayload::createFrom(int32_t{-50}));
    auto val2_opt = ctx.getPortValue(port1);
    ASSERT_TRUE(val2_opt.has_value());
    EXPECT_EQ(val2_opt.value().getAs<int32_t>(), -50);
 }
 TEST(ExecutionContextTest, ClearContext) {
    ExecutionContext ctx;
    PortRef port1 = {1, 0};
    ctx.setPortValue(port1, TypedPayload::createFrom(bool{true}));
    ASSERT_TRUE(ctx.getPortValue(port1).has_value());
    ctx.clear();
    EXPECT_FALSE(ctx.getPortValue(port1).has_value());
 }
 // TEST(ExecutionContextTest, CallStack) { // Defer until call stack implemented
 //     ExecutionContext ctx;
 //     EXPECT_TRUE(ctx.isCallStackEmpty());
 //     EXPECT_FALSE(ctx.popCall().has_value());
 //     ctx.pushCall(100);
 //     ctx.pushCall(200);
 //     EXPECT_FALSE(ctx.isCallStackEmpty());
 //     auto ret1 = ctx.popCall();
 //     ASSERT_TRUE(ret1.has_value());
 //     EXPECT_EQ(ret1.value(), 200);
 //     auto ret2 = ctx.popCall();
 //     ASSERT_TRUE(ret2.has_value());
 //     EXPECT_EQ(ret2.value(), 100);
 //     EXPECT_TRUE(ctx.isCallStackEmpty());
 // }
