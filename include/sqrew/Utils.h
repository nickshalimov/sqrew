#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <tuple>

template<class ValueT>
class Return
{
public:
    enum { size = 1 };

    template<class FuncT, class ...ArgsT>
    Return(FuncT* func, ArgsT&&... args)
        : value_(func(std::forward<ArgsT>(args)...))
    {}

    const ValueT& getValue() const { return value_; }

private:
    ValueT value_;
};

template<>
class Return<void>
{
public:
    enum { size = 0 };

    template<class FuncT, class ...ArgsT>
    Return(FuncT* func, ArgsT&&... args)
    {
        func(std::forward<ArgsT>(args)...);
    }
};

/*
template<class ...ReturnT>
struct ClassImpl::Return<std::tuple<ReturnT...>>
{
    std::tuple<ReturnT...> result;

    template<class FuncT, class ...ArgsT>
    Return(FuncT func, ArgsT&&... args)
        : result(func(std::forward<ArgsT>(args)...))
    {}

    Integer flush(HSQUIRRELVM) { return 0; }
};
*/

#endif // UTILS_H
