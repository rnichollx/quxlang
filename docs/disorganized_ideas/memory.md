# Memory

## Allocator

### Layer 0 - Per Thread Stack

The Layer 0 allocator consists of a per-thread stack of memory allocation pointers per size class.

Layer 0 allocators consist of three array pointers per size class:

  - layer0_active_alloc
  - layer0_active_start
  - layer0_active_end

When allocation or deallocation from layer 0 cannot proceed because the active plate is empty or full respectively, the layer 1 exchange algorithm is activated.

### Layer 1 - Per Thread Active-Secondary Element Exchange

Layer 1 uses a stack to store recent allocations. The active plate can contain any number of allocations. Each thread also stores a secondary plate, however, the secondary plate can only be in one of three states: Full, half, or empty. When the active plate needs refill or empty, it first attempts to push or pull 1/2 of a full plate of elements from the secondary plate. This design intentionally biases the active plate towards a half-full state.

#### Layer 1 - Allocation operations:

| Secondary State    | Operation                                                |
|--------------------|----------------------------------------------------------|
| Case 1: Full       | Move half the secondary allocations to the active plate. |
| Case 2: Half-empty | Exchange active and secondary plate pointers             |
| Case 3: Empty      | Perform Layer 2 exchange, then proceed with case 1       |

#### Layer 1 - Deallocation opertions

| Secondary State    | Operation                                                               |
|--------------------|-------------------------------------------------------------------------|
| Case 1: Full       | Perform layer 2 exchange, then proceed with case 3.                     |
| Case 2: Half-empty | Exchange active and secondary plate pointers                            |
| Case 3: Empty      | Move half the allocations from the active plate to the secondary plate. |

### Layer 2 - Global Plate Exchange

Layer 2 is the first global, non thread-local layer, and consists of a set of plates to be exchanged between threads. This layer consists of empty plates organized by size, plus full plates, organized by allocation class and size.

Conceptully, the typical workflow consists of taking a full or empty plate and exchanging it for one of the alternative occupancy, this usually consists of only a few modifications to the global state.

The plate-exchange algorithm is intended to minimize time spent holding global locks. For example, in the 16 byte size-class case, the default plate configuration can hold 256 allocations, meaning a single empty to full plate pointer exchange can prepare the receving thread for an additional 256 allocations before it has to retake any global locks.

### Layer 3 - Allocation Regions Manager

Occasionally the number of full or empty plates stored in the global plate exchange may grow too large or few for the configuration of the allocator. Layer 3 consists of the allocation region manager.

An allocation region stores metadata at the start of an allocation region, and subsequent data within the region stores the allocations within that region.

#### Layer 3 - In-Region Allocator

At the start of each allocation region, an alloc semaphore can be incremented to attempt to allocate from a region, followed by atomically setting bitmasks according to the address(es) which ought to be allocated.

#### Layer 3 - In-Region Deallocator

The in-region deallocator must likewise mutate the value of the region control semaphore, then atomically clear relevant allocation bitmasks.

### Layer 4 - Allocation Region Control Layer

The allocation region control layer accounts for allocating and freeing allocation regions. At layer 3 and below, allocation are typically typed according to specific size classes, at layer 4, allocation regions themselves are re-used accross multiple size classes. Layer 4 acts on a global stack of allocation regions, allowing freed allocation regions to be repurposed as another size class.

### Layer 5 - Mapping Control Layer

The mapping control layer is the final layer between the application and the operating system. It manages the mapping of pages and regions.