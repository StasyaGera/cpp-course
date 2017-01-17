#ifndef BIGINT_CONTAINER_V2_H
#define BIGINT_CONTAINER_V2_H

struct container {
    container();
    container(size_t size);
    container(size_t size, uint32_t value);
    container(container const& other);

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
    size_t sz, capacity = 1;
    bool reserved = false;
    union {
        uint32_t data_short;
        uint32_t* data_long;
    };

    inline void get_ownership();
    inline void new_data_long(size_t sz, uint32_t value);
    inline void delete_data_long();
    inline void delete_data_long(uint32_t* ptr);
};

#endif //BIGINT_CONTAINER_V2_H