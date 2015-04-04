#include "sqrew/Class.h"

#include "sqrew/Context.h"

#include <squirrel.h>


namespace sqrew {
namespace detail {

struct ClassImpl::Detail
{
    HSQOBJECT classObject;
    SQUserPointer typeTag;
};

ClassImpl::ClassImpl(const Context& context, const String& name, size_t typeTag)
    : detail_(new Detail()), context_(context), name_(name)
{
    detail_->typeTag = reinterpret_cast<SQUserPointer>(typeTag);
    sq_resetobject(&detail_->classObject);

    auto v = context_.getHandle();

    sq_pushroottable(v);

    sq_pushstring(v, name.c_str(), name.size());
    sq_newclass(v, false);
    sq_settypetag(v, -1, detail_->typeTag);
    sq_getstackobj(v, -1, &detail_->classObject);
    sq_newslot(v, -3, SQFalse);

    sq_pop(v, 1);
}

ClassImpl::~ClassImpl()
{
    sq_resetobject(&detail_->classObject);
}

void ClassImpl::registerConstructor(Func func)
{
    auto v = context_.getHandle();

    sq_pushobject(v, detail_->classObject);
    sq_pushstring(v, _SC("constructor"), -1);
    sq_newclosure(v, func, 0);
    sq_newslot(v, -3, false);
    sq_pop(v, 1);
}

void ClassImpl::registerMethod(const String& name, Func func)
{
    auto v = context_.getHandle();

    sq_pushobject(v, detail_->classObject);
    sq_pushstring(v, name.c_str(), name.size());
    sq_push(v, -3);
    sq_newclosure(v, func, 1);
    sq_setnativeclosurename(v, -1, name.c_str());
    sq_newslot(v, -3, false);
    sq_pop(v, 2);
}

void* ClassImpl::createUserData(HSQUIRRELVM v, size_t size, ReleaseHook releaseHook)
{
    auto ptr = sq_newuserdata(v, size);
    sq_setreleasehook(v, -1, releaseHook);
    return ptr;
}

void *ClassImpl::createUserData(size_t size, ClassImpl::ReleaseHook releaseHook)
{
    return createUserData(context_.getHandle(), size, releaseHook);
}

void* ClassImpl::getCalleeInstance(HSQUIRRELVM v)
{
    SQUserPointer ptr;
    sq_getuserdata(v, 1, &ptr, nullptr);
    return ptr;
}

void* ClassImpl::getUserData(HSQUIRRELVM v, Integer index)
{
    SQUserPointer ptr;
    sq_getuserdata(v, index, &ptr, nullptr);
    return ptr;
}

void ClassImpl::setInstance(HSQUIRRELVM v, Integer index, void* instance, ReleaseHook releaseHook)
{
    sq_setinstanceup(v, index, instance);
    sq_setreleasehook(v, index, releaseHook);
}

void* ClassImpl::getInstance(HSQUIRRELVM v, Integer index, size_t typeTag)
{
    SQUserPointer ptr;
    sq_getinstanceup(v, index, &ptr, reinterpret_cast<SQUserPointer>(typeTag));
    return ptr;
}

ClassImpl::ClassImpl(ClassImpl&& rhs)
    : detail_(std::move(detail_))
    , context_(std::move(rhs.context_))
    , name_(std::move(rhs.name_))
{}

template<>
Integer ClassImpl::getValue<Integer>(HSQUIRRELVM v, Integer index)
{
    SQInteger value;
    if (SQ_FAILED( sq_getinteger(v, index, &value) ))
        sq_throwerror(v, _SC("Invalid argument type"));

    return value;
}

template<>
Float ClassImpl::getValue<Float>(HSQUIRRELVM v, Integer index)
{
    SQFloat value;
    if (SQ_FAILED( sq_getfloat(v, index, &value) ))
        sq_throwerror(v, _SC("Invalid argument type"));

    return value;
}

template<>
void ClassImpl::putValue<Integer>(HSQUIRRELVM v, Integer value)
{
    sq_pushinteger(v, value);
}

template<>
void ClassImpl::putValue<Float>(HSQUIRRELVM v, Float value)
{
    sq_pushfloat(v, value);
}

} // namespace detail
} // namespace sqrew
