#include "sqrew/Context.h"

#include "sqrew/Interface.h"
#include "sqrew/Table.h"

#include <cstdarg>
#include <cstdio>

#include <vector>

#include <squirrel.h>

#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>
#include <sqstdaux.h>
#include <sqstdio.h>

namespace sqrew {

struct Context::Detail
{
    static void print(HSQUIRRELVM vm, const SQChar* format, ...);
    static void printError(HSQUIRRELVM vm, const SQChar* format, ...);

    static SQInteger handleError(HSQUIRRELVM vm);

    static void handleCompilerError(HSQUIRRELVM vm,
                                    const SQChar* error,
                                    const SQChar* source,
                                    SQInteger line,
                                    SQInteger column);

    static Context* getContext(HSQUIRRELVM vm);
};

namespace {

class StackGuard
{
public:
    explicit StackGuard(HSQUIRRELVM vm)
        : vm_(vm), top_(sq_gettop(vm))
    {}

    ~StackGuard() { sq_settop(vm_, top_); }

private:
    HSQUIRRELVM vm_;
    SQInteger top_;
};

}

Context::Context()
    : Context(1024)
{
}

Context::Context(int stackSize)
    : vm_(sq_open(stackSize))
{

}

Context::~Context()
{
    sq_close(vm_);
}

void Context::initialize()
{
    sq_setforeignptr(vm_, this);

    sq_setprintfunc(vm_, Detail::print, Detail::printError);

    sq_pushroottable(vm_);

    sqstd_register_mathlib(vm_);
    sqstd_register_stringlib(vm_);
    sqstd_register_bloblib(vm_);

    sq_setcompilererrorhandler(vm_, Detail::handleCompilerError);
    sq_newclosure(vm_, Detail::handleError, 0);
    sq_seterrorhandler(vm_);

    sq_pop(vm_, 1);

    Table::create(*this, _SC("__sqrew_classes"), TableDomain::Registry);
}

bool Context::executeBuffer(const String& buffer) const
{
    return executeBuffer(buffer, "?");
}

bool Context::executeBuffer(const String& buffer, const String& source) const
{
    StackGuard guard(vm_); (void)guard;
    sq_pushroottable(vm_);

    if (SQ_FAILED( sq_compilebuffer(vm_, buffer.c_str(), buffer.size(), source.c_str(), SQTrue) ))
        return false;

    sq_push(vm_, -2);
    return SQ_SUCCEEDED( sq_call(vm_, 1, SQFalse, SQTrue) );
}

void Context::Detail::print(HSQUIRRELVM vm, const SQChar* format, ...)
{
    Context* context = getContext(vm);

    if (!context->interface_)
        return;

    va_list args;
    va_start(args, format);
    const int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    va_start(args, format);
    std::vector<char> chars(size + 1, 0);
    vsnprintf(&chars[0], size + 1, format, args);
    va_end(args);

    context->interface_->print(String(chars.begin(), chars.end()));
}

void Context::Detail::printError(HSQUIRRELVM vm, const SQChar* format, ...)
{
    Context* context = getContext(vm);

    if (!context->interface_)
        return;

    va_list args;
    va_start(args, format);
    const int size = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    va_start(args, format);
    std::vector<char> chars(size + 1, 0);
    vsnprintf(&chars[0], size + 1, format, args);
    va_end(args);

    context->interface_->print(String(chars.begin(), chars.end()));
}

SQInteger Context::Detail::handleError(HSQUIRRELVM vm)
{
    const SQChar* error = 0;
    if (sq_gettop(vm) >= 1)
    {
        if (SQ_SUCCEEDED(sq_getstring(vm, 2, &error)))
            printError(vm, _SC("[%s]"), error);
        else
            printError(vm, _SC("unknown"));

        sqstd_printcallstack(vm);
    }

    return SQ_ERROR;
}

void Context::Detail::handleCompilerError(HSQUIRRELVM vm,
                                          const SQChar* error,
                                          const SQChar* source,
                                          SQInteger line,
                                          SQInteger column)
{

}

Context* Context::Detail::getContext(HSQUIRRELVM vm)
{
    return static_cast<Context*>(sq_getforeignptr(vm));
}

} // namespace sqrew
