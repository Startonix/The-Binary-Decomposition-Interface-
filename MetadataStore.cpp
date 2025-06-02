// File: bdi/meta/MetadataStore.cpp
 #include "MetadataStore.hpp"
 namespace bdi::meta {
 MetadataHandle MetadataStore::addMetadata(MetadataVariant metadata) {
    MetadataHandle handle = next_handle_.fetch_add(1, std::memory_order_relaxed);
    store_[handle] = std::move(metadata);
    return handle;
 }
 const MetadataVariant* MetadataStore::getMetadata(MetadataHandle handle) const {
    auto it = store_.find(handle);
    if (it != store_.end()) {
        return &it->second;
    }
    return nullptr;
 }
 MetadataVariant* MetadataStore::getMetadataMutable(MetadataHandle handle) {
     auto it = store_.find(handle);
    if (it != store_.end()) {
        return &it->second;
    }
    return nullptr;
 }
 bool MetadataStore::updateMetadata(MetadataHandle handle, MetadataVariant metadata) {
    auto it = store_.find(handle);
    if (it != store_.end()) {
        it->second = std::move(metadata);
        return true;
    }
    return false;
 }
 bool MetadataStore::removeMetadata(MetadataHandle handle) {
    return store_.erase(handle) > 0;
 }
 } // namespace bdi::meta
