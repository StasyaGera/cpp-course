#ifndef MY_ANY_H
#define MY_ANY_H

#include <typeinfo>
#include <memory>
#include <iostream>

struct my_any
{
    static const int MAX_SZ = 16;

    typedef void* (*creator_t)(void);
    typedef void  (*copier_t)(const void*, void*);
    typedef void  (*mover_t)(void*, void*);
    typedef void  (*deleter_t)(void*);

    typedef const std::type_info & (*type_t)(void);

    constexpr my_any()
        : data_place(NONE),
          heap_data(nullptr),
          creator(nullptr),
          copier(nullptr),
          mover(nullptr),
          deleter(nullptr),
          typer(my_any::my_type<void>)
    {}

    my_any(my_any const& other)
        : data_place(other.data_place),
          creator(other.creator),
          copier(other.copier),
          mover(other.mover),
          deleter(other.deleter),
          typer(other.typer)
    {
        switch (data_place)
        {
            case HEAP:
                heap_data = creator();
                copier(other.heap_data, heap_data);
                break;
            case STACK:
                copier(&other.local_data, &local_data);
                break;
            default:
                break;
        }
    }

    my_any(my_any&& other)
        : data_place(std::move(other.data_place)),
          creator(std::move(other.creator)),
          copier(std::move(other.copier)),
          mover(std::move(other.mover)),
          deleter(std::move(other.deleter)),
          typer(std::move(other.typer))
    {
        switch (data_place)
        {
            case HEAP:
                heap_data = other.heap_data;
                other.heap_data = nullptr;
                other.data_place = NONE;
                break;
            case STACK:
                mover(&other.local_data, &local_data);
                break;
            default:
                break;
        }
    }


    struct small_tag {};
    struct big_tag {};

    template<typename T>
    using is_small = typename std::integral_constant<
            bool,
            sizeof(std::decay_t<T>) <= MAX_SZ &&
            alignof(std::decay_t<T>) <= MAX_SZ &&
            std::is_nothrow_move_constructible<T>::value
    >;

    template<typename T>
    void ctor(T&& value, small_tag)
    {
        copier(&value, &local_data);
        deleter = my_any::my_stack_delete<typename std::decay<T>::type>;
        data_place = STACK;
    }

    template<typename T>
    void ctor(T&& value, big_tag)
    {
        heap_data = creator();
        copier(&value, heap_data);
        deleter = my_any::my_heap_delete<typename std::decay<T>::type>;
        data_place = HEAP;
    }

    template<typename T,
            typename = typename std::enable_if<!std::is_same<std::decay_t<T>, my_any>::value>::type>
    my_any(T&& value)
        : creator(my_any::my_create<typename std::decay<T>::type>),
          copier(my_any::my_copy<typename std::decay<T>::type>),
          mover(my_any::my_move<typename std::decay<T>::type>),
          typer(my_any::my_type<typename std::decay<T>::type>)
    {
        ctor(std::forward<T>(value),
             typename std::conditional<is_small<std::decay_t<T>>::value, small_tag, big_tag>::type());
    }

    my_any& operator=(my_any const& rhs)
    {
        my_any tmp(rhs);
        swap(tmp);
        return *this;
    }

    my_any& operator=(my_any&& rhs)
    {
        swap(rhs);
        return *this;
    }

    template<typename T>
    my_any& operator=(T&& rhs)
    {
        *this = my_any(std::forward<T>(rhs));
        return *this;
    }

    ~my_any()
    {
        reset();
    }

    void reset()
    {
        switch (data_place)
        {
            case HEAP:
                deleter(heap_data);
                break;
            case STACK:
                deleter(&local_data);
                break;
            default:
                break;
        }
        data_place = NONE;

        creator = nullptr;
        copier = nullptr;
        mover = nullptr;
        deleter = nullptr;
        typer = nullptr;
    }

    void swap(my_any& other)
    {
        using std::swap;
        swap(*this, other);
    }

    bool has_value() const
    {
        return data_place != NONE;
    }

    const std::type_info & type() const {
        return typer();
    }

    friend void swap(my_any& lhs, my_any& rhs);

    template<typename T>
    friend T any_cast(const my_any& operand);

    template<typename T>
    friend T any_cast(my_any& operand);

    template<typename T>
    friend T any_cast(my_any&& operand);

    template<typename T>
    friend const T* any_cast(const my_any* operand);

    template<typename T>
    friend T* any_cast(my_any* operand);

private:
    enum state
    {
        NONE,
        HEAP,
        STACK
    } data_place;

    union
    {
        void* heap_data;
        typename std::aligned_storage<MAX_SZ, MAX_SZ>::type local_data;
    };

    creator_t creator;
    copier_t  copier;
    mover_t   mover;
    deleter_t deleter;
    type_t typer;

    template <typename T>
    static void* my_create() {
        return new typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    }

    template <typename T>
    static void my_copy(const void* from, void* to) {
        new(to) T(*(const T*)from);
    }

    template<typename T>
    static void my_move(void* from, void* to) {
        new(to) T(std::move(*(T*)from));
    }

    template<typename T>
    static void my_heap_delete(void* t) {
        delete static_cast<T*>(t);
    }

    template <typename T>
    static void my_stack_delete(void* t) {
        ((T*)t)->~T();
    }

    template <typename T>
    const static std::type_info& my_type() {
        return typeid(T);
    }

    void* get() const
    {
        switch (data_place) {
            case HEAP:
                return heap_data;
            case STACK:
                return (void*)&local_data;
            default:
                return nullptr;
        }
    }
};

void swap(my_any& lhs, my_any& rhs)
{
    if (rhs.data_place == lhs.data_place) {
        if (rhs.data_place == my_any::state::STACK) {
            typename std::aligned_storage<my_any::MAX_SZ, my_any::MAX_SZ>::type tmp;

            lhs.mover(&lhs.local_data, &tmp);
            lhs.deleter(&lhs.local_data);

            rhs.mover(&rhs.local_data, &lhs.local_data);
            rhs.deleter(&rhs.local_data);

            lhs.mover(&tmp, &rhs.local_data);
            lhs.deleter(&tmp);
        }
        else if (rhs.data_place == my_any::state::HEAP) {
            std::swap(lhs.heap_data, rhs.heap_data);
        }
    }
    else {
        if (lhs.data_place == my_any::state::STACK && rhs.data_place == my_any::state::HEAP) {
            void* tmp = rhs.heap_data;

            lhs.mover(&lhs.local_data, &rhs.local_data);
            lhs.deleter(&lhs.local_data);

            lhs.heap_data = tmp;
        }
        else if (lhs.data_place == my_any::state::STACK && rhs.data_place == my_any::state::NONE) {
            lhs.mover(&lhs.local_data, &rhs.local_data);
            lhs.deleter(&lhs.local_data);
        }
        else if (lhs.data_place == my_any::state::HEAP && rhs.data_place == my_any::state::NONE) {
            rhs.heap_data = lhs.heap_data;
            lhs.heap_data = nullptr;
        }
        else {
            std::swap(rhs, lhs);
            return;
        }
    }

    std::swap(lhs.data_place, rhs.data_place);
    std::swap(lhs.creator, rhs.creator);
    std::swap(lhs.copier, rhs.copier);
    std::swap(lhs.deleter, rhs.deleter);
    std::swap(lhs.mover, rhs.mover);
    std::swap(lhs.typer, rhs.typer);
}

template<typename T>
T any_cast(const my_any& operand) {
    return *(std::add_const_t<std::remove_reference_t<T>>*)operand.get();
}

template<typename T>
T any_cast(my_any& operand) {
    return *(std::remove_reference_t<T>*)operand.get();
}

template<typename T>
T any_cast(my_any&& operand) {
    return *(std::remove_reference_t<T>*)operand.get();
}

template<typename T>
const T* any_cast(const my_any* operand) {
    if (operand != nullptr && typeid(T) == operand->type()) {
        return (const T *) operand->get();
    }
    else {
        return nullptr;
    }
}

template<typename T>
T* any_cast(my_any* operand) {
    if (operand != nullptr && typeid(T) == operand->type()) {
        return (T*) operand->get();
    }
    else {
        return nullptr;
    }
}


#endif //MY_ANY_H
