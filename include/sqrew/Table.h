#pragma once
#ifndef SQREW_TABLE_H
#define SQREW_TABLE_H

#include "sqrew/Forward.h"

namespace sqrew {

enum class TableDomain { Script = 0, Registry, Const };

class Table final
{
public:
    Table(Table&& rhs);
    ~Table();

    static Table get(const Context& context, const String& path, TableDomain domain = TableDomain::Script);

    static Table create(const Context& context, const String& path, TableDomain domain = TableDomain::Script);

    static Table getRoot(const Context& context, TableDomain domain = TableDomain::Script);

    //static Table create(const Table& context, const String& name, TableDomain domain = TableDomain::Script);
    //static bool create(const Context& context, const String& name, TableDomain domain = TableDomain::Script);

    bool isValid() const;

    Table createTable(const String& path) const;

    bool contains(const String& name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    explicit Table(const Context& context);
};

} // namespace sqrew

#endif // SQREW_TABLE_H
