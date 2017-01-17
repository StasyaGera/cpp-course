#include "big_integer.h"

#include <cstring>
#include <algorithm>
#include <iostream>

const int32_t POWER = 30, BASE = (1 << POWER);

big_integer::big_integer() :
        number(container(1, 0)),
        sign(1)
{}

big_integer::big_integer(big_integer const &other) :
        number(other.number),
        sign(other.sign)
{}

big_integer::big_integer(int a)
{
    uint32_t value;

    if (a < 0) {
        this->sign = -1;
        if (a == INT32_MIN)
            value = (uint32_t) (1 << 31);
        else
            value = (uint32_t) -a;
    } else {
        value = (uint32_t) a;
        this->sign = 1;
    }

    this->number = container(0);
    if (value >= BASE) {
        this->number.push_back(value % BASE);
        this->number.push_back(value >> POWER);
    } else
        this->number.push_back(value);
}

big_integer::big_integer(std::string const &str) :
        number(1, 0),
        sign(1)
{
    size_t s = 0;
    bool neg = false;
    while (s < str.length() && (str[s] == '-' || str[s] == '+')) {
        if (str[s] == '-')
            neg = !neg;
        s++;
    }

    for (size_t i = s; i < str.length(); i++) {
        *this *= 10;
        *this += str[i] - '0';
    }

    if (neg)
        this->sign = -1;
}

big_integer::~big_integer() {}

big_integer &big_integer::operator=(big_integer const &other)
{
    this->number = other.number;
    this->sign = other.sign;

    return *this;
}

big_integer &big_integer::operator+=(big_integer const &rhs)
{
    if (this->sign == rhs.sign) {
        *this = this->add(rhs);
        return *this;
    } else {
        if (this->abs() > rhs.abs()) {
            *this = this->sub(rhs);
            return *this;
        } else {
            *this = rhs.sub(*this);
            this->sign = rhs.sign;
            return *this;
        }
    }
}

big_integer &big_integer::operator-=(big_integer const &rhs)
{
    if (this->sign != rhs.sign) {
        *this = this->add(rhs);
        return *this;
    } else {
        if (this->abs() > rhs.abs()) {
            *this = this->sub(rhs);
            return *this;
        } else {
            *this = rhs.sub(*this);
            this->sign = -rhs.sign;
            return *this;
        }
    }
}

big_integer &big_integer::operator*=(big_integer const &rhs)
{
    big_integer copy(*this);
    this->number.assign(this->number.size() + rhs.number.size() + 1, 0);

    for (size_t i = 0; i < copy.number.size(); i++) {
        int64_t temp = 0;
        for (size_t j = 0; j < rhs.number.size(); j++) {
            temp += (int64_t) rhs.number[j] * copy.number[i] + this->number[i + j];
            this->number[i + j] = (uint32_t) (temp & (BASE - 1));

            temp >>= POWER;
        }
        this->number[i + rhs.number.size()] += temp;
    }

    this->trim();
    this->sign *= rhs.sign;

    return *this;
}

big_integer &big_integer::operator/=(big_integer const &rhs)
{
    if (this->abs() < rhs.abs()) {
        *this = 0;
        return *this;
    }

    if (rhs.number.size() == 1) {
        this->div_long_short(rhs.number[0]);
        this->sign *= rhs.sign;
        return *this;
    }

    big_integer divider = rhs.abs(), carry;
    for (int i = (int) (this->number.size() - 1); i >= 0; i--) {
        carry = (carry << POWER) + this->number[i];

        if (carry >= rhs) {
            int l = 0, r = BASE, m = (l + r) / 2;
            while (r - l > 1) {
                big_integer temp = m * divider;
                if (temp <= carry)
                    l = m;
                else
                    r = m;

                m = (l + r) / 2;
            }

            this->number[i] = (uint32_t) l;
            carry -= l * divider;
        } else if ((uint32_t) i == this->number.size() - 1)
            this->number.pop_back();
    }

    this->trim();
    this->sign *= rhs.sign;

    return *this;
}

big_integer &big_integer::operator%=(big_integer const &rhs)
{
    big_integer temp(*this / rhs);
    return (*this -= temp * rhs);
}

big_integer &big_integer::operator&=(big_integer const &rhs)
{
    if (this->sign < 0) {
        *this = this->convert();
        if (this->number.size() < rhs.number.size())
            for (size_t i = this->number.size(); i < rhs.number.size(); i++)
                this->number.push_back((uint32_t) (BASE - 1));
    }

    big_integer rhscopy;
    if (rhs.sign < 0) {
        rhscopy = rhs.convert();
        if (this->number.size() > rhscopy.number.size())
            for (size_t i = rhscopy.number.size(); i < this->number.size(); i++)
                rhscopy.number.push_back((uint32_t) (BASE - 1));
    } else
        rhscopy = rhs;

    unsigned size = (uint32_t) std::min(this->number.size(), rhscopy.number.size());
    for (size_t i = 0; i < size; i++)
        this->number[i] &= rhscopy.number[i];

    if (this->sign > 0)
        this->number.resize(size);
    else
        *this = this->convert();

    return *this;
}

big_integer &big_integer::operator|=(big_integer const &rhs)
{
    unsigned size = (uint32_t) std::min(this->number.size(), rhs.number.size());

    if (this->sign < 0)
        *this = this->convert();

    big_integer rhscopy;
    if (rhs.sign < 0)
        rhscopy = rhs.convert();
    else
        rhscopy = rhs;

    if (this->number.size() < rhscopy.number.size()) {
        this->number.resize(rhscopy.number.size());
        if (this->sign > 0) {
            this->number.copy(size, rhscopy.number, size, rhscopy.number.size());
        }
    }

    for (size_t i = 0; i < size; i++)
        this->number[i] |= rhscopy.number[i];

    if (this->sign < 0 || rhscopy.sign < 0) {
        *this = this->convert();
        this->sign = -1;
    }

    return *this;
}

big_integer &big_integer::operator^=(big_integer const &rhs)
{
    unsigned size = (uint32_t) std::min(this->number.size(), rhs.number.size());

    if (this->sign < 0)
        *this = this->convert();

    big_integer rhscopy;
    if (rhs.sign < 0)
        rhscopy = rhs.convert();
    else
        rhscopy = rhs;

    if (this->number.size() < rhscopy.number.size()) {
        this->number.resize(rhscopy.number.size());
        this->number.copy(size, rhscopy.number, size, rhscopy.number.size());
    }

    for (size_t i = 0; i < size; i++)
        this->number[i] ^= rhscopy.number[i];

    this->sign *= rhs.sign;

    if (this->sign < 0)
        *this = this->convert();

    return *this;
}

big_integer &big_integer::operator<<=(int32_t rhs)
{
    uint32_t n = (uint32_t) (rhs / (POWER)), r = (uint32_t) (rhs % POWER), size = (uint32_t) this->number.size();

    this->number.resize(this->number.size() + n + 1);

    if (n) {
        for (int32_t i = size - 1; i >= 0; i--) {
            this->number[i + n] = this->number[i];
            this->number[i] = 0;
        }
    }

    if (r) {
        uint32_t last = (uint32_t) (this->number.size() - 2);
        this->number.back() = (this->number[this->number.size() - 2] >> (POWER - r));

        for (int32_t i = last; i > 0; i--) {
            this->number[i] <<= r;
            this->number[i] &= BASE - 1;

            int32_t temp = (this->number[i - 1] >> (POWER - r));
            this->number[i] += temp;
            this->number[i - 1] -= temp << (POWER - r);
        }

        this->number[0] <<= r;
        this->number[0] &= BASE - 1;
    }

    this->trim();
    return *this;
}

big_integer &big_integer::operator>>=(int32_t rhs)
{
    uint32_t n = (uint32_t) (rhs / (POWER)), r = (uint32_t) (rhs % POWER), size = (uint32_t) this->number.size();

    if (n) {
        for (size_t i = 0; i < size - n; i++) {
            this->number[i] = this->number[i + n];
            this->number[i + n] = 0;
        }
    }

    if (this->sign < 0) {
        *this = this->convert();
        this->number.push_back((uint32_t) (BASE - 1));
    } else {
        this->number.resize(size - n);
        this->number.push_back(0);
    }

    if (r) {
        for (size_t i = 0; i < this->number.size() - 1; i++) {
            this->number[i] >>= r;
            this->number[i] += (this->number[i + 1] << (POWER - r)) & (BASE - 1);
        }
    }

    if (this->sign < 0)
        *this = this->convert();

    this->trim();
    return *this;
}

big_integer big_integer::operator+() const
{
    return *this;
}

big_integer big_integer::operator-() const
{
    big_integer copy = *this;
    copy.sign *= -1;
    return copy;
}

big_integer big_integer::operator~() const
{
    return -(*this) - 1;
}

big_integer &big_integer::operator++()
{
    return (*this += 1);
}

big_integer big_integer::operator++(int32_t a)
{
    big_integer num = a;
    return num++;
}

big_integer &big_integer::operator--()
{
    return (*this -= 1);
}

big_integer big_integer::operator--(int32_t a)
{
    big_integer num = a;
    return num--;
}

big_integer operator+(big_integer a, big_integer const &b)
{
    return big_integer(a) += b;
}

big_integer operator-(big_integer a, big_integer const &b)
{
    return big_integer(a) -= b;
}

big_integer operator*(big_integer a, big_integer const &b)
{
    return big_integer(a) *= b;
}

big_integer operator/(big_integer a, big_integer const &b)
{
    return big_integer(a) /= b;
}

big_integer operator%(big_integer a, big_integer const &b)
{
    return big_integer(a) %= b;
}

big_integer operator&(big_integer a, big_integer const &b)
{
    return big_integer(a) &= b;
}

big_integer operator|(big_integer a, big_integer const &b)
{
    return big_integer(a) |= b;
}

big_integer operator^(big_integer a, big_integer const &b)
{
    return big_integer(a) ^= b;
}

big_integer operator<<(big_integer a, int32_t b)
{
    return big_integer(a) <<= b;
}

big_integer operator>>(big_integer a, int32_t b)
{
    return big_integer(a) >>= b;
}

bool operator==(big_integer const &a, big_integer const &b)
{
    if ((a.number.size() == 1) && (a.number[0] == 0) && (b.number.size() == 1) && (b.number[0] == 0))
        return true;

    if (a.sign != b.sign)
        return false;
    else {
        if (a.number.size() != b.number.size())
            return false;
        else {
            int32_t i = (int) (a.number.size() - 1);
            while (i >= 0 && a.number[i] == b.number[i])
                i--;

            return i < 0;
        }
    }
}

bool operator!=(big_integer const &a, big_integer const &b)
{
    return !(a == b);
}

bool operator<(big_integer const &a, big_integer const &b)
{
    if ((a == 0) && (b == 0))
        return false;

    if (a.sign != b.sign)
        return a.sign < 0;
    else {
        if (a.number.size() < b.number.size())
            return a.sign > 0;
        else if (a.number.size() > b.number.size())
            return a.sign < 0;
        else {
            int32_t i = (int) (a.number.size() - 1);
            while (i >= 0 && a.number[i] == b.number[i])
                i--;

            if (i < 0)
                return false;
            else
              return a.number[i] < b.number[i];
        }
    }
}

bool operator>(big_integer const &a, big_integer const &b)
{
    return b < a;
}

bool operator<=(big_integer const &a, big_integer const &b)
{
    return !(a > b);
}

bool operator>=(big_integer const &a, big_integer const &b)
{
    return b <= a;
}

std::string to_string(big_integer const &a)
{
    if (a == 0)
        return "0";

    string result = "";

    big_integer ten = (int) 1e8, copy = a;
    while (copy != 0) {
        string mod = to_string((copy % ten).number[0]);
        copy /= ten;

        while ((copy != 0) && (mod.length() < 8))
            mod = "0" + mod;
        result = mod + result;
    }

    if (a.sign < 0)
        result = "-" + result;

    return result;
}

std::ostream &operator<<(std::ostream &s, big_integer const &a)
{
    return s << to_string(a);
}

// returns the absolute value of (this)
big_integer big_integer::abs() const
{
    big_integer copy = *this;
    copy.sign = 1;
    return copy;
}

// adds two numbers as if they have the same sign
big_integer big_integer::add(big_integer const &rhs) const
{
    big_integer result;
    result.sign = this->sign;

    size_t size = std::max(this->number.size(), rhs.number.size());
    result.number.reserve(size + 1);
    result.number.resize(size);

    int64_t temp = 0;
    for (size_t i = 0; i < size; i++) {
        temp += (i < this->number.size()) ? this->number[i] : 0;
        temp += (i < rhs.number.size()) ? rhs.number[i] : 0;

        result.number[i] = (uint32_t) (temp & (BASE - 1));
        temp >>= POWER;
    }

    if (temp != 0)
        result.number.push_back((uint32_t) (temp & (BASE - 1)));

    return result;
}

// subtracts two numbers as if they have the same sign and (this) is greater than (rhs)
big_integer big_integer::sub(big_integer const &rhs) const
{
    big_integer result;
    result.sign = this->sign;

    size_t size = std::max(this->number.size(), rhs.number.size());
    result.number.reserve(size + 1);
    result.number.resize(size);

    int64_t temp = 0;
    bool borrow = 0;
    for (size_t i = 0; i < size; i++) {
        temp += (i < this->number.size()) ? this->number[i] : 0;
        temp -= borrow;
        temp -= (i < rhs.number.size()) ? rhs.number[i] : 0;

        borrow = 0;

        if (temp < 0) {
            temp += BASE;
            borrow++;
        }

        result.number[i] = (uint32_t) (temp & (BASE - 1));
        temp >>= POWER;
    }

    if (temp != 0)
        result.number.push_back((uint32_t) (temp & (BASE - 1)));

    result.trim();
    return result;
}

// converts (this) to the two's complement representation
// the (sign) remains
big_integer big_integer::convert() const
{
    big_integer copy;
    copy.sign = this->sign;
    copy.number.resize(this->number.size());

    copy.number[0] = (-this->number[0]) & (BASE - 1);
    for (size_t i = 1; i < this->number.size(); i++)
        copy.number[i] = (~this->number[i]) & (BASE - 1);

    return copy;
}

// divides (this) by (rhs)
// returns remainder
// does not change sign
uint32_t big_integer::div_long_short(uint32_t rhs)
{
    int64_t carry = 0;

    for (int32_t i = (int) this->number.size() - 1; i >= 0; i--) {
        carry = (carry << POWER) + this->number[i];
        if (carry >= rhs) {
            this->number[i] = (uint32_t) (carry / rhs);
            carry %= rhs;
        } else if ((uint32_t) i == this->number.size() - 1)
            this->number.pop_back();
    }

    return (uint32_t) carry;
}

void big_integer::trim()
{
    while ((this->number.size() > 1) && (!this->number.back()))
        this->number.pop_back();
}