#pragma once

#include "managym/state/zone.h"

#include <memory>
#include <vector>

class Game; // Forward declaration
class Card;
class Zones;

// `Stack` adds stack access patterns to `Zone`
// Stacks will eventually be able to contain triggered and activated abilities
// in addition to cards.
class Stack : public Zone {
public:
    using Zone::Zone;

    // Data
    std::vector<Card*> objects;

    // Reads

    // Get the most recently added object without removing it
    Card* top();

protected:
    friend class Zones;

    // Writes

    // Add a new object to the stack
    void push(Card* card);
    // Remove and return the top object from the stack
    Card* pop();
};