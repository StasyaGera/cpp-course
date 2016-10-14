//
// Created by penguinni on 12.10.16.
//

#ifndef PERSISTENT_SET_H
#define PERSISTENT_SET_H

#include <utility>
#include <stack>
#include <list>
#include <memory>

struct persistent_set
{
public:
    typedef int value_type;

    // Bidirectional iterator.
    struct iterator;

private:
    struct node;

    typedef std::shared_ptr<node> node_ptr;

    struct node
    {
        node() {};
        node(value_type data) :
            data(data)
        {};
        node(value_type data, node_ptr left, node_ptr right) :
            data(data),
            left(left),
            right(right)
        {};

        value_type data;
        node_ptr left = nullptr;
        node_ptr right = nullptr;
    };

    node_ptr root;
    node_ptr afterlast = std::shared_ptr<node>(new node());
    //std::list<iterator*> iterators;

public:
    persistent_set();
    persistent_set(persistent_set const& other);

    persistent_set& operator=(persistent_set const& rhs);

    ~persistent_set();

    iterator find(value_type x);

    std::pair<iterator, bool> insert(value_type x);

    void erase(iterator it);

    iterator begin() const;
    iterator end() const;

private:
    void invalidate_iterators();
    iterator find_max(node_ptr r, std::stack<persistent_set::node_ptr> path) const;
    iterator find_min(node_ptr r, std::stack<persistent_set::node_ptr> path) const;
};

struct persistent_set::iterator
{
private:
    std::stack<node_ptr> path;
    const persistent_set& set;

    iterator(std::stack<node_ptr> path, const persistent_set& set) :
            path(path),
            set(set)
    {};
    node_ptr get_root();

public:
    value_type const& operator*() const;

    iterator& operator++();
    iterator operator++(int);

    iterator& operator--();
    iterator operator--(int);

    friend bool operator==(iterator a, iterator b);
    friend bool operator!=(iterator a, iterator b);

    friend class persistent_set;
};

bool operator==(persistent_set::iterator a, persistent_set::iterator b);
bool operator!=(persistent_set::iterator a, persistent_set::iterator b);

#endif //PERSISTENT_SET_H
