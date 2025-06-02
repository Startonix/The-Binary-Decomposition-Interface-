// File: bdi/runtime/MemoryManager.cpp
 #include "MemoryManager.hpp"
 #include <stdexcept> // For invalid_argument
 #include <cstring> // For memcpy
 #include <iostream> // For debug/warnings
 namespace bdi::runtime {
 MemoryManager::MemoryManager(size_t total_memory_bytes)
    : memory_block_(total_memory_bytes), next_region_id_(1), next_allocation_offset_(0)
 {
    if (total_memory_bytes == 0) {
        throw std::invalid_argument("MemoryManager size cannot be zero.");
    }
    std::cout << "MemoryManager: Initialized with " << total_memory_bytes << " bytes." << std::endl;
 }
 std::optional<RegionID> MemoryManager::allocateRegion(size_t size_bytes, bool read_only) {
    // Very simple bump allocator - DOES NOT HANDLE FREEING PROPERLY YET
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    if (next_allocation_offset_ + size_bytes > memory_block_.size()) {
        std::cerr << "MemoryManager Error: Out of memory trying to allocate " << size_bytes << " bytes." << std::endl;
        return std::nullopt; // Out of memory
    }
    RegionID new_id = next_region_id_++;
    uintptr_t base_address = next_allocation_offset_;
    next_allocation_offset_ += size_bytes;
    allocated_regions_.emplace(new_id, MemoryRegion(new_id, base_address, size_bytes, read_only));
    std::cout << "MemoryManager: Allocated Region " << new_id << " (" << size_bytes << " bytes) at address " << base_address << std::endl;
    return new_id;
 }
 bool MemoryManager::freeRegion(RegionID region_id) {
    // STUB: Freeing requires a more complex allocator than bump allocation
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
auto it = allocated_regions_.find(region_id);
    if (it != allocated_regions_.end()) {
        std::cout << "MemoryManager: Freeing Region " << region_id << " (Allocator Stub - No actual free)" << std::endl;
        // With a real allocator, mark space as free here
        allocated_regions_.erase(it); // Remove tracking info
        return true;
    }
    std::cerr << "MemoryManager Error: Cannot free non-existent region " << region_id << std::endl;
    return false;
 }
 std::optional<MemoryRegion> MemoryManager::getRegionInfo(RegionID region_id) const {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    auto it = allocated_regions_.find(region_id);
    if (it != allocated_regions_.end()) {
        return it->second;
    }
    return std::nullopt;
 }
 bool MemoryManager::readMemory(uintptr_t address, std::byte* buffer, size_t size_bytes) const {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    if (address + size_bytes > memory_block_.size()) {
         std::cerr << "MemoryManager Error: Read access out of bounds (Address: " << address << ", Size: " << size_bytes << ")" << std::endl;
        return false; // Out of bounds
    }
    // TODO: Check if read overlaps with valid allocated regions? (More complex check)
    std::memcpy(buffer, memory_block_.data() + address, size_bytes);
    return true;
 }
 bool MemoryManager::writeMemory(uintptr_t address, const std::byte* buffer, size_t size_bytes) {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    if (address + size_bytes > memory_block_.size()) {
        std::cerr << "MemoryManager Error: Write access out of bounds (Address: " << address << ", Size: " << size_bytes << ")" << std::endl;
        return false; // Out of bounds
    }
    // TODO: Check if write overlaps with valid allocated regions?
    // TODO: Check read-only flags of overlapping regions?
    // This simple implementation doesn't check region permissions.
    std::memcpy(memory_block_.data() + address, buffer, size_bytes);
    return true;
 }
 // --- Raw Pointer Access (Use with Caution) --
std::byte* MemoryManager::getRawPointer(uintptr_t address) {
     if (address >= memory_block_.size()) return nullptr;
     return memory_block_.data() + address;
 }
 const std::byte* MemoryManager::getRawPointer(uintptr_t address) const {
      if (address >= memory_block_.size()) return nullptr;
     return memory_block_.data() + address;
 }
 size_t MemoryManager::getUsedSize() const {
    // For bump allocator, this is just the next offset
    return next_allocation_offset_;
 }
 } // namespace bdi::runtime
