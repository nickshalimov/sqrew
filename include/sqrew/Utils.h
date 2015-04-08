#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "sqrew/Forward.h"

#include <tuple>
#include <vector>
#include <sstream>

namespace sqrew {

template<class ValueT>
class Return
{
public:
    enum { size = 1 };

    template<class FuncT, class ...ArgsT>
    Return(FuncT* func, ArgsT&&... args)
        : value_(func(std::forward<ArgsT>(args)...))
    {}

    inline const ValueT& getValue() const { return value_; }

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

inline std::vector<String> splitPath(const String& path)
{
    auto delim = '.';

    std::basic_istringstream<String::value_type> stream(path);
    std::vector<String> items;
    String string;

    while (std::getline(stream, string, delim))
        items.push_back(string);

    return std::move(items);
}

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

} // namespace sqrew

#endif // UTILS_H
