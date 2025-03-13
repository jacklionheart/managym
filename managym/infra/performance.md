# Performance Optimization Design: managym

## 1. Introduction

This document outlines a strategic approach to improving the performance of managym, focusing on environment step speed in CPU-based, multi-threaded contexts. Based on profiling data and research into high-performance RL environments, we've identified several key optimization opportunities:

1. **Cache-friendly data structures** to maximize locality and reduce memory access latency
2. **Object pooling** to eliminate excessive allocation/deallocation cycles
3. **Profiler enhancements** to measure and validate improvements
4. **Core architectural changes** that provide a foundation for further optimizations

The design prioritizes changes that will yield the most significant performance improvements while maintaining compatibility with the existing codebase.

## 2. Current Performance Bottlenecks

From profiling data, we've identified several key bottlenecks:

1. **Observation Construction**: Creating a new Observation per step (using std::map) incurs significant allocation overhead
2. **Zone Operations**: Map-based lookups and frequent pointer indirection in zone management
3. **Object Creation/Destruction**: Frequent allocation of Actions and other game objects
4. **Redundant Traversals**: Multiple iterations over the same data during state preparation

## 3. Optimization Strategies

### 3.1 Cache-Friendly Data Structures

#### 3.1.1 Replace Maps with Vector-Based Containers in Observation

**Current Implementation:**
```cpp
// observation.h
std::map<int, CardData> agent_cards;
std::map<int, CardData> opponent_cards;
std::map<int, PermanentData> agent_permanents;
std::map<int, PermanentData> opponent_permanents;
```

**Proposed Implementation:**
```cpp
// observation.h
struct CardDataEntry {
    int id;
    CardData data;
};

struct PermanentDataEntry {
    int id;
    PermanentData data;
};

std::vector<CardDataEntry> agent_cards;
std::vector<CardDataEntry> opponent_cards;
std::vector<PermanentDataEntry> agent_permanents;
std::vector<PermanentDataEntry> opponent_permanents;
```

**Population Method Updates:**
```cpp
void Observation::populateCards(const Game* game) {
    for (const Card* card : hand->cards.at(agent_player)) {
        CardDataEntry entry;
        entry.id = static_cast<int>(card->id);
        // Populate entry.data based on card
        agent_cards.push_back(std::move(entry));
    }
    // Similarly process opponent's cards
}
```

**Lookup Utilities:**
```cpp
const CardData* Observation::findAgentCard(int cardId) const {
    for (const auto& entry : agent_cards) {
        if (entry.id == cardId) return &entry.data;
    }
    return nullptr;
}
```

#### 3.1.2 Optimize Zone Storage with Vector-of-Vectors

**Current Implementation:**
```cpp
// zone.h
std::map<const Player*, std::vector<Card*>> cards;
```

**Proposed Implementation:**
```cpp
// zone.h
std::vector<std::vector<Card*>> cards;
```

**Zone Construction:**
```cpp
Zone::Zone(const std::vector<Player*>& players, Zones* zones) : zones(zones) {
    cards.resize(players.size());
}
```

**Method Updates:**
```cpp
void Zone::enter(Card* card) {
    int idx = card->owner->index;  // Using stable index
    cards[idx].push_back(card);
}
```

#### 3.1.3 Optimize Card-to-Zone Mapping

**Current Implementation:**
```cpp
// zones.h
std::map<Card*, Zone*> card_to_zone;
```

**Proposed Implementation:**
```cpp
// zones.h
std::vector<Zone*> card_to_zone_lookup;
```

**Usage Updates:**
```cpp
void Zones::move(Card* card, ZoneType toZone) {
    if (card->id >= card_to_zone_lookup.size()) {
        card_to_zone_lookup.resize(card->id + 1, nullptr);
    }
    
    Zone* newZone = getZone(toZone);
    Zone* oldZone = card_to_zone_lookup[card->id];
    
    if (oldZone) {
        oldZone->exit(card);
    }
    
    newZone->enter(card);
    card_to_zone_lookup[card->id] = newZone;
}
```

### 3.2 Object Pooling

#### 3.2.1 Observation Pool

**Implementation:**
```cpp
// observation_pool.h
class ObservationPool {
public:
    ObservationPool(size_t poolSize) {
        for (size_t i = 0; i < poolSize; i++) {
            pool.push(std::make_unique<Observation>());
        }
    }

    std::unique_ptr<Observation> acquire() {
        if (pool.empty()) {
            return std::make_unique<Observation>();
        }
        auto obj = std::move(pool.front());
        pool.pop();
        return obj;
    }

    void release(std::unique_ptr<Observation> obj) {
        // Reset the object to initial state
        obj->reset();
        pool.push(std::move(obj));
    }

private:
    std::queue<std::unique_ptr<Observation>> pool;
};
```

**Integration with Environment:**
```cpp
// env.h
class Env {
private:
    // Previous members
    std::unique_ptr<ObservationPool> observation_pool;
    
public:
    Env(int seed = 0, bool skip_trivial = false) 
      : skip_trivial(skip_trivial), seed(seed) {
        observation_pool = std::make_unique<ObservationPool>(10);
        // Other initialization
    }
    
    // Method implementations using the pool
};

// env.cpp
std::pair<Observation*, std::map<std::string, std::string>>
Env::reset(const std::vector<PlayerConfig>& player_configs) {
    // Destroy any old game
    game.reset(new Game(player_configs, skip_trivial, profiler.get()));

    // Get observation from pool and populate it
    std::unique_ptr<Observation> obs = observation_pool->acquire();
    obs->populateFromGame(game.get());
    
    current_observation = std::move(obs);
    
    // Rest of method
}
```

#### 3.2.2 Action Pool

**Implementation:**
```cpp
// action_pool.h
template <typename T>
class ActionPool {
public:
    ActionPool(size_t poolSize) {
        for (size_t i = 0; i < poolSize; i++) {
            pool.push(std::make_unique<T>());
        }
    }

    template <typename... Args>
    std::unique_ptr<T> acquire(Args&&... args) {
        if (pool.empty()) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        auto obj = std::move(pool.front());
        pool.pop();
        
        // Initialize the object with the new arguments
        obj->initialize(std::forward<Args>(args)...);
        return obj;
    }

    void release(std::unique_ptr<T> obj) {
        pool.push(std::move(obj));
    }

private:
    std::queue<std::unique_ptr<T>> pool;
};

// For specific action types
using PassPriorityPool = ActionPool<PassPriority>;
using PlayLandPool = ActionPool<PlayLand>;
using CastSpellPool = ActionPool<CastSpell>;
```

**Integration with Action Creation:**
```cpp
// priority.cpp
std::vector<std::unique_ptr<Action>> PrioritySystem::availablePriorityActions(Player* player) {
    std::vector<std::unique_ptr<Action>> actions;
    
    // Add pass priority action (reused from pool)
    actions.push_back(pass_priority_pool->acquire(player, game));
    
    // Rest of method
}
```

### 3.3 Profiler Enhancements

#### 3.3.1 Memory Profiling Capabilities

**Implementation:**
```cpp
// profiler.h
struct MemoryStats {
    size_t bytes_allocated;
    size_t bytes_freed;
    int allocation_count;
    int deallocation_count;
};

class Profiler {
public:
    // Existing code...
    
    // Track memory allocation
    void trackAllocation(size_t bytes) {
        memory_stats.bytes_allocated += bytes;
        memory_stats.allocation_count++;
    }
    
    // Track memory deallocation
    void trackDeallocation(size_t bytes) {
        memory_stats.bytes_freed += bytes;
        memory_stats.deallocation_count++;
    }
    
    // Get memory statistics
    MemoryStats getMemoryStats() const {
        return memory_stats;
    }
    
private:
    // Existing code...
    MemoryStats memory_stats = {0, 0, 0, 0};
};
```

#### 3.3.2 Allocation Tracking

**Custom Allocator Implementation:**
```cpp
// memory_tracking.h
template <typename T>
class TrackedAllocator {
public:
    using value_type = T;
    
    TrackedAllocator(Profiler* profiler) : profiler(profiler) {}
    
    template <typename U>
    TrackedAllocator(const TrackedAllocator<U>& other) : profiler(other.profiler) {}
    
    T* allocate(size_t n) {
        size_t size = n * sizeof(T);
        T* ptr = static_cast<T*>(::operator new(size));
        if (profiler) {
            profiler->trackAllocation(size);
        }
        return ptr;
    }
    
    void deallocate(T* p, size_t n) {
        if (profiler) {
            profiler->trackDeallocation(n * sizeof(T));
        }
        ::operator delete(p);
    }
    
private:
    Profiler* profiler;
};
```

#### 3.3.3 Per-Object Type Profiling

**Implementation:**
```cpp
// profiler.h
enum class ObjectType {
    OBSERVATION,
    ACTION,
    CARD,
    PERMANENT,
    OTHER
};

struct ObjectTypeStats {
    int created;
    int destroyed;
};

class Profiler {
public:
    // Existing code...
    
    // Track object creation
    void trackObjectCreation(ObjectType type) {
        object_stats[type].created++;
    }
    
    // Track object destruction
    void trackObjectDestruction(ObjectType type) {
        object_stats[type].destroyed++;
    }
    
    // Get object statistics
    std::map<ObjectType, ObjectTypeStats> getObjectStats() const {
        return object_stats;
    }
    
private:
    // Existing code...
    std::map<ObjectType, ObjectTypeStats> object_stats;
};
```

### 3.4 Core Architectural Changes

#### 3.4.1 Reusable Observation Pattern

**Implementation:**
```cpp
// observation.h
class Observation {
public:
    // Existing members...
    
    // Reset the observation to an empty state
    void reset() {
        game_over = false;
        won = false;
        agent_cards.clear();
        opponent_cards.clear();
        agent_permanents.clear();
        opponent_permanents.clear();
        // Reset other fields
    }
    
    // Update from game rather than constructing from scratch
    void populateFromGame(const Game* game) {
        // Clear previous state or reuse existing containers
        reset();
        
        // Populate with new data
        populateTurn(game);
        populateActionSpace(game);
        populatePlayers(game);
        populateCards(game);
        populatePermanents(game);
    }
    
    // Existing methods...
};
```

#### 3.4.2 Optimized Action Space Creation

**Implementation:**
```cpp
// action_space.h
class ActionSpace {
public:
    // Existing code...
    
    // Reserve space for a known number of actions
    void reserveActionCapacity(size_t capacity) {
        actions.reserve(capacity);
    }
    
    // Add an action without allocating
    void addAction(std::unique_ptr<Action> action) {
        actions.push_back(std::move(action));
    }
};

// Adding a method to pre-allocate common actions
ActionSpace* PrioritySystem::createPriorityActionSpace(Player* player) {
    // Create action space with pre-reserved capacity
    auto action_space = std::make_unique<ActionSpace>(ActionSpaceType::PRIORITY, player);
    action_space->reserveActionCapacity(player->hand.size() + 1); // +1 for pass
    
    // Add pass action (from pool)
    action_space->addAction(pass_priority_pool->acquire(player, game));
    
    // Rest of implementation
    return action_space.release();
}
```

#### 3.4.3 Iterate-Once Observation Building

**Implementation:**
```cpp
// observation.cpp
void Observation::populateFromGame(const Game* game) {
    reset();
    
    // Basic flags
    game_over = game->isGameOver();
    if (game_over) {
        int winner_idx = game->winnerIndex();
        won = (winner_idx != -1 && game->players[winner_idx].get() == game->agentPlayer());
    }
    
    // Identify agent and opponent
    const Player* agent_player = game->agentPlayer();
    const Player* opponent_player = nullptr;
    
    for (const auto& p : game->playersStartingWithAgent()) {
        if (p != agent_player) {
            opponent_player = p;
            break;
        }
    }
    
    // Setup player data
    agent.id = agent_player->id;
    agent.player_index = agent_player->index;
    agent.is_agent = true;
    agent.is_active = (agent_player == game->activePlayer());
    agent.life = agent_player->life;
    
    opponent.id = opponent_player->id;
    opponent.player_index = opponent_player->index;
    opponent.is_agent = false;
    opponent.is_active = (opponent_player == game->activePlayer());
    opponent.life = opponent_player->life;
    
    // Single pass to populate zone counts and collect objects
    populateAllFromZones(game, agent_player, opponent_player);
    
    // Turn and action space
    populateTurn(game);
    populateActionSpace(game);
}

void Observation::populateAllFromZones(const Game* game, const Player* agent_player, const Player* opponent_player) {
    // Reset zone counts
    for (int i = 0; i < 7; i++) {
        agent.zone_counts[i] = 0;
        opponent.zone_counts[i] = 0;
    }
    
    // Single traversal of all zones to populate everything
    for (int zone_idx = 0; zone_idx < 7; zone_idx++) {
        ZoneType zone = static_cast<ZoneType>(zone_idx);
        
        // Count objects in zone
        agent.zone_counts[zone_idx] = game->zones->size(zone, agent_player);
        opponent.zone_counts[zone_idx] = game->zones->size(zone, opponent_player);
        
        // Skip hidden information for opponent
        bool include_opponent_cards = (zone != ZoneType::HAND);
        
        // Collect objects from zone
        if (zone == ZoneType::BATTLEFIELD) {
            // Special handling for battlefield (has permanents)
            for (const auto& perm : game->zones->constBattlefield()->permanents.at(agent_player)) {
                addPermanent(perm.get());
                addCard(perm->card, zone);
            }
            
            if (include_opponent_cards) {
                for (const auto& perm : game->zones->constBattlefield()->permanents.at(opponent_player)) {
                    addPermanent(perm.get());
                    addCard(perm->card, zone);
                }
            }
        } else {
            // Standard zone traversal for cards
            game->zones->forEach(
                [&](Card* card) { addCard(card, zone); },
                zone, const_cast<Player*>(agent_player));
            
            if (include_opponent_cards) {
                game->zones->forEach(
                    [&](Card* card) { addCard(card, zone); },
                    zone, const_cast<Player*>(opponent_player));
            }
        }
    }
}
```

## 4. Implementation Roadmap

### Phase 1: Profiler Enhancements
1. Extend Profiler with memory tracking capabilities
2. Implement custom allocator for tracking
3. Add object creation/destruction tracking by type
4. Create benchmarks to establish baseline performance

### Phase 2: Observation Refactor
1. Replace maps with vectors in Observation
2. Implement findCard helper methods
3. Design and implement reusable Observation pattern
4. Add single-pass zone traversal

### Phase 3: Object Pool Integration
1. Implement ObservationPool
2. Implement ActionPool for common action types
3. Update Env to use ObservationPool
4. Update action creation code to use ActionPool

### Phase 4: Zone Structure Refactor
1. Convert Zone::cards from map to vector-of-vectors
2. Implement card_to_zone_lookup vector
3. Update Zone methods to use player indices
4. Optimize card moving operations

### Phase 5: Measurement and Iteration
1. Conduct comprehensive profiling with improved profiler
2. Identify remaining hotspots
3. Apply targeted optimizations to critical paths
4. Compare against baseline benchmarks

## 5. Expected Outcomes

The proposed optimizations are expected to yield several key improvements:

1. **Reduced Allocation Overhead**: Object pooling and container reuse should dramatically decrease dynamic memory allocations per step.

2. **Improved Cache Locality**: Vector-based storage will result in better spatial locality, improving CPU cache utilization.

3. **Lower Memory Footprint**: More efficient data structures will reduce the total memory requirements.

4. **Faster Step Execution**: Combined optimizations should significantly increase steps per second, potentially by 2-5x depending on scenario complexity.

5. **Better Scalability**: The optimized architecture will scale better with increasing game complexity and in multi-threaded contexts.

## 6. Risks and Considerations

1. **Card ID Assumptions**: The vector-based lookup assumes card IDs are relatively small, contiguous integers. If IDs use a different strategy, we may need to adjust our approach.

2. **Data Access Patterns**: Switching from maps to vectors changes the O(log n) lookup to O(n) for unsorted vectors. For small collections, this is faster due to cache locality, but we should validate with benchmarks.

3. **Object Pool Sizing**: Initial pool sizes need to be carefully chosen based on typical usage patterns.

4. **Testing Coverage**: Comprehensive testing is essential to ensure optimization changes don't introduce subtle bugs.

## 7. Conclusion

This performance optimization plan addresses fundamental efficiency bottlenecks in managym by adopting cache-friendly data structures, implementing object pooling, enhancing profiling capabilities, and making strategic architectural changes. The phased implementation approach allows for incremental improvement and validation.

By following this plan, we expect to achieve a significant reduction in per-step overhead and improved overall throughput, making managym better suited for large-scale RL training scenarios.