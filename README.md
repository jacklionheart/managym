# managym   

managym is an reinforcement learning environment for the game of Magic: The Gathering.

It is built to be used with [manabot](https://github.com/jacklionheart/manabot).

# Codebase Areas

The major areas of the codebase are:

- `managym/state/`: All of the game state. The most foundational package. Should have minimal dependencies on other parts of the codebase.
- `managym/flow/`: Executes the flow of game, including the core game loop, turn structure, and combat.
- `managym/agent`: The interface and execution of actions, i.e. making choices in gameplay and the execution of those choices.
- `managym/ui/`: Some basic code for rendering the game. Mostly for debugging.
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

Generally, this codebase aspires to use as little of C++ as possible. We are currently avoiding:

- Multiple inheritance
- Operator overloading (beyond ==)
- Templates and metaprogramming
- Macros
- Manual memory management / reference counting
- dynamic_cast
- std::shared_ptr