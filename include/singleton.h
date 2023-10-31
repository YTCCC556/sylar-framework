//
// Created by YTCCC on 2023/10/30.
//

#ifndef SYLAR_FRAMEWORK_SINGLETON_H
#define SYLAR_FRAMEWORK_SINGLETON_H

#include <memory>
namespace ytccc {
template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T *GetInstance() {
        static T v;
        return &v;
    }
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}// namespace ytccc


#endif//SYLAR_FRAMEWORK_SINGLETON_H
