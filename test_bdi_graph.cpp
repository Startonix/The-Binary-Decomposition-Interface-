 // File: tests/test_bdi_graph.cpp
 #include "gtest/gtest.h"
 #include "bdi/core/graph/BDIGraph.hpp" // Adjust path as needed
 #include "bdi/core/graph/BDINode.hpp"
 #include "bdi/core/types/BDITypes.hpp"
 #include "bdi/frontend/api/GraphBuilder.hpp" // Use builder for convenience
 #include <fstream>
 #include <filesystem> // Requires C++17
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::frontend::api;
 TEST(BDIGraphTest, AddNodesAndConnect) {
    GraphBuilder builder("TestGraph");
    NodeID node1 = builder.addNode(BDIOperationType::META_START);
    NodeID node2 = builder.addNode(BDIOperationType::ARITH_ADD);
    NodeID node3 = builder.addNode(BDIOperationType::MEM_STORE);
    NodeID node4 = builder.addNode(BDIOperationType::META_END);
    // Define ports needed for connection validation (types are illustrative)
    builder.defineDataOutput(node1, 0, BDIType::INT32); // Simulate some constant output from start
    builder.defineDataOutput(node1, 1, BDIType::INT32);
    builder.defineDataOutput(node2, 0, BDIType::INT32); // Output of ADD
    builder.defineDataOutput(node3, 0, BDIType::BOOL); // STORE might output success status
    EXPECT_TRUE(builder.connectData(node1, 0, node2, 0)); // Start output 0 -> ADD input 0
    EXPECT_TRUE(builder.connectData(node1, 1, node2, 1)); // Start output 1 -> ADD input 1
    EXPECT_TRUE(builder.connectData(node2, 0, node3, 1)); // ADD output 0 -> STORE input 1 (value)
    // Assume STORE input 0 is address (not connected here for simplicity)
    EXPECT_TRUE(builder.connectControl(node1, node2));
    EXPECT_TRUE(builder.connectControl(node2, node3));
    EXPECT_TRUE(builder.connectControl(node3, node4));
    std::unique_ptr<BDIGraph> graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    EXPECT_EQ(graph->getNodeCount(), 4);
    EXPECT_TRUE(graph->validateGraph()); // Basic structural validation
    // Check connections
    auto node2_ref_opt = graph->getNode(node2);
    ASSERT_TRUE(node2_ref_opt.has_value());
    const BDINode& node2_ref = node2_ref_opt.value();
    ASSERT_EQ(node2_ref.data_inputs.size(), 2);
    EXPECT_EQ(node2_ref.data_inputs[0].node_id, node1);
    EXPECT_EQ(node2_ref.data_inputs[0].port_index, 0);
    EXPECT_EQ(node2_ref.data_inputs[1].node_id, node1);
    EXPECT_EQ(node2_ref.data_inputs[1].port_index, 1);
    ASSERT_EQ(node2_ref.control_inputs.size(), 1);
    EXPECT_EQ(node2_ref.control_inputs[0], node1);
    ASSERT_EQ(node2_ref.control_outputs.size(), 1);
    EXPECT_EQ(node2_ref.control_outputs[0], node3);
 }
 TEST(BDIGraphTest, SerializationDeserialization) {
    GraphBuilder builder("SerializeTest");
    NodeID n_start = builder.addNode(BDIOperationType::META_START);
    NodeID n_const = builder.addNode(BDIOperationType::ARITH_ADD); // Use ADD just to have payload
    NodeID n_end = builder.addNode(BDIOperationType::META_END);
    builder.setNodePayload(n_const, TypedPayload::createFrom(int32_t{999}));
    builder.defineDataOutput(n_const, 0, BDIType::INT32);
    builder.connectControl(n_start, n_const);
    builder.connectControl(n_const, n_end);
    std::unique_ptr<BDIGraph> original_graph = builder.finalizeGraph();
    ASSERT_NE(original_graph, nullptr);
    ASSERT_TRUE(original_graph->validateGraph());
    const std::string temp_filename = "test_graph_serialization.bdi";
    // Serialize
    {
        std::ofstream ofs(temp_filename, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ASSERT_TRUE(original_graph->serialize(ofs));
    }
    // Deserialize
    std::unique_ptr<BDIGraph> deserialized_graph;
    {
        std::ifstream ifs(temp_filename, std::ios::binary);
        ASSERT_TRUE(ifs.is_open());
        deserialized_graph = BDIGraph::deserialize(ifs);
        ASSERT_NE(deserialized_graph, nullptr);
    }
    // Compare basic properties
    EXPECT_EQ(original_graph->getName(), deserialized_graph->getName());
    EXPECT_EQ(original_graph->getNodeCount(), deserialized_graph->getNodeCount());
    ASSERT_TRUE(deserialized_graph->validateGraph()); // Check structure integrity
    // Compare specific node
    auto orig_const_opt = original_graph->getNode(n_const);
    auto deser_const_opt = deserialized_graph->getNode(n_const); // Assumes IDs are preserved
    ASSERT_TRUE(orig_const_opt.has_value());
    ASSERT_TRUE(deser_const_opt.has_value());
    const BDINode& orig_const = orig_const_opt.value();
    const BDINode& deser_const = deser_const_opt.value();
    EXPECT_EQ(orig_const.operation, deser_const.operation);
    EXPECT_EQ(orig_const.payload.type, deser_const.payload.type);
    EXPECT_EQ(orig_const.payload.getAs<int32_t>(), deser_const.payload.getAs<int32_t>());
    EXPECT_EQ(orig_const.control_inputs, deser_const.control_inputs);
    EXPECT_EQ(orig_const.control_outputs, deser_const.control_outputs);
    // Clean up temporary file
    std::filesystem::remove(temp_filename);
 }
