#pragma once
// info_dict.h
// Generic nested info dictionary
//
// ------------------------------------------------------------------------------------------------
// This file defines InfoValue and InfoDict types that can store either a string, an int,
// a float, or a nested InfoDict. Utility functions are provided for creating, inserting,
// and converting the dictionary to a JSON–like string.
// Additionally, free inline helper functions are provided to simulate a map-like API.
// ------------------------------------------------------------------------------------------------ 

#include <map>
#include <string>
#include <variant>
#include <stdexcept>

// Forward declaration of InfoValue.
struct InfoValue;

// InfoDict is a typedef for a map from strings to InfoValue.
typedef std::map<std::string, InfoValue> InfoDict;

struct InfoValue {
    // Variant can hold: string, nested dict, int, or float.
    std::variant<std::string, InfoDict, int, float> value;

    // Constructors
    InfoValue();
    InfoValue(const std::string & s);
    InfoValue(const InfoDict & dict);
    InfoValue(int i);
    InfoValue(float f);
};

/// @brief Creates an empty InfoDict.
InfoDict create_empty_info_dict();

/// @brief Inserts a string value into the dictionary.
void insert_info(InfoDict & dict, const std::string & key, const std::string & value);

/// @brief Inserts a nested dictionary into the dictionary.
void insert_info(InfoDict & dict, const std::string & key, const InfoDict & value);

/// @brief Inserts an integer value into the dictionary.
void insert_info(InfoDict & dict, const std::string & key, int value);

/// @brief Inserts a float value into the dictionary.
void insert_info(InfoDict & dict, const std::string & key, float value);

/// @brief Recursively converts an InfoDict to a JSON–like string.
std::string info_dict_to_string(const InfoDict & dict);

//
// Free helper functions to simulate a map-like API.
//

/// @brief Returns true if the key exists in the dictionary.
inline bool dict_contains(const InfoDict & dict, const std::string & key) {
    return dict.find(key) != dict.end();
}

/// @brief Erases the key (and its value) from the dictionary.
inline void dict_erase(InfoDict & dict, const std::string & key) {
    dict.erase(key);
}

/// @brief Returns the number of key-value pairs in the dictionary.
inline size_t dict_size(const InfoDict & dict) {
    return dict.size();
}

/// @brief Gets a reference to the value associated with the key. Throws if key is not found.
inline InfoValue& dict_get(InfoDict & dict, const std::string & key) {
    auto it = dict.find(key);
    if (it == dict.end()) {
        throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
}

/// @brief Gets a const reference to the value associated with the key. Throws if key is not found.
inline const InfoValue& dict_get(const InfoDict & dict, const std::string & key) {
    auto it = dict.find(key);
    if (it == dict.end()) {
        throw std::out_of_range("Key not found: " + key);
    }
    return it->second;
}

/// @brief Updates dict by inserting all key-value pairs from other (overwriting keys if needed).
inline void dict_update(InfoDict & dict, const InfoDict & other) {
    for (const auto & kv : other) {
        dict[kv.first] = kv.second;
    }
}

/// @brief Clears all entries from the dictionary.
inline void dict_clear(InfoDict & dict) {
    dict.clear();
}
