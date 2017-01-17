#include <stdexcept>
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
        : sz(other.sz)
{
    if (other.sz == 1)
        this->data_short = other.data_short;
    else if (other.sz > 1) {
        this->data_long = other.data_long;
        this->data_long[0]++;
    }
}

container::~container()
{
    if (sz > 1)
        delete_data_long();
}

void container::assign(size_t sz, uint32_t value)
{
    if (this->sz > 1 || reserved) {
        if (1 < sz && sz <= capacity) {
            if (data_long[0] > 1)
                get_ownership();
            std::fill_n(data_long, sz + 1, value);
            this->data_long[0] = 1;
            this->sz = sz;
            return;
        }
        delete_data_long();
    }

    if (sz <= 1)
        data_short = value;
    else
        new_data_long(sz, value);

    this->sz = sz;
}

void container::resize(size_t sz)
{
    if (sz == this->sz)
        return;
    if ((this->sz > 1 || reserved) && (capacity >> 1) < sz && sz <= capacity) {
        this->sz = sz;
        return;
    }

    if (this->sz > 1 || reserved) {
        uint32_t* temp = data_long;
        if (sz > 1) {
            new_data_long(sz, 0);
            for (size_t i = 1; i <= std::min(this->sz, sz); i++)
                data_long[i] = temp[i];
        } else if (sz == 1)
            data_short = temp[1];
        delete_data_long(temp);
    } else {
        uint32_t temp = data_short;
        if (sz > 1) {
            new_data_long(sz, 0);
            data_long[1] = temp;
        }
    }

    this->sz = sz;
}

void container::reserve(size_t sz)
{
    if ((sz <= 1) || ((this->sz > 1 || reserved) && sz <= capacity))
        return;

    size_t temp = this->sz;
    resize(sz);
    this->sz = temp;
    reserved = true;
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

    if (sz == 1) {
        uint32_t temp = data_short;
        new_data_long(r - l + 1, 0);

        size_t j = (i == 1) ? 0 : r - l;
        data_long[j + 1] = temp;
        if (other.size() == 1) {
            data_long[i + 1] = other.data_short;
            sz = 2;
            return;
        }
    } else if (sz > 1) {
        uint32_t *temp = data_long;
        new_data_long(r - l + sz, 0);

        for (size_t j = 1; j <= i; j++)
            data_long[j] = temp[j];
        for (size_t j = i + (r - l); j < (r - l) + sz; j++)
            data_long[j + 1] = temp[j - (r - l) + 1];

        delete_data_long(temp);
    } else {
        new_data_long(r - l + sz, 0);
    }

    for (size_t j = l + 1; j <= r; j++)
        data_long[i + (j - l)] = other.data_long[j];
    sz = r - l + sz;
}

container &container::operator=(container const &other)
{
    if (sz > 1)
        delete_data_long(data_long);

    sz = other.sz;
    if (sz > 1) {
        data_long = other.data_long;
        data_long[0]++;
    }
    else if (sz == 1)
        data_short = other.data_short;

    return *this;
}

uint32_t const &container::operator[](size_t i) const
{
    if (sz == 1)
        return data_short;
    return data_long[i + 1];
}

uint32_t &container::operator[](size_t i)
{
    if (sz == 1)
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

    if (sz == 1)
        return data_short;
    return data_long[sz];
}

uint32_t &container::back()
{
    if (sz == 0)
        throw std::out_of_range(
                "big_integer: container: in function back(): no elements"
        );

    if (sz == 1)
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

    if (sz == 2) {
        uint32_t temp = data_long[1];
        delete_data_long(data_long);
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
    if (data_long[0] > 1) {
        data_long[0]--;
        uint32_t *temp = data_long;

        new_data_long(sz, 0);
        for (size_t i = 1; i <= sz; i++)
            data_long[i] = temp[i];
    }
}

inline void container::new_data_long(size_t sz, uint32_t value)
{
    capacity = sz << 1;
    data_long = new uint32_t[capacity];
    std::fill_n(data_long, sz + 1, value);
    data_long[0] = 1;
}

inline void container::delete_data_long()
{
    if (data_long[0] > 1)
        data_long[0]--;
    else
        delete[] data_long;
}

inline void container::delete_data_long(uint32_t *ptr) {
    if (ptr[0] > 1)
        ptr[0]--;
    else
        delete[] ptr;
}