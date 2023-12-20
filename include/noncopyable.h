//
// Created by YTCCC on 2023/12/18.
//

#ifndef YTCCC_MODULE_NONCOPYABLE_H
#define YTCCC_MODULE_NONCOPYABLE_H

namespace ytccc {
class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};
}// namespace ytccc

#endif//YTCCC_MODULE_NONCOPYABLE_H
