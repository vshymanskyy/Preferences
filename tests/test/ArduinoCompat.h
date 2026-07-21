#ifndef TEST_ARDUINO_COMPAT_H
#define TEST_ARDUINO_COMPAT_H

#include <stdint.h>
#include <string.h>
#include <string>

class String {
public:
    String() {}
    String(const char* s) : _value(s ? s : "") {}
    String(const std::string& s) : _value(s) {}

    const char* c_str() const { return _value.c_str(); }
    size_t length() const { return _value.length(); }

    String substring(size_t from, size_t to) const {
        if (from > _value.length()) return String();
        if (to > _value.length()) to = _value.length();
        if (to < from) to = from;
        return String(_value.substr(from, to - from));
    }

    String& operator=(const char* s) {
        _value = s ? s : "";
        return *this;
    }

    String operator+(const String& other) const {
        return String(_value + other._value);
    }

    String operator+(const char* other) const {
        return String(_value + (other ? other : ""));
    }

private:
    std::string _value;
};

inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

#endif
