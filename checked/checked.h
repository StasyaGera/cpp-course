#ifndef CHECKED_H
#define CHECKED_H

#include <exception>
#include <string>
#include <limits>

// exceptions

struct flow_exception : std::exception {
    flow_exception(std::string description) :
            description(description)
    {}

private:
    std::string description;

public:
    const char* what() const noexcept {
        return this->description.c_str();
    }
};

struct overflow_exception : flow_exception {
    overflow_exception(std::string description) :
            flow_exception("Overflow in " + description)
    {}
};

struct underflow_exception : flow_exception {
    underflow_exception(std::string description) :
            flow_exception("Underflow in " + description)
    {}
};

// struct checked

template <typename T>
struct checked {
    checked() : value(T()) {};
    checked(T value) : value(value) {};

private:
    T value;

public:
    checked<T>& operator=(checked<T> const& other) {
        this->value = other.value;
        return *this;
    }

    checked& operator+=(checked const& rhs);
    checked& operator-=(checked const& rhs);
    checked& operator*=(checked const& rhs);
    checked& operator/=(checked const& rhs);

    checked operator-() const;

    const T& get_value() const { return this->value; }

    operator T() const { return this->value; }
};

template <typename T>
std::string to_string(checked<T> const& a) noexcept {
    return std::to_string(a.get_value());
}

// arithmetics

template <typename T>
checked<T> &checked<T>::operator+=(checked const& rhs) {
    std::string operation = to_string(*this) + " += " + to_string(rhs);

    if ((rhs.value > 0) && (this->value > std::numeric_limits<T>::max() - rhs.value)) {
        throw overflow_exception(operation);
    } else if ((rhs.value < 0) && (this->value < std::numeric_limits<T>::min() - rhs.value)) {
        throw underflow_exception(operation);
    }

    this->value += rhs.value;
    return *this;
}

template <typename T>
checked<T> &checked<T>::operator-=(checked const& rhs) {
    std::string operation = to_string(*this) + " -= " + to_string(rhs);

    if ((rhs.value < 0) && (this->value > std::numeric_limits<T>::max() + rhs.value)) {
        throw overflow_exception(operation);
    } else if ((rhs.value > 0) && (this->value < std::numeric_limits<T>::min() + rhs.value)) {
        throw underflow_exception(operation);
    }

    this->value -= rhs.value;
    return *this;
}

template <typename T>
checked<T> &checked<T>::operator*=(checked const& rhs) {
    std::string operation = to_string(*this) + " *= " + to_string(rhs);

    if (this->value > 0) {
        if (rhs.value > 0) {
            if (this->value > (std::numeric_limits<T>::max() / rhs.value)) {
                throw overflow_exception(operation);
            }
        } else {
            if (rhs.value < (std::numeric_limits<T>::min() / this->value)) {
                throw underflow_exception(operation);
            }
        }
    } else {
        if (rhs.value > 0) {
            if (this->value < (std::numeric_limits<T>::min() / rhs.value)) {
                throw underflow_exception(operation);
            }
        } else {
            if ((this->value != 0) && (rhs.value < (std::numeric_limits<T>::max() / this->value))) {
                throw overflow_exception(operation);
            }
        }
    }

    this->value *= rhs.value;
    return *this;
}

template <typename T>
checked<T> &checked<T>::operator/=(checked const& rhs) {
    std::string operation = to_string(*this) + " /= " + to_string(rhs);

    if ((rhs.value == 0) || ((this->value == std::numeric_limits<T>::min()) && (rhs == -1))) {
        throw overflow_exception(operation);
    }

    this->value /= rhs.value;
    return *this;
}

template <typename T>
checked<T> checked<T>::operator-() const {
    try {
        return checked<T>(0) - *this;
    } catch (std::exception e) {
        throw overflow_exception("-(" + to_string(*this) + ")");
    }
}

template <typename T>
checked<T> operator+(checked<T> a, checked<T> const& b) { return a += b; }
template <typename T>
checked<T> operator-(checked<T> a, checked<T> const& b) { return a -= b; }
template <typename T>
checked<T> operator*(checked<T> a, checked<T> const& b) { return a *= b; }
template <typename T>
checked<T> operator/(checked<T> a, checked<T> const& b) { return a /= b; }

#endif //CHECKED_H
