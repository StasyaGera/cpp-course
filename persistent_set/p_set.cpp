#include "p_set.h"

persistent_set::persistent_set() : root(nullptr) {}

persistent_set::persistent_set(persistent_set const & other) :
    root(other.root)
{ }

persistent_set &persistent_set::operator=(persistent_set const &rhs) {
    this->invalidate_iterators();
    this->root = rhs.root;

    return *this;
}

persistent_set::~persistent_set() {
    this->invalidate_iterators();
}

persistent_set::iterator persistent_set::find(persistent_set::value_type x) {
    if (this->root == nullptr) {
        return this->end();
    }

    std::stack<node_ptr> path;
    node_ptr curr = root;
    while (x != curr->data) {
        path.push(curr);

        if (x < curr->data) {
            if (curr->left != nullptr) {
                curr = curr->left;
            } else {
                return this->end();
            }
        } else {
            if (curr->right != nullptr) {
                curr = curr->right;
            } else {
                return this->end();
            }
        }
    }
    path.push(curr);

    return iterator(path, *this);
}

std::pair<persistent_set::iterator, bool> persistent_set::insert(persistent_set::value_type x) {
    iterator it = this->find(x);

    if (it != this->end()) {
        return { it, false };
    }

    this->invalidate_iterators();

    std::stack<node_ptr> path;
    node_ptr curr = this->root, copy, prev = nullptr;
    while (curr != nullptr) {
        copy = std::shared_ptr<node>(new node(curr->data));

        if (prev != nullptr) {
            copy->data < prev->data ? prev->left = copy : prev->right = copy;
        } else {
            this->root = copy;
        }

        if (x < curr->data) {
            copy->right = curr->right;
            curr = curr->left;
        } else {
            copy->left = curr->left;
            curr = curr->right;
        }

        prev = copy;
        path.push(copy);
    }

    curr = std::shared_ptr<node>(new node(x));
    if (prev == nullptr) {
        this->root = curr;
    } else {
        if (x < prev->data) {
            prev->left = curr;
        } else {
            prev->right = curr;
        }
    }
    path.push(curr);

    return { persistent_set::iterator(path, *this), true };
}

void persistent_set::erase(persistent_set::iterator it) {
    node_ptr copy, prev, my = it.path.top();
    it.path.pop();

    if (my->left == nullptr && my->right == nullptr) {
        prev = nullptr;
    } else if (my->left == nullptr) {
        prev = my->right;
    } else if (my->right == nullptr) {
        prev = my->left;
    } else {
        value_type mindata = (it.set.find_min(my->right, new std::stack<node_ptr>)).path.top()->data;

        persistent_set small;
        small.root = my;
        small.erase(small.find(mindata));

        prev = small.root;
        prev->data = mindata;
    }

    while (!it.path.empty()) {
        copy = std::shared_ptr<node>(new node(it.path.top()->data));
        if (my->data < copy->data) {
            copy->right = it.path.top()->right;
            copy->left = prev;
        } else {
            copy->right = prev;
            copy->left = it.path.top()->left;
        }
        it.path.pop();
        prev = copy;
    }

    root = prev;
    invalidate_iterators();
    return;
}

persistent_set::iterator persistent_set::find_max(node_ptr r, std::stack<persistent_set::node_ptr>* path) const {
    while (r != nullptr) {
        path->push(r);
        r = r->right;
    }
    return persistent_set::iterator(*path, *this);
}

persistent_set::iterator persistent_set::find_min(node_ptr r, std::stack<node_ptr>* path) const {
    while (r != nullptr) {
        path->push(r);
        r = r->left;
    }
    return iterator(*path, *this);
}

persistent_set::iterator persistent_set::begin() const {
    if (this->root == nullptr) return this->end();
    return this->find_min(this->root, new std::stack<node_ptr>);
}

persistent_set::iterator persistent_set::end() const {
    std::stack<node_ptr> path = this->find_max(this->root, new std::stack<node_ptr>).path;
    path.push(afterlast);
    return persistent_set::iterator(path, *this);
}

void persistent_set::invalidate_iterators() {
//    for (std::list::iterator it = this->iterators.begin(); it != this->iterators.end(); it++) {
//        while (!it->path.empty()) {
//            it->path.pop();
//        }
//    }
//
//    this->iterators.clear();
}

const persistent_set::value_type &persistent_set::iterator::operator*() const {
    return this->path.top()->data;
}

persistent_set::iterator &persistent_set::iterator::operator++() {
    node_ptr my = this->path.top();

    if (my->right != nullptr) {
        this->path = this->set.find_min(my->right, &(this->path)).path;
        return *this;
    } else if (*this == this->set.end()) {
        return *this;
    } else if (*this == this->set.find_max(this->get_root(), new std::stack<node_ptr>)) {
        this->path = this->set.end().path;
        return *this;
    } else {
        this->path.pop();
        while (!this->path.empty() && this->path.top()->data < my->data) {
            this->path.pop();
        }
        return *this;
    }
}

persistent_set::iterator persistent_set::iterator::operator++(int) {
    persistent_set::iterator it = *this;
    ++(*this);
    return it;
}

persistent_set::iterator &persistent_set::iterator::operator--() {
    node_ptr my = this->path.top();
    if (my->left != nullptr) {
        this->path = this->set.find_max(my->left, &(this->path)).path;
        return *this;
    } else if (*this == this->set.begin()) {
        return *this;
    } else if (*this == this->set.end()) {
        this->path = this->set.find_max(this->get_root(), new std::stack<node_ptr>).path;
        return *this;
    } else {
        this->path.pop();
        while (!this->path.empty() && this->path.top()->data > my->data) {
            this->path.pop();
        }
        return *this;
    }
}

persistent_set::iterator persistent_set::iterator::operator--(int) {
    persistent_set::iterator it = *this;
    --(*this);
    return it;
}

bool operator==(persistent_set::iterator a, persistent_set::iterator b) {
    return (a.get_root() == b.get_root() && a.path.top() == b.path.top());
}

bool operator!=(persistent_set::iterator a, persistent_set::iterator b) {
    return !(a == b);
}

persistent_set::node_ptr persistent_set::iterator::get_root() {
    return this->set.root;
}