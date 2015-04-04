#pragma once
#ifndef SQREW_CLASS_H
#define SQREW_CLASS_H

#include <typeindex>

#include "sqrew/Forward.h"

namespace sqrew {

namespace detail {

class ClassImpl
{
protected:
    using Func = Integer (*)(HSQUIRRELVM);
    using ReleaseHook = Integer (*)(void*, Integer);

    ClassImpl(const Context& context, const String& name, size_t typeTag);
    virtual ~ClassImpl();

    void registerConstructor(Func func);
    void registerMethod(const String& name, Func func);

    static void* createUserData(HSQUIRRELVM v, size_t size, ReleaseHook releaseHook);
    void* createUserData(size_t size, ReleaseHook releaseHook);
    static void* getUserData(HSQUIRRELVM v, Integer index);

    static void setInstance(HSQUIRRELVM v, Integer index, void* instance, ReleaseHook releaseHook);
    static void* getInstance(HSQUIRRELVM v, Integer index, size_t typeTag);

    template<class ValueT>
    static ValueT getValue(HSQUIRRELVM v, Integer index);

    template<class ValueT>
    static void putValue(HSQUIRRELVM v, ValueT value);

    ClassImpl(ClassImpl&& rhs);

    template<class ValueT>
    struct Return;

private:
    struct Detail;

    std::unique_ptr<Detail> detail_;
    const Context& context_;
    const String name_;

    ClassImpl(const ClassImpl&) = delete;
    ClassImpl& operator=(const ClassImpl&) = delete;
    ClassImpl& operator=(ClassImpl&&) = delete;
};

template<class ValueT>
struct ClassImpl::Return
{
    ValueT value;

    template<class FuncT, class ...ArgsT>
    Return(FuncT func, ArgsT&&... args)
        : value(func(std::forward<ArgsT>(args)...))
    {}

    Integer flush(HSQUIRRELVM v)
    {
        ClassImpl::putValue(v, value);
        return 1;
    }
};

template<>
struct ClassImpl::Return<void>
{
    template<class FuncT, class ...ArgsT>
    Return(FuncT func, ArgsT&&... args) { func(std::forward<ArgsT>(args)...); }

    Integer flush(HSQUIRRELVM) { return 0; }
};
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
}

template<class ClassT>
struct DefaultAllocator
{
    using Pointer = ClassT*;

    template<class ...ArgsT>
    inline static Pointer createInstance(ArgsT&&... args) { return new ClassT(std::forward<ArgsT>...); }

    inline static void destroyInstance(Pointer instance) { delete instance; }

    inline static ClassT* castInstance(Pointer ptr) { return ptr; }
};

template<class ClassT, template<class> class AllocatorT = DefaultAllocator>
class Class: protected detail::ClassImpl
{
    static constexpr size_t getTypeTag() { return typeid(ClassT).hash_code(); }

    using Allocator = AllocatorT<ClassT>;

    template<class ReturnT, class ...ArgsT>
    using Method = ReturnT (ClassT::*)(ArgsT...);

    template<class ReturnT, class ...ArgsT>
    using ConstMethod = ReturnT (ClassT::*)(ArgsT...) const;

    template<class ReturnT, class ...ArgsT>
    using ExtensionMethod = ReturnT (*)(typename Allocator::Pointer, ArgsT...);

    template<class FieldT>
    using Field = FieldT (ClassT::*);

    template<class ReturnT, class ...ArgsT>
    class MethodDelegate;

    static Integer destroyInstance(void* ptr, Integer)
    {
        Allocator::destroyInstance(static_cast<typename Allocator::Pointer>(ptr));
        return 0;
    }

    template<class ...ArgsT>
    static Integer createInstance(HSQUIRRELVM v)
    {
        Integer index = 2;
        setInstance(v, 1, Allocator::createInstance(getValue<ArgsT>(v, index++)...), destroyInstance);
        return 0;
    }

    template<class ReturnT, class ...ArgsT>
    static Integer callMethodDelegate(HSQUIRRELVM v)
    {
        auto instance = static_cast<typename Allocator::Pointer>(getInstance(v, 1, getTypeTag()));
        auto method = static_cast<MethodDelegate<ReturnT, ArgsT...>*>(getUserData(v, -1));

        Integer index = 2;
        auto result = method->invoke(Allocator::castInstance(instance), getValue<ArgsT>(v, index++)...);
        return result.flush(v);
    }

    template<class ReturnT, class ...ArgsT>
    static Integer releaseMethodDelegate(void* ptr, Integer)
    {
        auto instance = reinterpret_cast<MethodDelegate<ReturnT, ArgsT...>*>(ptr);
        instance->~MethodDelegate<ReturnT, ArgsT...>();
        return 0;
    }

    template<class ReturnT, class ...ArgsT>
    void addDelegate(const String& name, MethodDelegate<ReturnT, ArgsT...>&& methodDelegate)
    {
        auto ptr = createUserData(sizeof(MethodDelegate<ReturnT, ArgsT...>), releaseMethodDelegate<ReturnT, ArgsT...>);
        *reinterpret_cast<MethodDelegate<ReturnT, ArgsT...>*>(ptr) = std::move(methodDelegate);
        registerMethod(name, callMethodDelegate<ReturnT, ArgsT...>);
    }

public:
    static Class expose(const Context& context, const String& fullName)
    {
        return Class(context, fullName);
    }

    template<class ...ArgsT>
    Class& setConstructor()
    {
        registerConstructor(createInstance<ArgsT...>);
        return *this;
    }

    template<class ReturnT, class ...ArgsT>
    Class& setMethod(const String& name, Method<ReturnT, ArgsT...> method)
    {
        addDelegate(name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class ReturnT, class ...ArgsT>
    Class& setMethod(const String& name, ConstMethod<ReturnT, ArgsT...> method)
    {
        addDelegate(name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class ReturnT, class ...ArgsT>
    Class& setMethod(const String& name, ExtensionMethod<ReturnT, ArgsT...> method)
    {
        addDelegate(name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, Field<FieldT> field)
    {
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, ConstMethod<FieldT> getter, Method<void, FieldT> setter)
    {
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, Method<FieldT> getter, Method<void, FieldT> setter)
    {
        return *this;
    }

private:
    Class(const Context& context, const String& path)
        : detail::ClassImpl(context, path, getTypeTag()) {}

    Class(Class&& rhs)
        : detail::ClassImpl(std::forward<detail::ClassImpl>(rhs)) {}
};

template<class ClassT, template<class> class AllocatorT>
template<class ReturnT, class ...ArgsT>
class Class<ClassT, AllocatorT>::MethodDelegate
{
public:
    using Method = Class<ClassT, AllocatorT>::Method<ReturnT, ArgsT...>;
    using ConstMethod = Class<ClassT, AllocatorT>::ConstMethod<ReturnT, ArgsT...>;
    using ExtensionMethod = Class<ClassT, AllocatorT>::ExtensionMethod<ReturnT, ArgsT...>;

    explicit MethodDelegate(Method method)
        : method_(method)
        , invoke_(invokeMethod)
    {}

    explicit MethodDelegate(ConstMethod method)
        : constMethod_(method)
        , invoke_(invokeConstMethod)
    {}

    explicit MethodDelegate(ExtensionMethod method)
        : extensionMethod_(method)
        , invoke_(invokeExtensionMethod)
    {}

    Return<ReturnT> invoke(ClassT* instance, ArgsT&&... args)
    {
        return Return<ReturnT>(invoke_, this, instance, std::forward<ArgsT>(args)...);
    }

private:
    using MethodInvoke = ReturnT (*)(MethodDelegate* self, ClassT* instance, ArgsT&&... args);

    Method method_;
    ConstMethod constMethod_;
    ExtensionMethod extensionMethod_;
    MethodInvoke invoke_;

    static ReturnT invokeMethod(MethodDelegate* self, ClassT* instance, ArgsT&&... args)
    {
        return (instance->*self->method_)(std::forward<ArgsT>(args)...);
    }

    static ReturnT invokeConstMethod(MethodDelegate* self, ClassT* instance, ArgsT&&... args)
    {
        return (instance->*self->constMethod_)(std::forward<ArgsT>(args)...);
    }

    static ReturnT invokeExtensionMethod(MethodDelegate* self, ClassT* instance, ArgsT&&... args)
    {
        return self->extensionMethod_(instance, std::forward<ArgsT>(args)...);
    }
};

} // namespace sqrew

#endif // SQREW_CLASS_H
