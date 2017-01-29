#include <stdexcept>
#include <iostream>
#include "container_v1.h"

container::container()
    : sz(0)
{}

container::container(size_t size)
    : sz(size)
{
    if (size > 1)
        data_long = std::make_shared< std::vector<uint32_t> >(size);
}

container::container(const container &other)
    : sz(other.sz)
{
    if (sz == 1)
        data_short = other.data_short;
    else
        data_long = other.data_long;
}

container::container(size_t size, uint32_t value)
    : sz(size)
{
    if (size == 1)
        data_short = value;
    else
        data_long = std::make_shared< std::vector<uint32_t> >(size, value);
}

container::~container()
{}

void container::assign(size_t sz, uint32_t value)
{
    if (sz > 1) {
        this->sz = sz;
        data_long = std::make_shared< std::vector<uint32_t> >(sz, value);
        return;
    }

    if (this->sz > 1)
        data_long.reset();
    data_short = value;
    this->sz = sz;
}

void container::resize(size_t sz)
{
    if (this->sz > 1) {
        if (sz > 1) {
            real_copy();
            data_long->resize(sz);
        } else if (sz == 1) {
            data_short = (*data_long)[0];
            data_long.reset();
        }
    } else if (sz > 1) {
        data_long = std::make_shared< std::vector<uint32_t> >(sz);
        if (this->sz == 1)
            (*data_long)[0] = data_short;
    }
    this->sz = sz;
}

void container::reserve(size_t sz)
{
    if (sz <= this->sz)
        return;

    if (this->sz > 1)
        real_copy();
    else
        data_long = std::make_shared< std::vector<uint32_t> >(1, data_short);

    data_long->reserve(sz);
}

void container::copy(size_t i, container const &other, size_t l, size_t r)
{
    if (r <= l)
        throw std::invalid_argument(
                "big_integer: container: in function copy(): left bound is larger than the right one"
        );

    if (r - l == 1 && sz == 0) {
        sz = 1;
        data_short = *(other.data_long->begin() + l);
        return;
    }

    if (sz == 1)
        data_long = std::make_shared< std::vector<uint32_t> > (1, data_short);
    else
        real_copy();

    if (other.size() == 1)
        data_long->insert(data_long->begin() + i, other.data_short);
    else
        std::copy(other.data_long->begin() + l,
                  other.data_long->begin() + r,
                  this->data_long->begin() + i
        );

    sz = data_long->size();
}

container& container::operator=(container const &other)
{
    sz = other.sz;
    if (other.sz > 1)
        data_long = other.data_long;
    else {
        data_long.reset();
        data_short = other.data_short;
    }

    return *this;
}

uint32_t const& container::operator[](size_t i) const
{
    if (sz == 1)
        return data_short;
    return (*data_long)[i];
}

uint32_t& container::operator[](size_t i)
{
    if (sz == 1)
        return data_short;

    real_copy();
    return (*data_long)[i];
}

uint32_t const& container::back() const
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function back(): no elements"
        );

    if (sz == 1)
        return data_short;
    return data_long->back();
}

uint32_t& container::back()
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function back(): no elements"
        );

    if (sz == 1)
        return data_short;
    real_copy();
    return data_long->back();
}

void container::push_back(const uint32_t &value)
{
    if (sz == 0) {
        sz++;
        data_short = value;
        return;
    }

    if (sz == 1)
        data_long = std::make_shared< std::vector<uint32_t> >(1, data_short);
    else
        real_copy();
    sz++;
    data_long->push_back(value);
}

void container::pop_back()
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function pop_back(): no elements"
        );

    if (sz == 2) {
        data_short = (*data_long)[0];
        data_long.reset();
    } else if (sz > 2) {
        real_copy();
        data_long->pop_back();
    }

    sz--;
    return;
}

size_t container::size() const
{
    return sz;
}

inline void container::real_copy()
{
    if (!data_long.unique())
        data_long = std::make_shared< std::vector<uint32_t> >(*data_long);
}