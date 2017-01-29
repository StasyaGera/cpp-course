#ifndef MY_BIND_H
#define MY_BIND_H

#include <tuple>

namespace bind_arg_types {
    template <typename T>
    struct my_const;

    template <size_t I>
    struct get;

    template <typename F, typename... Args>
    struct my_bind;
};


template <typename T>
struct arg_wrapper
{
    typedef bind_arg_types::my_const<T> type;
};

template <size_t N>
struct arg_wrapper< bind_arg_types::get<N> >
{
    typedef bind_arg_types::get<N> type;
};

template <typename F, typename... Args>
struct arg_wrapper< bind_arg_types::my_bind<F, Args...> >
{
    typedef bind_arg_types::my_bind<F, Args...> type;
};


namespace bind_arg_types
{
// operator() for const values
    template <typename T>
    struct my_const {
        my_const(T&& value)
            : value(std::forward<T>(value))
        {}

        template <typename... Args>
        T& operator()(Args const&...) {
            return value;
        }

    private:
        T value;
    };

// operator() for an I-th argument of the first pack
    template <size_t I>
    struct get
    {
        constexpr get() {}

        template <typename... Args>
        auto&& operator()(Args&&... args) {
            return std::get<I>(std::forward_as_tuple(args...));
        };
    };

// operator() for a bind object
    template <typename F, typename... Args>
    struct my_bind
    {
        //would be better if it was private
        my_bind(F&& f, Args&&... args) //universal references
                : f(std::forward<F>(f)),
                  less_args(std::forward<typename std::decay<Args>::type>(args)...)
        {}

        template <typename... More_args>
        auto operator()(More_args&&... more_args) { //universal reference
            return call(std::index_sequence_for<Args...>{}, std::forward<More_args>(more_args)...);
        }

    private:
        F f;
        std::tuple< typename arg_wrapper< typename std::decay<Args>::type >::type... > less_args;


        template <size_t... Indices, typename... More_args>
        auto call(std::index_sequence<Indices...>, More_args&&... more_args) { //universal reference
            return f(std::get<Indices>(less_args)(std::forward<More_args>(more_args)...)...);
        }
    };
}


template <size_t N>
using my_placeholder = bind_arg_types::get<N>;
constexpr my_placeholder<0> _1;
constexpr my_placeholder<1> _2;
constexpr my_placeholder<2> _3;

template <typename F, typename... Args>
bind_arg_types::my_bind<F, Args...> bind(F&& function, Args&&... args) { // universal references
    return bind_arg_types::my_bind<F, Args...>(std::forward<F>(function), std::forward<Args>(args)...);
};

#endif //MY_BIND_H