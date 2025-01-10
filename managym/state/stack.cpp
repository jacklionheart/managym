// stack.cpp
#include "stack.h"

#include <format>

void Stack::push(Card* card) {
  objects.push_back(card);
  Zone::enter(card);
}

Card* Stack::pop() {
  Card* card = objects.back();
  objects.pop_back();
  Zone::exit(card);
  return card;
}

Card* Stack::top() { return objects.back(); }
