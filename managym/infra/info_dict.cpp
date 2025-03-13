#include "info_dict.h"
#include <sstream>

// ---------- InfoValue Implementation ----------

InfoValue::InfoValue() : value(std::string("")) {}

InfoValue::InfoValue(const std::string & s) : value(s) {}

InfoValue::InfoValue(const InfoDict & dict) : value(dict) {}

InfoValue::InfoValue(int i) : value(i) {}

InfoValue::InfoValue(float f) : value(f) {}

// ---------- Utility Functions ----------

InfoDict create_empty_info_dict() {
    InfoDict dict;
    return dict;
}

void insert_info(InfoDict & dict, const std::string & key, const std::string & value) {
    dict[key] = InfoValue(value);
}

void insert_info(InfoDict & dict, const std::string & key, const InfoDict & value) {
    dict[key] = InfoValue(value);
}

void insert_info(InfoDict & dict, const std::string & key, int value) {
    dict[key] = InfoValue(value);
}

void insert_info(InfoDict & dict, const std::string & key, float value) {
    dict[key] = InfoValue(value);
}

// ---------- Recursive Conversion to JSON-like String ----------

static std::string indent_string(int indent) {
    std::string result;
    for (int i = 0; i < indent; i++) {
        result += "  ";
    }
    return result;
}

static void info_dict_to_string_recursive(const InfoDict & dict, std::ostringstream & oss, int indent) {
    oss << "{\n";
    int count = 0;
    int total = dict.size();
    for (auto it = dict.begin(); it != dict.end(); ++it) {
        oss << indent_string(indent + 1) << "\"" << it->first << "\": ";
        // Check the type held by the variant.
        if (it->second.value.index() == 0) { // std::string
            oss << "\"" << std::get<std::string>(it->second.value) << "\"";
        } else if (it->second.value.index() == 1) { // InfoDict
            info_dict_to_string_recursive(std::get<InfoDict>(it->second.value), oss, indent + 1);
        } else if (it->second.value.index() == 2) { // int
            oss << std::get<int>(it->second.value);
        } else if (it->second.value.index() == 3) { // float
            oss << std::get<float>(it->second.value);
        }
        count++;
        if (count < total) {
            oss << ",";
        }
        oss << "\n";
    }
    oss << indent_string(indent) << "}";
}

std::string info_dict_to_string(const InfoDict & dict) {
    std::ostringstream oss;
    info_dict_to_string_recursive(dict, oss, 0);
    return oss.str();
}
