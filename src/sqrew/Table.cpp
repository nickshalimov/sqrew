#include "sqrew/Table.h"

#include "sqrew/Context.h"
#include "sqrew/Utils.h"

#include <squirrel.h>

namespace sqrew {

struct Table::Impl
{
    const Context& context;
    HSQOBJECT object;

    explicit Impl(const Context& ctx)
        : context(ctx)
    {
        sq_resetobject(&object);
    }

    void create(const String& path)
    {
        auto v = context.getHandle();

        HSQOBJECT tableObject;
        sq_resetobject(&tableObject);

        auto pathItems = splitPath(path);
        for (const auto& name: pathItems)
        {
            sq_pushstring(v, name.c_str(), name.size());
            if (SQ_FAILED( sq_get(v, -2) ))
            {
                sq_pushstring(v, name.c_str(), name.size());
                sq_newtable(v);
                sq_getstackobj(v, -1, &tableObject);
                sq_newslot(v, -3, SQTrue);
                sq_pushobject(v, tableObject);
            }
            else
            {
                sq_getstackobj(v, -1, &tableObject);
                if (!sq_istable(tableObject))
                    throw std::runtime_error(std::string("Can't create table at path ") + path);
            }
        }

        sq_pop(v, pathItems.size());

        object = tableObject;
    }
};

Table Table::create(const Context& context, const String& path, TableDomain domain)
{
    auto v = context.getHandle();

    if (domain == TableDomain::Script)
        sq_pushroottable(v);
    else if (domain == TableDomain::Registry)
        sq_pushregistrytable(v);

    Table table(context);
    table.impl_->create(path);

    sq_pop(v, 1);

    return std::move(table);
}

Table Table::getRoot(const Context& context, TableDomain domain)
{
    auto v = context.getHandle();

    if (domain == TableDomain::Script)
        sq_pushroottable(v);
    else if (domain == TableDomain::Registry)
        sq_pushregistrytable(v);

    Table table(context);
    sq_getstackobj(v, -1, &table.impl_->object);

    sq_pop(v, 1);

    return std::move(table);
}

Table Table::createTable(const String& path) const
{
    auto v = impl_->context.getHandle();
    sq_pushobject(v, impl_->object);

    Table table(impl_->context);
    table.impl_->create(path);

    sq_pop(v, 1);

    return std::move(table);
}

void Table::put() const
{
    auto v = impl_->context.getHandle();
    sq_pushobject(v, impl_->object);
}

Table::Table(const Context& context)
    : impl_(new Impl(context))
{}

Table::Table(Table&& rhs)
    : impl_(std::move(rhs.impl_))
{}

Table::~Table() {}

} // namespace sqrew
