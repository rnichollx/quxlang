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

An allocation region stores a header and an allocation bitmap before its payload slots. The header identifies the region's control block and allocation class. Layer 3 acquires temporary access through the control block before allocating or returning slots.

#### Layer 3 - Region Metadata

The region header contains the following metadata:

| Field | Description |
|-------|-------------|
| `region_control_pointer` | Points to the independently stored allocation region control block. |
| `class_control_pointer` | Points to immutable metadata describing the allocation class. This pointer remains unchanged while the region is assigned to that class. |
| `allocation_bitmap` | Contains atomic bitmap words identifying outstanding payload slots. Header storage and alignment padding are excluded from the allocatable slots. |
| `availability_state` | Contains an atomic value indicating whether the assigned region is advertised on the available list. The values are `AVAILABLE` and `UNAVAILABLE`. |

Each control block remains allocated for the allocator's lifetime and remains associated with one allocation class. Control blocks are allocated in fixed chunks whose addresses remain stable. The class's availability lists index individual control blocks independently of their placement within those chunks. An allocating thread can retain a control-block pointer while another thread retires or replaces the associated region. Control-block storage remains accessible after the region is returned to layer 4 or its backing is released by layer 5.

The control block contains the following fields:

| Field | Description |
|-------|-------------|
| `access_control_semaphore` | Contains an atomic unsigned semaphore value counting active shared layer-3 uses. The maximum representable value, `EXCLUSIVE`, denotes exclusive ownership. |
| `region_pointer` | Identifies the region currently assigned to the control block. A null pointer indicates that the control block has no assigned region. |
| `next` | Points to the next control block or sentinel in the containing availability list. |
| `prev` | Points to the previous control block or sentinel in the containing availability list. |

With a pointer-sized semaphore, these four fields occupy four machine words before alignment padding. The atomic availability state remains in the region header. Control-block storage chunks provide stable addresses and locality, while the intrusive links provide direct removal from availability lists.

Shared or exclusive ownership protects every access to `region_pointer`. The pointer changes only under exclusive ownership. The pointer and semaphore can therefore occupy separate fields without requiring a double-width atomic operation.

The semaphore describes active layer-3 operations. The bitmap describes slot ownership:

| Bit value | Slot ownership |
|-----------|----------------|
| `0` | The slot is available for allocation by layer 3. |
| `1` | The slot is outstanding outside layer 3, or has been claimed by a layer-3 operation preparing to publish it. |

An outstanding slot includes an allocation held by an application, a pointer cached in layers 0 through 2, or a pointer in transit between those layers and layer 3. Its bit remains set until layer 3 accepts the slot back. Allocation and deallocation within layers 0 through 2 do not update the region bitmap or semaphore. A zero semaphore count can coexist with outstanding slots.

#### Layer 3 - Availability Lists

Each allocation class owns one list mutex and three intrusive doubly linked lists with circular sentinel heads:

| List | Membership |
|------|------------|
| Available | Contains control blocks whose assigned regions are advertised as candidates for slot allocation. |
| Fully issued | Contains control blocks whose assigned regions have been withdrawn from allocation search after a protected check found every valid slot outstanding. |
| Detached | Contains control blocks whose region pointers are null and which can receive replacement backing from layer 4. |

Each published control block belongs to exactly one list except during a move protected by the list mutex. New control blocks are initialized with a null region pointer and a zero semaphore, then inserted into the detached list under that mutex. List sentinels identify list boundaries and are excluded from region selection.

Every read or modification of a list head, `next`, or `prev` requires the class's list mutex. A known control block can be unlinked by updating its immediate neighbors and inserted at another list's head in O(1) work. Circular sentinels allow the same operations for the first, last, and sole entries. Allocation discovery selects directly from the available list's head. Selection of a control block for replacement backing uses the detached list's head.

The list mutex is released before acquiring or waiting for shared or exclusive region access. A selecting thread copies only the stable control-block pointer while the mutex is held; it reads the region pointer after acquiring region access. Operations that change membership first acquire region access, then acquire the list mutex for the move. Bitmap scans, slot claims and returns, layer-4 handoffs, and OS mapping operations occur outside the list mutex.

The region bitmap remains authoritative for slot claims. Available-list membership advertises a candidate whose slots may be consumed by another thread before access is acquired. The fully issued list may temporarily contain a region with newly returned slots while the returning operation completes its publication. An availability transition updates the links and `availability_state` together under the list mutex. State updates use release semantics, and state reads outside the list mutex use acquire semantics while region access is held.

A transition from `AVAILABLE` to `UNAVAILABLE` requires exclusive region ownership and a current bitmap check. A return operation can perform the reverse transition while holding shared region access and the list mutex. This ordering allows a return to an already-advertised region to complete without acquiring the class mutex, while preventing a concurrent withdrawal from overlooking that return.

Candidate selection and list insertion, removal, and movement require O(1) index work. Region bitmap processing and synchronization retries have separate costs. The lists do not establish a bound on allocation latency under contention.

#### Layer 3 - Shared Access

A thread acquires one shared use by performing a compare-and-exchange from an observed count `n` to `n + 1`. The increment is permitted only when `n < EXCLUSIVE - 1`. Observing `EXCLUSIVE` or the maximum shared count does not permit an increment. A failed compare-and-exchange does not acquire shared ownership, and a retry uses the newly observed semaphore value.

A successful shared acquisition uses acquire semantics. The thread reads `region_pointer` only after acquisition. The thread completes all accesses to the region before releasing its shared use through an atomic decrement with release semantics. Successful exclusive acquisitions also use acquire semantics, and exclusive release stores zero with release semantics. These operations synchronize region initialization and completed shared accesses with subsequent ownership acquisitions.

An allocating thread selects a stable control block from its allocation class's available list under the list mutex, releases that mutex, and then acquires shared access. If the protected `region_pointer` is null, the thread releases its shared use and selects another control block. Each successful acquisition reads the current region pointer; a previously observed region address is not retained across shared uses.

A thread returning an outstanding allocation can obtain the control-block pointer from that allocation's region header. The allocation's set bitmap bit prevents retirement while the thread reads the header and acquires shared access. The bit remains set until shared access has been acquired and the slot is returned. The protected region pointer therefore continues to identify the region containing that valid outstanding allocation.

#### Layer 3 - In-Region Allocator

The in-region allocator operates while holding shared access. It selects clear bits representing payload slots and claims them with an atomic compare-and-exchange on the containing bitmap word. A successful claim uses acquire-release semantics and preserves unrelated bits. If the word changes before the claim succeeds, the allocator retries against the updated word. Only successfully claimed slots are published to a plate or another recipient.

One shared use can cover multiple slot claims from the same region. Every published slot has its bitmap bit set before the allocator releases shared access. If an operation abandons a claim before publishing the slot, it returns the claimed slot through the in-region deallocation protocol while access is still held, including publication of availability when required.

An allocator that observes no available slots can attempt exclusive acquisition through the control block. After successful acquisition, it rechecks the current region pointer. If the pointer is null, the owner releases exclusive ownership without accessing a region. Otherwise, it rechecks the bitmap. If every valid slot remains outstanding, the owner acquires the list mutex, moves the control block to the fully issued list if necessary, and stores `UNAVAILABLE` in the region's availability state. The owner releases the list mutex before releasing exclusive ownership. If the region has available slots, it remains eligible for allocation. A failed exclusive acquisition can leave a fully issued region temporarily advertised as available; subsequent operations revalidate the candidate.

#### Layer 3 - In-Region Deallocator

The in-region deallocator acquires shared access before returning slots. The caller owns every returned slot exclusively, including removal of its pointer from any cache or plate that could otherwise issue it. The deallocator clears the corresponding bits with atomic read-modify-write operations using release semantics and preserving unrelated bits. Slot claims that observe these cleared bits acquire the preceding return operation's writes.

After clearing the bits, the deallocator reads `availability_state` while retaining shared access. If the state is `AVAILABLE`, the return requires no list operation. If the state is `UNAVAILABLE`, the deallocator acquires the list mutex and rechecks the state. If the region remains unadvertised, the deallocator moves its control block from the fully issued list to the available list and stores `AVAILABLE`. It then releases the list mutex. Concurrent return operations that find the transition already completed perform no additional move. Publication finishes before shared access is released, so a completed return cannot leave newly available capacity hidden by an earlier fullness observation.

One shared use can cover multiple returns to the same region. Shared access protects the region while the deallocator finishes its metadata accesses, including after the last outstanding bit is cleared. The deallocator completes those accesses before releasing its shared use or successfully converting it to exclusive ownership.

#### Layer 3 - Exclusive Access and Retirement

A thread without shared ownership acquires exclusive ownership by performing a compare-and-exchange from zero to `EXCLUSIVE`. A thread that owns exactly one shared use can instead attempt a compare-and-exchange from one to `EXCLUSIVE`. A successful conversion consumes that shared use, so the thread subsequently releases only its exclusive ownership. A failed conversion leaves the thread responsible for its existing shared use.

Exclusive acquisition cannot overwrite a nonzero count belonging to other users. A thread holding shared access must either convert its sole remaining use or release that use before attempting acquisition from zero. The thread cannot wait for a zero count while retaining its own shared use.

Exclusive ownership prevents new shared acquisitions and establishes that no shared users remain. Retirement then proceeds as follows:

1. The exclusive owner reads `region_pointer`. If the pointer is null, the owner releases exclusive ownership without a region handoff.
2. The owner rechecks all valid bitmap entries. If any slot remains outstanding, the owner aborts retirement and releases exclusive ownership.
3. The owner saves the region address and all information required for the layer-4 handoff in storage outside the region.
4. The owner acquires the list mutex, sets the region's availability state to `UNAVAILABLE`, sets `region_pointer` to null, and moves the control block to the detached list. The owner then releases the list mutex while retaining exclusive region ownership.
5. The owner returns the saved region to layer 4 outside the list mutex. The handoff permits immediate reuse or release of the region's storage.
6. The owner releases exclusive ownership through the independently stored control block. The owner performs no further accesses to the returned region. A thread that selected the detached control block during the handoff acquires exclusive ownership before attempting to assign new backing.

An earlier empty-bitmap observation does not replace the exclusive recheck. Concurrent shared users may claim or return slots before exclusive acquisition succeeds. An unsuccessful retirement attempt leaves the region subject to subsequent retirement attempts under the allocator's reclamation policy.

### Layer 4 - Allocation Region Control Layer

The allocation region control layer accounts for allocating and freeing allocation regions. At layer 3 and below, allocation are typically typed according to specific size classes, at layer 4, allocation regions themselves are re-used accross multiple size classes. Layer 4 acts on a global stack of allocation regions, allowing freed allocation regions to be repurposed as another size class.

Layer 4 receives a region after its previous control block has been detached by setting that block's region pointer to null. The previous control block retains its original class association and lifetime. Reassigning the region to another class replaces the region header's control-block and class-control pointers with those of the destination class.

#### Layer 4 - Region Assignment and Publication

Assigning backing to a control block requires exclusive ownership of that block and a null `region_pointer`. The assigning thread selects a control block from the detached list under the list mutex and releases the mutex before acquiring exclusive ownership. It then rechecks the region pointer. If another thread has already assigned backing, the thread releases exclusive ownership and selects another detached control block. Otherwise, it obtains a region from layer 4 outside the list mutex and completes initialization before publishing it:

1. The thread initializes the region header with the destination control-block pointer and class-control pointer.
2. The thread establishes the destination class's payload layout, initializes every valid bitmap entry to zero, and initializes `availability_state` to `UNAVAILABLE`. Only payload slots represented by valid entries are eligible for allocation.
3. The thread acquires the list mutex, stores the initialized region address in the protected `region_pointer` field, moves the control block from the detached list to the available list, and stores `AVAILABLE` in the region's availability state.
4. The thread releases the list mutex, then releases exclusive ownership with release semantics.

A subsequent shared acquisition observes the initialized region through the control block. A thread delayed before acquisition therefore observes either a null pointer or a currently assigned region of the same class. The class association of the control block remains unchanged even when successive backing regions have different addresses.

### Layer 5 - Mapping Control Layer

The mapping control layer is the final layer between the application and the operating system. It manages the mapping of pages and regions.
