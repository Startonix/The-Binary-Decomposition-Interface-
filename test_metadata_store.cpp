 // File: tests/test_metadata_store.cpp
 #include "gtest/gtest.h"
 #include "bdi/meta/MetadataStore.hpp" // Adjust path as needed
 using namespace bdi::meta;
 TEST(MetadataStoreTest, AddAndGet) {
    MetadataStore store;
    SemanticTag tag1 = {"DSL:Test:1", "Test semantic tag"};
    ProofTag tag2 = {ProofTag::ProofSystem::LEAN_HASH, {std::byte{0xAB}, std::byte{0xCD}}};
    HardwareHints tag3 = {HardwareHints::CacheLocality::HINT_L2, 5, true};
    MetadataHandle h1 = store.addMetadata(tag1);
    MetadataHandle h2 = store.addMetadata(tag2);
    MetadataHandle h3 = store.addMetadata(tag3);
    EXPECT_NE(h1, 0);
    EXPECT_NE(h2, 0);
    EXPECT_NE(h3, 0);
    EXPECT_NE(h1, h2);
    EXPECT_NE(h1, h3);
    EXPECT_NE(h2, h3);
    const MetadataVariant* v1 = store.getMetadata(h1);
    ASSERT_NE(v1, nullptr);
    EXPECT_TRUE(std::holds_alternative<SemanticTag>(*v1));
    EXPECT_EQ(std::get<SemanticTag>(*v1).dsl_source_ref, "DSL:Test:1");
const MetadataVariant* v2 = store.getMetadata(h2);
    ASSERT_NE(v2, nullptr);
    EXPECT_TRUE(std::holds_alternative<ProofTag>(*v2));
    EXPECT_EQ(std::get<ProofTag>(*v2).system, ProofTag::ProofSystem::LEAN_HASH);
    EXPECT_EQ(std::get<ProofTag>(*v2).proof_data_hash.size(), 2);
    const MetadataVariant* v3 = store.getMetadata(h3);
    ASSERT_NE(v3, nullptr);
    EXPECT_TRUE(std::holds_alternative<HardwareHints>(*v3));
    EXPECT_EQ(std::get<HardwareHints>(*v3).cache_hint, HardwareHints::CacheLocality::HINT_L2);
    EXPECT_EQ(std::get<HardwareHints>(*v3).preferred_compute_unit_id, 5);
    const MetadataVariant* v_null = store.getMetadata(h3 + 10); // Non-existent handle
    EXPECT_EQ(v_null, nullptr);
 }
 TEST(MetadataStoreTest, Update) {
    MetadataStore store;
    SemanticTag tag_orig = {"Orig:1", "Original"};
    MetadataHandle h1 = store.addMetadata(tag_orig);
    SemanticTag tag_updated = {"Updated:2", "New Value"};
    EXPECT_TRUE(store.updateMetadata(h1, tag_updated));
    const MetadataVariant* v1 = store.getMetadata(h1);
    ASSERT_NE(v1, nullptr);
    EXPECT_TRUE(std::holds_alternative<SemanticTag>(*v1));
    EXPECT_EQ(std::get<SemanticTag>(*v1).dsl_source_ref, "Updated:2");
    EXPECT_FALSE(store.updateMetadata(h1 + 1, tag_updated)); // Update non-existent handle
 }
 TEST(MetadataStoreTest, Remove) {
     MetadataStore store;
     MetadataHandle h1 = store.addMetadata(SemanticTag{"Test:1", "Data"});
     EXPECT_NE(store.getMetadata(h1), nullptr);
     EXPECT_TRUE(store.removeMetadata(h1));
     EXPECT_EQ(store.getMetadata(h1), nullptr);
     EXPECT_FALSE(store.removeMetadata(h1)); // Remove again fails
