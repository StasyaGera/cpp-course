#ifndef CHECKED_H
#define CHECKED_H

#include <exception>
#include <string>
#include <limits>

// exceptions

struct flow_exception : std::exception {
    flow_exception() : description("") {}
    flow_exception(std::string description) : description(description) {}

protected:
    std::string description;

public:
    virtual void set_description(std::string text) {
        description += text;
    }
    const char* what() const noexcept {
        return this->description.c_str();
    }
};

struct overflow_exception : flow_exception {
    overflow_exception() : flow_exception("Overflow in ") {};
    overflow_exception(std::string description) :
            flow_exception("Overflow in " + description)
    {}

    void set_description(std::string text) {
        description = "Overflow in " + text;
    }
};

struct underflow_exception : flow_exception {
    underflow_exception() : flow_exception("Underflow in ") {};
    underflow_exception(std::string description) :
            flow_exception("Underflow in " + description)
    {}

    void set_description(std::string text) {
        description = "Underflow in " + text;
    }
};

// tags
struct tag {};
struct signed_tag : tag {};
struct unsigned_tag : tag {};

// struct

template <typename T>
struct checked {
    typedef typename std::conditional<std::is_signed<T>::value, signed_tag, unsigned_tag>::type checked_type;

    checked() : value(T()) {};
    checked(T value) : value(value) {};

private:
    T value;

public:
    checked<T>& operator=(checked<T> const& other) {
        this->value = other.value;
        return *this;
    }

    const T& get_value() const { return this->value; }
    operator T() const { return this->value; }
};

template <typename T>
std::string to_string(checked<T> const& a) noexcept {
    return std::to_string(a.get_value());
}


template <typename T>
T add(T left, T right, signed_tag) {
    if ((right > 0) && (left > std::numeric_limits<T>::max() - right)) {
        throw overflow_exception();
    } else if ((right < 0) && (left < std::numeric_limits<T>::min() - right)) {
        throw underflow_exception();
    }

    return left + right;
}

template <typename T>
T add(T left, T right, unsigned_tag) {
    if (std::numeric_limits<T>::max() - left < right) {
        throw overflow_exception();
    }

    return left + right;
}

template <typename T>
T sub(T left, T right, signed_tag) {
    if ((right < 0) && (left > std::numeric_limits<T>::max() + right)) {
        throw overflow_exception();
    } else if ((right > 0) && (left < std::numeric_limits<T>::min() + right)) {
        throw underflow_exception();
    }

    return left - right;
}

template <typename T>
T sub(T left, T right, unsigned_tag) {
    if (left < right) {
        throw underflow_exception();
    }

    return left - right;
}

template <typename T>
T mul(T left, T right, signed_tag) {
    if (left > 0) {
        if (right > 0) {
            if (left > (std::numeric_limits<T>::max() / right)) {
                throw overflow_exception();
            }
        } else {
            if (right < (std::numeric_limits<T>::min() / left)) {
                throw underflow_exception();
            }
        }
    } else {
        if (right > 0) {
            if (left < (std::numeric_limits<T>::min() / right)) {
                throw underflow_exception();
            }
        } else {
            if ((left != 0) && (right < (std::numeric_limits<T>::max() / left))) {
                throw overflow_exception();
            }
        }
    }

    return left * right;
}

template <typename T>
T mul(T left, T right, unsigned_tag) {
    if (left > (std::numeric_limits<T>::max() / right)) {
        throw overflow_exception();
    }

    return left * right;
}

template <typename T>
T div(T left, T right, signed_tag) {
    if ((right == 0) || ((left == std::numeric_limits<T>::min()) && (right == -1))) {
        throw overflow_exception();
    }

    return left / right;
}

template <typename T>
T div(T left, T right, unsigned_tag) {
    if (right == 0) {
        throw overflow_exception();
    }

    return left / right;
}

template <typename T>
checked<T> operator-(checked<T> const& a) {
    try {
        return checked<T>(0) - a;
    } catch (flow_exception e) {
        throw overflow_exception("-(" + to_string(a) + ")");
    }
}

template <typename T>
checked<T> operator+(checked<T> const& a, checked<T> const& b) {
    try {
        return checked<T>(add(a.get_value(), b.get_value(), typename checked<T>::checked_type()));
    } catch (flow_exception e) {
        e.set_description(to_string(a) + " + " + to_string(b));
        throw e;
    }
}

template <typename T>
checked<T> operator-(checked<T> const& a, checked<T> const& b) {
    try {
        return checked<T>(sub(a.get_value(), b.get_value(), typename checked<T>::checked_type()));
    } catch (flow_exception e) {
        e.set_description(to_string(a) + " - " + to_string(b));
        throw e;
    }
}

template <typename T>
checked<T> operator*(checked<T> const& a, checked<T> const& b) {
    try {
        return checked<T>(mul(a.get_value(), b.get_value(), typename checked<T>::checked_type()));
    } catch (flow_exception e) {
        e.set_description(to_string(a) + " * " + to_string(b));
        throw e;
    }
}

template <typename T>
checked<T> operator/(checked<T> const& a, checked<T> const& b) {
    try {
        return checked<T>(div(a.get_value(), b.get_value(), typename checked<T>::checked_type()));
    } catch (flow_exception e) {
        e.set_description(to_string(a) + " / " + to_string(b));
        throw e;
    }
}

#endif //CHECKED_H
