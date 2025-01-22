#pragma once

using ObjectId = int;

class IDGenerator {
public:
    IDGenerator() = default;
    ObjectId next() { return next_id++; }

private:
    ObjectId next_id = 0;
};

struct GameObject {
    ObjectId id;

    explicit GameObject(ObjectId id) : id(id) {}
    virtual ~GameObject() = default;
};