# managym

managym is a reinforcement learning environment for Magic: The Gathering, built to be used with [manabot](https://github.com/jacklionheart/manabot).

managym is written in C++ but uses pybind11 to create a python library `managym`.

```
python
import managym
env = managym.Env()
...
```

## Installation

```zsh
# Install in dev mode
pip install -e .
```

## Architecture 

The codebase is organized into the following key areas:

1. `managym/agent/`: Core API for RL agents to interact with the game. Defines observation and action spaces. The pybind11 interface used by manabot lives in `managym/agent/pybind.cpp`.

2. `managym/flow/`: Game state progression including the turn system, priority system, and combat phases. Handles core game loop and rule enforcement.

3. `managym/state/`: Game state tracking and data structures. Manages cards, players, permanents, and the zones system.

4. `managym/cardsets/`: Card implementations and registry. Currently focused on basic lands and simple creatures. 

5. `managym/infra/`: Core infrastructure, currently just logging.

Dependencies flow downward: agent → flow → state → infra. (cardsets is a bit messy right now)

## Testing

```zsh
# Run all tests
mkdir -p build && cd build
cmake ..
make run_tests

# Run specific tests
./managym_test --gtest_filter=TestRegex.* --log=priority,turn,test
```

Test options:
- `--gtest_filter=<pattern>`: Run tests matching pattern
- `--gtest_list_tests`: List available tests  
- `--log=<cat1,cat2>`: Enable logging categories
  - priority, turn, state, rules, combat, agent, test

## Style Guide

### Code Organization

cpp files should follow this template:

```cpp
// filename.h/.cpp
// One-line purpose of file
//
// ------------------------------------------------------------------------------------------------
// EDITING INSTRUCTIONS:
// Instructions for collaborators (both human and LLM) on how to approach editing.
// Keep these focused and impactful.
// ------------------------------------------------------------------------------------------------ 

// Corresponding header to this .cpp filen
#include "me.h"

// Headers in same directory
#include "sibling.h"

// Any other managym headers  
#include "managym/other.h"

// 3rd Party 
#include <3rdparty.h>

// Built-ins
#include <std>
```

Header files should have comments on each public method, but most data fields should be self-explantory.

### Objects

Objects should be declared as `struct` unless access control is needed. Each object has exactly one owner, which stores it in a `std::unique_ptr<T>`. Other references are raw pointers (`T*`). This ensures:
- Clear and reliable memory management 
- Consistent access patterns throughout the codebase
- Compile-time errors for accidental copies

Simple subobjects which are not referenced by other objects should be stored directly as member variables.

### Error Handling

Exceptions serve two distinct purposes:
1. `AgentError`: Thrown for invalid API usage or bad parameters. These represent user errors and should be handled gracefully.
2. Other exceptions: Thrown for internal errors, invariant violations, or unrecoverable states. These represent bugs in managym.

### Documentation 

Comments use // rather than /* */. Each file should have a one-line summary and optional editing instructions at the top.

When a comment starts with MR405.1 that references section 405.1 of the Magic Rules. A searchable version can be found at [yawgatog](https://yawgatog.com/resources/magic-rules/).

Public APIs should have clear, concise explanations focused on behavior and edge cases. Organizational comments are welcome but most code should speak for itself.

### DO NOT USE

LITERARLLY NEVER DO ANY OF THIS:
- std::shared_ptr
- Templates and metaprogramming
- Macros
- Manual memory management / reference counting
- dynamic_cast
- Multiple inheritance

STRONGLY AVOID USING:
- auto typing
- Implementiation of methods in header files
- Operator overloading (beyond ==)  

### Instructions for LLM Codegen

When working with this codebase, LLMs should:
- Avoid comments that are transient/denote changes. Imagine the code will be directly copied into the codebase for eternity.
- Pay special attention to file headers and README content for context
- Propose small, iterative changes
- End responses with:
  1. Full implementations of changed files that can be copied into the codebase 
  2. Questions that could clarify intent
  3. Notes on what was intentionally left out
  