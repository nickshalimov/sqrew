#include <sqrew/Context.h>
#include <sqrew/Interface.h>
#include <sqrew/Class.h>

#include <iostream>
#include <array>

class TestInterface: public sqrew::Interface
{
    void print(const sqrew::String& message) override
    {
        std::cout << message.c_str() << std::endl;
    }
};

class ExposeTest
{
public:
    int f;
    ExposeTest(): f(0) {}
    ExposeTest(int s): f(s) {}

    void someMethod(std::string) {}

    static std::shared_ptr<ExposeTest> create()
    {
        return std::make_shared<ExposeTest>();
    }

    void setF(int value)
    {
        f = value;
    }

    const char* marker = "marker";

    int getF() const { return f; }

    void setF1(int value)
    {
        f = value;
    }

    void setF2(int value) const
    {

    }

    ~ExposeTest()
    {
        std::cout << "destructor called: " << f << std::endl;
    }
};


template<class ClassT>
struct SharedCreationAllocator
{
    using Shared = std::shared_ptr<ClassT>;
    using Pointer = Shared*;

    template<class ...ArgsT>
    inline static Pointer createInstance(ArgsT&&... args) { return new Shared(ClassT::create(std::forward<ArgsT>...)); }

    inline static void destroyInstance(Pointer instance) { delete instance; }

    inline static ClassT* castInstance(Pointer ptr) { return ptr->get(); }
};

template<class ClassT, class ReturnT, class ...ArgsT>
class MethodDelegate
{
public:
    using Method = ReturnT (ClassT::*)(ArgsT...);
    using ConstMethod = ReturnT (ClassT::*)(ArgsT...) const;

    explicit MethodDelegate(Method method)
        : method_(method)
        , invoke_(callMethod)
    {}

    explicit MethodDelegate(ConstMethod method)
        : constMethod_(method)
        , invoke_(callConstMethod)
    {}

    ReturnT operator()(ClassT* instance, ArgsT&&... args)
    {
        return invoke_(this, instance, std::forward<ArgsT>(args)...);
    }

private:
    using MathodInvoke = ReturnT (*)(MethodDelegate* self, ClassT* instance,  ArgsT&&... args);

    Method method_;
    ConstMethod constMethod_;
    MathodInvoke invoke_;

    static ReturnT callMethod(MethodDelegate* self, ClassT* instance,  ArgsT&&... args)
    {
        return (instance->*self->method_)(std::forward<ArgsT>(args)...);
    }

    static ReturnT callConstMethod(MethodDelegate* self, ClassT* instance,  ArgsT&&... args)
    {
        return (instance->*self->method_)(std::forward<ArgsT>(args)...);
    }
};

namespace sqrew {

    class Expose
    {
    public:

        class Table;

        template<class ...ArgsT>
        Expose(Context& context, ArgsT... path)
            : vm_(context.getHandle())
        {
            std::array<String, sizeof...(ArgsT)> package = {{ path... }};
        }

        /*template<class ClassT>
        Class<ClassT> addClass(const String& name)
        {
            return Class<ClassT>();
        }*/

    private:
        HSQUIRRELVM vm_;
    };
}


void func()
{

}

int main(int /*argc*/, char * /*argv*/[])
{
    sqrew::Context context;
    context.initialize();
    context.setInterface<TestInterface>();

    //sqrew::Class<ExposeTest>::expose(context, "package.name.ExposeTest")
    sqrew::Class<ExposeTest, SharedCreationAllocator>::expose(context, "ExposeTest")
        .setConstructor<>()
        //.setConstructor<int>()
        //.setMethod("someMethod", &ExposeTest::someMethod)
        .setMethod("getF", &ExposeTest::getF)
        .setMethod("setF", &ExposeTest::setF)
        //.setField("f", &ExposeTest::f)
        //.setField("f", &ExposeTest::getF, &ExposeTest::setF)
        ;

    //Class<ExposeTest>(context)
    //    .construct()


    bool result = context.executeBuffer("local foo = ExposeTest(); \n foo.setF(17); \n local f = foo.getF(); \n ::print(f);");
    int kp = 90;
    int nno = kp + 87;
    return 0;
}
