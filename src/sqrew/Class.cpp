#include "sqrew/Class.h"

#include "sqrew/Context.h"
#include "sqrew/Table.h"

#include <squirrel.h>

namespace sqrew {
namespace detail {

struct ClassImpl::Detail
{
    HSQOBJECT classObject;
    HSQOBJECT setTable;
    HSQOBJECT getTable;

    bool isConstructorSet = false;

    Detail()
    {
        sq_resetobject(&classObject);
        sq_resetobject(&setTable);
        sq_resetobject(&getTable);
    }

    ~Detail() {}

    void registerClosure(HSQUIRRELVM v, ClosureType type, const String& name, Func func)
    {
        const HSQOBJECT* registerAs[] = { &classObject, &setTable, &getTable };

        sq_pushobject(v, *registerAs[static_cast<int>(type)]);
        sq_pushstring(v, name.c_str(), name.size());
        sq_push(v, -3);
        sq_newclosure(v, func, 1);
        sq_setnativeclosurename(v, -1, name.c_str());
        sq_newslot(v, -3, SQFalse);
        sq_pop(v, 2);
    }

    static int set(HSQUIRRELVM v)
    {
        sq_push(v, 2);
        if (SQ_FAILED( sq_get(v, -2) ))
        {
            //String errorMessage
            return sq_throwerror(v, _SC("the index '' does not exist"));
        }

        sq_push(v, 1);
        sq_push(v, 3);

        sq_call(v, 2, SQFalse, SQTrue);
        return 0;
    }

    static int get(HSQUIRRELVM v)
    {
        sq_push(v, 2);
        if (SQ_FAILED( sq_get(v, -2) ))
        {
            //String errorMessage
            return sq_throwerror(v, _SC("the index '' does not exist"));
        }

        sq_push(v, 1);

        sq_call(v, 1, SQTrue, SQTrue);
        return 1;
    }
};

ClassImpl::ClassImpl(const Context& context)
    : detail_(new Detail())
	, context_(context)
{}

ClassImpl::~ClassImpl() {}

void ClassImpl::initialize(const String& name, size_t typeTag)
{
    auto v = context_.getHandle();



    //auto rootTable = Table::getRoot(context_);
    //rootTable.createClass(name);

    sq_pushroottable(v);

    sq_pushstring(v, name.c_str(), name.size());
    sq_newclass(v, SQFalse);
    sq_settypetag(v, -1, reinterpret_cast<SQUserPointer>(typeTag));
    sq_getstackobj(v, -1, &detail_->classObject);
    sq_newslot(v, -3, SQFalse);

    sq_pop(v, 1);

    //auto classRegistry = Table::

    /*
    auto classesTable = Table::create(context_, _SC("__sqrew_classes"), TableDomain::Registry);
    auto classRegistry = classesTable.createTable(name);
    auto setTable = classRegistry.create("set");
    auto getTable = classRegistry.create("get");
    */

    sq_pushregistrytable(v);

    sq_pushstring(v, _SC("__sqrew_classes"), -1);
    sq_get(v, -2);

    HSQOBJECT registryTable;
    sq_resetobject(&registryTable);
    sq_pushstring(v, name.c_str(), name.size());
    sq_newtable(v);
    sq_getstackobj(v, -1, &registryTable);
    sq_newslot(v, -3, SQTrue);

    sq_pop(v, 2);

    sq_pushobject(v, registryTable);

    sq_pushstring(v, _SC("set"), -1);
    sq_newtable(v);
    sq_getstackobj(v, -1, &detail_->setTable);
    sq_newslot(v, -3, SQTrue);

    sq_pushstring(v, _SC("get"), -1);
    sq_newtable(v);
    sq_getstackobj(v, -1, &detail_->getTable);
    sq_newslot(v, -3, SQTrue);

    sq_pop(v, 1);

    sq_pushobject(v, detail_->classObject);

    sq_pushstring(v, _SC("_set"), -1);
    sq_pushobject(v, detail_->setTable);
    sq_newclosure(v, Detail::set, 1);
    sq_newslot(v, -3, SQFalse);

    sq_pushstring(v, _SC("_get"), -1);
    sq_pushobject(v, detail_->getTable);
    sq_newclosure(v, Detail::get, 1);
    sq_newslot(v, -3, SQFalse);

    sq_pop(v, 1);
}

void ClassImpl::registerConstructor(Func func)
{
    auto v = context_.getHandle();

    if (detail_->isConstructorSet)
        throw std::runtime_error("Class can have only one constructor");

    sq_pushobject(v, detail_->classObject);
    sq_pushstring(v, _SC("constructor"), -1);
    sq_newclosure(v, func, 0);
    sq_newslot(v, -3, false);
    sq_pop(v, 1);

    detail_->isConstructorSet = true;
}

void ClassImpl::registerClosure(ClassImpl::ClosureType type, const String& name, ClassImpl::Func func)
{
    auto v = context_.getHandle();
    detail_->registerClosure(v, type, name, func);
}

void* ClassImpl::createUserData(HSQUIRRELVM v, size_t size, ReleaseHook releaseHook)
{
    auto ptr = sq_newuserdata(v, size);
    if (releaseHook != nullptr)
        sq_setreleasehook(v, -1, releaseHook);
    return ptr;
}

void *ClassImpl::createUserData(size_t size, ClassImpl::ReleaseHook releaseHook)
{
    return createUserData(context_.getHandle(), size, releaseHook);
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
	: detail_(std::move(rhs.detail_))
    , context_(std::move(rhs.context_))
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
