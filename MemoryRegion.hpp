 // File: bdi/runtime/MemoryRegion.hpp
 #ifndef BDI_RUNTIME_MEMORYREGION_HPP
 #define BDI_RUNTIME_MEMORYREGION_HPP
 #include "../core/types/BDITypes.hpp" // For RegionID
 #include <cstddef> // For size_t
 #include <cstdint> // For uintptr_t
 namespace bdi::runtime {
 using bdi::core::types::RegionID;
 // Basic descriptor for an allocated memory region
 struct MemoryRegion {
    RegionID id = 0;
    uintptr_t base_address = 0; // Offset within the MemoryManager's simulated space
    size_t size = 0;
    bool read_only = false; // Example property
    // Add other properties: executable, persistence type, owning agent ID, etc.
    MemoryRegion(RegionID r_id, uintptr_t base, size_t sz, bool ro = false)
        : id(r_id), base_address(base), size(sz), read_only(ro) {}
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_MEMORYREGION_HPP
