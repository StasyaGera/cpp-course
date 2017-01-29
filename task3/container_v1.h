#ifndef BIGINT_CONTAINER_H
#define BIGINT_CONTAINER_H

#include <vector>
#include <memory>

struct container {
    container();
    container(size_t size);
    container(size_t size, uint32_t value);
    container(const container& other);

    ~container();

    void assign(size_t size, uint32_t value);
    void resize(size_t size);
    void reserve(size_t size);
    void copy(size_t i, container const& other, size_t l, size_t r);

    container& operator=(container const& other);

    uint32_t const& operator[](size_t i) const;
    uint32_t& operator[](size_t i);

    uint32_t const& back() const;
    uint32_t& back();

    void push_back(uint32_t const& value);
    void pop_back();

    size_t size() const;

private:
    size_t sz;

    std::shared_ptr< std::vector<uint32_t> > data_long;
    uint32_t data_short;

    inline void real_copy();
};

#endif //BIGINT_CONTAINER_H