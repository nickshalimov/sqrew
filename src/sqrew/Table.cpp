#include "sqrew/Table.h"

#include "sqrew/Context.h"
#include "sqrew/Utils.h"

#include <squirrel.h>

namespace sqrew {

inline static void pushDomainTable(HSQUIRRELVM v, TableDomain domain)
{
    switch (domain)
    {
    case TableDomain::Script: sq_pushroottable(v); break;
    case TableDomain::Registry: sq_pushregistrytable(v); break;
    case TableDomain::Const: sq_pushconsttable(v); break;
    }
}

struct Table::Impl
{
    const Context& context;
    HSQOBJECT object;

    explicit Impl(const Context& ctx)
        : context(ctx)
    {
        sq_resetobject(&object);
    }

    ~Impl()
    {
        if (isValid())
            sq_release(context.getHandle(), &object);
    }

    inline bool isValid() const
    {
        return sq_istable(object);
    }

    void setFromTop()
    {
        auto v = context.getHandle();

        if (isValid())
            sq_release(v, &object);

        sq_getstackobj(v, -1, &object);

        if (isValid())
            sq_addref(v, &object);
    }

    void setFromObject(HSQOBJECT& obj)
    {
        auto v = context.getHandle();

        if (isValid())
            sq_release(v, &object);

        object = obj;

        if (isValid())
            sq_addref(v, &object);
    }

    void create(const String& path)
    {
        StackLock lock(context);

        auto v = context.getHandle();

        HSQOBJECT tableObject;
        sq_resetobject(&tableObject);

        const bool created = iteratePath(path, [&](const String& name) -> bool
        {
            sq_pushstring(v, name.c_str(), name.size());
            if (SQ_FAILED( sq_get(v, -2) ))
            {
                sq_pushstring(v, name.c_str(), name.length());
                sq_newtable(v);
                sq_getstackobj(v, -1, &tableObject);
                sq_newslot(v, -3, SQTrue);
                sq_pushobject(v, tableObject);
                return true;
            }

            sq_getstackobj(v, -1, &tableObject);
            return sq_istable(tableObject);
        });

        if (!created)
            throw std::runtime_error(std::string("Can't create table at path ") + path);

        setFromObject(tableObject);
    }
};

Table Table::get(const Context& context, const String& path, TableDomain domain)
{
    StackLock lock(context);

    auto v = context.getHandle();
    pushDomainTable(v, domain);

    const bool found = iteratePath(path, [=](const String& name) -> bool
    {
        sq_pushstring(v, name.c_str(), name.size());
        return SQ_SUCCEEDED( sq_get(v, -2) );
    });

    Table table(context);

    if (found)
        table.impl_->setFromTop();

    return std::move(table);
}

Table Table::create(const Context& context, const String& path, TableDomain domain)
{
    StackLock lock(context);

    auto v = context.getHandle();

    pushDomainTable(v, domain);

    Table table(context);
    table.impl_->create(path);
    return std::move(table);
}

Table Table::getRoot(const Context& context, TableDomain domain)
{
    StackLock lock(context);

    auto v = context.getHandle();

    pushDomainTable(v, domain);

    Table table(context);
    table.impl_->setFromTop();
    return std::move(table);
}

bool Table::isValid() const
{
    return impl_->isValid();
}

Table Table::createTable(const String& path) const
{
    if (!isValid())
        return Table(impl_->context);

    StackLock lock(impl_->context);

    sq_pushobject(impl_->context.getHandle(), impl_->object);

    Table table(impl_->context);
    table.impl_->create(path);
    return std::move(table);
}

bool Table::contains(const String& name) const
{
    if (!isValid())
        return false;

    StackLock lock(impl_->context);

    auto v = impl_->context.getHandle();

    sq_pushobject(v, impl_->object);
    sq_pushstring(v, name.c_str(), name.length());
    return SQ_SUCCEEDED( sq_get(v, -2) );
}

Table::Table(const Context& context)
    : impl_(new Impl(context))
{}

Table::Table(Table&& rhs)
    : impl_(std::move(rhs.impl_))
{}

Table::~Table() {}

} // namespace sqrew
