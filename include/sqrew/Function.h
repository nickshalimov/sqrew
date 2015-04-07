#pragma once
#ifndef SQREW_FUNCTION_H
#define SQREW_FUNCTION_H

#include "sqrew/Utils.h"

namespace sqrew {
/*
template<class ReturnT, class... ArgsT>
ReturnT dummyCall(ArgsT&&...)
{
    return ReturnT();
}

template<class... ArgsT>
void dummyCall<void, ArgsT...>(ArgsT&&...)
{
}*/

static void intCall(int)
{

}

template<class ReturnT>
class Function
{
public:
    Function() {}

    template<class... ArgsT>
    Return<ReturnT> operator()(ArgsT&&... args)
    {
        //return Return<ReturnT>(dummyCall<ReturnT, ArgsT...>, std::forward<ArgsT>(args)...);
        return Return<ReturnT>(intCall, 12);
    }

    bool isValid() const
    {
        return true;
    }
};

} // namespace sqrew

#endif // SQREW_FUNCTION_H
