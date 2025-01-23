


# managym   

managym is an reinforcement learning environment for the game of Magic: The Gathering.

It is built to be used with [manabot](https://github.com/jacklionheart/manabot).

# Build & Install

```zsh
# Install in dev mode
# TODO: Dependency management
pip install -e .
```

See CMakeLists.txt for more details.

## Tests

```zsh
# Run all tests
mkdir -p build && cd build
cmake ..
make run_tests

# Run specific C++ tests 
cd build
./managym_test --gtest_filter=TestRegex.* --log=priority,turn,test 
```

### Test Options

```
- `--gtest_filter=<pattern>`: Run tests matching pattern  
- `--gtest_list_tests`: List available tests
- `--log=<cat1,cat2>`: Enable logging categories
  - priority, turn, state, rules, combat, agent, test
```

# Codebase Areas

API:
- `managym/agent`: Defines how agents (like manabot) can interact with the game.
The literal Python API used by manabot is defined in `managym/agent/pybind.cpp`.

Core game logic:
- `managym/state/`: Game state tracking and mutations.
- `managym/flow/`: Executes the flow of game, including the core game loop, turn structure, and combat.

Additional areas:
- `managym/ui/`: Some basic code for rendering the game. Mostly for debugging.
- `managym/infra/`: Currently just logging.
- `managym/cardsets`: Implementations of individual Magic cards that can be used in the game. 

# Style Guide

Use the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards) as a suggestion for anything unspecified here html.

## Formatting

Objects are `CamelCase`. Member variables are `snake_case`. Methods are `camelCase`. Files are `snake_case`. 
Pointers are `Object* ptr`, not `Object *ptr`. 

Indentations are 4 spaces. 

Includes should be sorted in group order, with spaces between the groups:

- Corresponding header: #include "me.h"
- Headers in same directory: #include "sibling.h"
- Any other managym headers: #include "managym/other.h"
- 3rd Party: #include <3rdparty.h>
- Built-ins: #include <spdlog/spdlog.h>

Prefer alphabetical within each indentation groups.

## Objects

Each object has one owner, which stores it in a std::unique_ptr<T>. Other references are simple T* pointers. We do not use std::shared_ptr or std::weak_ptr. Objects should be declared as `struct` unless access control is used. Classes in headers may be forward-declared to avoid unnecessary imports.

Simple subobjects which are not referenced by other objects are stored directly as member variables.

## Comments

Comments use // rather than /* */.

When a comment starts with MR405.1 that is a reference to the section 405.1 of the Magic Rules. A searchable version of the Magic Rules can be found at [yawgatog](https://yawgatog.com/resources/magic-rules/). 

Public APIs should have concise explanations with an emphasis on clarity rather than exhaustiveness. These comments should live in the cpp file.

In both header and cpp files, include sectional headers for e.g. the following major areas, with further subdivision as appropriate, in the following order:

// Data
// Reads
// Writes

The first line in a struct or class should be its constructor.

General overviews should appear as class comments. Header comments should be 1-line summaries of each methods. Most code should speak for itself, but organizational comments are welcome.

## C++

Generally, this codebase aspires to use as little of C++ as possible. AVOID:

- Multiple inheritance
- Operator overloading (beyond ==)
- Templates and metaprogramming
- Macros
- Manual memory management / reference counting
- dynamic_cast
- std::shared_ptr
- `auto` typing