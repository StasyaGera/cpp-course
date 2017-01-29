#include <stdexcept>
#include <iostream>
#include "container_v2.2.h"

container::container()
        : sz(0),
          data_short(0)
{}

container::container(size_t sz)
        : sz(sz)
{
    if (sz == 1)
        data_short = 0;
    else if (sz > 1)
        new_data_long(sz, 0);
}

container::container(size_t sz, uint32_t value)
        : sz(sz)
{
    if (sz == 1)
        data_short = value;
    else if (sz > 1)
        new_data_long(sz, value);
}

container::container(container const& other)
        : sz(other.sz),
          capacity(other.capacity)
{
    if (other.capacity == 1)
        this->data_short = other.data_short;
    else if (other.sz == 1)
        this->data_short = other.data_long[1];
    else if (other.sz > 1) {
        other.data_long[0]++;
        this->data_long = other.data_long;
    }
}

container::~container()
{
    if (capacity > 1)
        delete_data_long();
}

void container::assign(size_t sz, uint32_t value)
{
    if (capacity > 1) {                 // data long is allocated already
        if (1 < sz && sz <= capacity) { // but we have enough storage (and no need to use short data)
            get_ownership();            // make a copy if we are not a unique user
            std::fill_n(data_long, sz + 1, value);
            this->sz = sz;
            return;
        }
        delete_data_long();             //if we don't have enough storage then prepare for new alloc
    }

    if (sz <= 1)
        data_short = value;
    else
        new_data_long(sz, value);

    this->sz = sz;
}

void container::resize(size_t sz)
{
    if (sz == this->sz) // stupid user
        return;

    if (capacity > 1 && (capacity >> 1) < sz && sz <= capacity) {
        // 1. we already have allocated data long
        // 2. new size is larger than half of the capacity (we don't want to trim our memory)
        // 2+. we are not going to switch to data short
        // 3. we already have enough storage
        this->sz = sz;
        return;
    }

    if (capacity > 1) {             // we have allocated data long, but we want to allocate another
        uint32_t* temp = data_long; // temporary pointer to current data long

        if (sz > 1) {               // we are not going to switch to data short
            new_data_long(sz, 0);   // allocate (sz << 1) + 1
            for (size_t i = 1; i <= std::min(this->sz, sz); i++) // copy old data from temp
                data_long[i] = temp[i];
        } else if (sz == 1) {       // we will switch to data short
            data_short = temp[1];   // copy old data
            capacity = 1;
        }
        delete_data_long(temp);     // free old data

    } else {                        // we don't have allocated data long => nothing to free
        uint32_t temp = data_short;
        if (sz > 1) {
            new_data_long(sz, 0);   // allocate new memory
            data_long[1] = temp;
        }
    }

    this->sz = sz;
}

void container::reserve(size_t sz)
{
    if ((sz <= 1) || (capacity > 1 && sz <= capacity))
        return;

    size_t temp = this->sz;
    resize(sz);
    this->sz = temp;
}

void container::copy(size_t i, container const &other, size_t l, size_t r)
{
    if (r <= l)
        throw std::invalid_argument(
                "big_integer: container: in function copy(): left bound is larger than the right one"
        );

    if (r - l == 1 && sz == 0) {
        sz = 1;
        data_short = (other.size() > 1) ? other.data_long[l + 1] : other.data_short;
        return;
    }

    size_t temp = sz;
    resize(r - l + sz + 1);
    for (size_t j = 0; j < temp - i; j++)
        data_long[sz - j] = data_long[temp - j];
    if (other.sz > 1) {
        for (size_t j = l + 1; j <= r; j++)
            data_long[i + j - l] = other.data_long[j];
    }
    else if (other.sz == 1)
        data_long[i + 1] = other.data_short;

    sz = r - l + sz;
}

container &container::operator=(container const &other)
{
    if (capacity > 1)
        delete_data_long(); // free old long data

    sz = other.sz;
    capacity = other.capacity;
    if (other.capacity == 1)
        data_short = other.data_short;
    else if (sz == 1) {
        data_short = other.data_long[1];
        capacity = 1;
    } else if (sz > 1) {
        data_long = other.data_long; // no real copy
        data_long[0]++;              // subscribe as owner
    } else if (sz == 0)
        capacity = 0;

    return *this;
}

uint32_t const &container::operator[](size_t i) const
{
    if (capacity == 1)
        return data_short;
    return data_long[i + 1];
}

uint32_t &container::operator[](size_t i)
{
    if (i >= sz) {
        std::cout << "warning";
    }
    if (capacity == 1)
        return data_short;
    get_ownership();
    return data_long[i + 1];
}

uint32_t const &container::back() const
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function back(): no elements"
        );

    if (capacity == 1)
        return data_short;
    return data_long[sz];
}

uint32_t &container::back()
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function back(): no elements"
        );

    if (capacity == 1)
        return data_short;
    get_ownership();
    return data_long[sz];
}

void container::push_back(const uint32_t &value)
{
    if (sz == 0) {
        sz = 1;
        data_short = value;
        return;
    }

    resize(sz + 1);
    data_long[sz] = value;
    return;
}

void container::pop_back()
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function pop_back(): no elements"
        );

    if (sz == 2) {                   // we want to switch to short data
        uint32_t temp = data_long[1];
        delete_data_long();          // so here we free long data
        data_short = temp;
    }

    sz--;
}

size_t container::size() const
{
    return sz;
}

inline void container::get_ownership()
{
    if (data_long[0] > 1) {         // if we are not a unique owner
        data_long[0]--;             // unsubscribe
        uint32_t *temp = data_long; // hold a copy pointer

        try {
            data_long = new uint32_t[capacity + 1]; // allocate new memory
        } catch (std::bad_alloc& e) {
            std::cout << "unsuccessful memory allocation: " << e.what() << std::endl;
            throw e;
        }
        data_long[0] = 1;           // set the refcount to 1

        for (size_t i = 1; i <= sz; i++)
            data_long[i] = temp[i]; // copy data
    }   // we don't need to free copied pointer's mem as there are other owners
}

inline void container::new_data_long(size_t sz, uint32_t value)
{
    capacity = sz << 1;
    try {
        data_long = new uint32_t[capacity + 1];
    } catch (std::bad_alloc& e) {
        std::cout << "unsuccessful memory allocation: " << e.what() << std::endl;
        throw e;
    }
    std::fill_n(data_long, sz + 1, value);
    data_long[0] = 1;
}

inline void container::delete_data_long()
{
    capacity = 1;
    if (data_long[0] > 1)
        data_long[0]--;
    else
        delete[] data_long;
}

inline void container::delete_data_long(uint32_t* ptr) {
    if (ptr[0] > 1)
        ptr[0]--;
    else
        delete[] ptr;
}