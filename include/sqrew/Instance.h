#pragma once
#ifndef SQREW_INSTANCE_H
#define SQREW_INSTANCE_H

#include "sqrew/Forward.h"
#include "sqrew/Function.h"

namespace sqrew {

class Instance
{
public:
    Instance(const Context& context, const String& className);
    ~Instance();


    template<class ReturnT = void>
    Function<ReturnT> getMethod(const String& name)
    {
        return Function<ReturnT>();
    }

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace sqrew

#endif // SQREW_INSTANCE_H
