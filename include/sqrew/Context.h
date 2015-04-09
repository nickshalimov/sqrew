#pragma once
#ifndef SQREW_CONTEXT_H
#define SQREW_CONTEXT_H

#include "sqrew/Forward.h"

namespace sqrew {

class Context final
{
public:
    Context();
    explicit Context(int stackSize);
    ~Context();

    inline HSQUIRRELVM getHandle() const { return vm_; }

    void initialize();

    template<class InterfaceT, class ...ArgsT>
    inline void setInterface(ArgsT&&... args)
    {
        interface_.reset(new InterfaceT(std::forward<ArgsT>(args)...));
    }

    bool executeBuffer(const String& buffer) const;
    bool executeBuffer(const String& buffer, const String& source) const;

private:
    struct Detail;

    HSQUIRRELVM vm_;
    std::unique_ptr<Interface> interface_;
};

class StackLock final
{
public:
    explicit StackLock(const Context& ctx);
    ~StackLock();

private:
    const Context& context_;
    Integer top_;

    StackLock(const StackLock&) = delete;
    StackLock(StackLock&&) = delete;
    StackLock& operator=(const StackLock&) = delete;
    StackLock& operator=(StackLock&&) = delete;
};

} // namespace sqrew

#endif // SQREW_CONTEXT_H
