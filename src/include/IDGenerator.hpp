#pragma once

#include <atomic>

using IDCode = uint32_t;

class ID {
    friend struct std::hash<ID>;
private:
    static std::atomic_uint32_t nextID;

    IDCode id;
    explicit ID(IDCode id) : id(id) {}

public:
    const static IDCode INVALID_ID = 0;
    ID(): ID(generate()) { }
    ID(const ID&)= delete;
    ID& operator=(const ID&)= delete;

    ID(ID&& other) noexcept : id(other.id) {}
    ID& operator=(ID&& other) noexcept {
        if (this != &other) {
            id = other.id;
        }
        return *this;
    }

    bool operator==(const ID& other) const { return id == other.id; }
    bool operator!=(const ID& other) const { return id != other.id; }
    bool operator<(const ID& other) const { return id < other.id; }
    bool operator<=(const ID& other) const { return id <= other.id; }
    bool operator>(const ID& other) const { return id > other.id; }
    bool operator>=(const ID& other) const { return id >= other.id; }

    bool isValid() const { return id != 0; }

    auto getCode() const -> IDCode { return id; }

public:
    static ID generate() {
        return ID(++nextID);
    }

    static void reset() {
        nextID = 0;
    }

    static IDCode peekNextID() {
        return nextID.load();
    }
};

namespace std {
    template <>
    struct hash<ID> {
        size_t operator()(const ID& id) const {
            return std::hash<IDCode>()(id.id);
        }
    };
}