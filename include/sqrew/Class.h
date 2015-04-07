#pragma once
#ifndef SQREW_CLASS_H
#define SQREW_CLASS_H

#include <typeindex>

#include "sqrew/Forward.h"
#include "sqrew/Utils.h"

namespace sqrew {

namespace detail {

class ClassImpl
{
protected:
    enum class ClosureType { Method = 0, Setter, Getter };

    using Func = Integer (*)(HSQUIRRELVM);
    using ReleaseHook = Integer (*)(void*, Integer);

    ClassImpl(const Context& context);
    virtual ~ClassImpl();

    void initialize(const String& name, size_t typeTag);

    void registerConstructor(Func func);
    void registerClosure(ClosureType type, const String& name, Func func);

    static void* createUserData(HSQUIRRELVM v, size_t size, ReleaseHook releaseHook);
    void* createUserData(size_t size, ReleaseHook releaseHook);
    static void* getUserData(HSQUIRRELVM v, Integer index);

    static void setInstance(HSQUIRRELVM v, Integer index, void* instance, ReleaseHook releaseHook);
    static void* getInstance(HSQUIRRELVM v, Integer index, size_t typeTag);

    template<class ValueT>
    static ValueT getValue(HSQUIRRELVM v, Integer index);

    template<class ValueT>
    static void putValue(HSQUIRRELVM v, ValueT value);

    template<class ValueT>
    static void putReturn(HSQUIRRELVM v, const Return<ValueT>& ret);

    ClassImpl(ClassImpl&& rhs);

private:
    struct Detail;

    std::unique_ptr<Detail> detail_;
    const Context& context_;

    ClassImpl(const ClassImpl&) = delete;
    ClassImpl& operator=(const ClassImpl&) = delete;
    ClassImpl& operator=(ClassImpl&&) = delete;
};

template<class ValueT>
inline void ClassImpl::putReturn(HSQUIRRELVM v, const Return<ValueT>& ret)
{
    putValue<ValueT>(v, ret.getValue());
}

template<>
inline void ClassImpl::putReturn(HSQUIRRELVM, const Return<void>&) {}

}

template<class ClassT>
struct DefaultAllocator
{
    using Pointer = ClassT*;

    template<class ...ArgsT>
    inline static Pointer createInstance(ArgsT&&... args) { return new ClassT(std::forward<ArgsT>(args)...); }

    inline static void destroyInstance(Pointer instance) { delete instance; }

    inline static ClassT* castInstance(Pointer ptr) { return ptr; }
};

template<class ClassT, template<class> class AllocatorT = DefaultAllocator>
class Class: protected detail::ClassImpl
{
    static size_t getTypeTag() { return typeid(ClassT).hash_code(); }

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
    static Integer callMethod(HSQUIRRELVM v)
    {
        auto instance = static_cast<typename Allocator::Pointer>(getInstance(v, 1, getTypeTag()));
        auto method = static_cast<MethodDelegate<ReturnT, ArgsT...>*>(getUserData(v, -1));

        Integer index = 2;
        auto result = method->invoke(Allocator::castInstance(instance), getValue<ArgsT>(v, index++)...);
        putReturn<ReturnT>(v, result);
        return Return<ReturnT>::size;
    }

    template<class FieldT>
    static Integer callSetter(HSQUIRRELVM v)
    {
        auto instance = static_cast<typename Allocator::Pointer>(getInstance(v, 1, getTypeTag()));
        auto field = static_cast<Field<FieldT>*>(getUserData(v, -1));

        Allocator::castInstance(instance)->*(*field) = getValue<FieldT>(v, 2);
        return 0;
    }

    template<class FieldT>
    static Integer callGetter(HSQUIRRELVM v)
    {
        auto instance = static_cast<typename Allocator::Pointer>(getInstance(v, 1, getTypeTag()));
        auto field = static_cast<Field<FieldT>*>(getUserData(v, -1));

        putValue(v, Allocator::castInstance(instance)->*(*field));
        return 1;
    }

    template<class ReturnT, class ...ArgsT>
    static Integer releaseMethod(void* ptr, Integer)
    {
        auto instance = reinterpret_cast<MethodDelegate<ReturnT, ArgsT...>*>(ptr);
        instance->~MethodDelegate<ReturnT, ArgsT...>();
        return 0;
    }

    template<class ReturnT, class ...ArgsT>
    void addMethod(ClosureType type, const String& name, MethodDelegate<ReturnT, ArgsT...>&& methodDelegate)
    {
        auto ptr = createUserData(sizeof(MethodDelegate<ReturnT, ArgsT...>), releaseMethod<ReturnT, ArgsT...>);
        *reinterpret_cast<MethodDelegate<ReturnT, ArgsT...>*>(ptr) = std::move(methodDelegate);
        registerClosure(type, name, callMethod<ReturnT, ArgsT...>);
    }

    template<class FieldT>
    void addSetter(const String& name, Field<FieldT> field)
    {
        auto ptr = createUserData(sizeof(Field<FieldT>), nullptr);
        *reinterpret_cast<Field<FieldT>*>(ptr) = field;
        registerClosure(ClosureType::Setter, name, callSetter<FieldT>);
    }

    template<class FieldT>
    void addGetter(const String& name, Field<FieldT> field)
    {
        auto ptr = createUserData(sizeof(Field<FieldT>), nullptr);
        *reinterpret_cast<Field<FieldT>*>(ptr) = field;
        registerClosure(ClosureType::Getter, name, callGetter<FieldT>);
    }

public:
    static Class expose(const Context& context, const String& fullName)
    {
		Class definition(context);
		definition.initialize(fullName, getTypeTag());
		return std::move(definition);
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
        addMethod(ClosureType::Method, name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class ReturnT, class ...ArgsT>
    Class& setMethod(const String& name, ConstMethod<ReturnT, ArgsT...> method)
    {
        addMethod(ClosureType::Method, name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class ReturnT, class ...ArgsT>
    Class& setMethod(const String& name, ExtensionMethod<ReturnT, ArgsT...> method)
    {
        addMethod(ClosureType::Method, name, MethodDelegate<ReturnT, ArgsT...>(method));
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, Field<FieldT> field)
    {
        addSetter(name, field);
        addGetter(name, field);
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, Method<void, FieldT> setter, ConstMethod<FieldT> getter)
    {
        addMethod(ClosureType::Setter, name, MethodDelegate<void, FieldT>(setter));
        addMethod(ClosureType::Getter, name, MethodDelegate<FieldT>(getter));
        return *this;
    }

    template<class FieldT>
    Class& setField(const String& name, Method<void, FieldT> setter, Method<FieldT> getter)
    {
        addMethod(ClosureType::Setter, name, MethodDelegate<void, FieldT>(setter));
        addMethod(ClosureType::Getter, name, MethodDelegate<FieldT>(getter));
        return *this;
    }

private:
    Class(const Context& context)
        : detail::ClassImpl(context) {}

    Class(Class&& rhs)
        : detail::ClassImpl(std::move(rhs)) {}
};

template<class ClassT, template<class> class AllocatorT>
template<class ReturnT, class ...ArgsT>
class Class<ClassT, AllocatorT>::MethodDelegate
{
public:
#if defined(__GNUC__)
    using Method = Class<ClassT, AllocatorT>::Method<ReturnT, ArgsT...>;
    using ConstMethod = Class<ClassT, AllocatorT>::ConstMethod<ReturnT, ArgsT...>;
    using ExtensionMethod = Class<ClassT, AllocatorT>::ExtensionMethod<ReturnT, ArgsT...>;
#elif defined(_MSC_VER)
    using Method = Class<ClassT, AllocatorT>::template Method<ReturnT, ArgsT...>;
    using ConstMethod = Class<ClassT, AllocatorT>::template ConstMethod<ReturnT, ArgsT...>;
    using ExtensionMethod = Class<ClassT, AllocatorT>::template ExtensionMethod<ReturnT, ArgsT...>;
#endif

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

    union
    {
        Method method_;
        ConstMethod constMethod_;
        ExtensionMethod extensionMethod_;
    };

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
