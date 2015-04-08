#include <sqrew/Context.h>
#include <sqrew/Interface.h>
#include <sqrew/Class.h>
#include <sqrew/Table.h>

#include <sqrew/Instance.h>

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

void extendTest(ExposeTest* expose, float xx)
{
    std::cout << "hello from extension method. you passed " << xx << ". and f value is " << expose->f << std::endl;
}

template<class ClassT>
struct MyDefaultAllocator
{
    using Pointer = ClassT*;

    template<class ...ArgsT>
    inline static Pointer createInstance(ArgsT&&... args) { return new ClassT(std::forward<ArgsT>(args)...); }

    inline static void destroyInstance(Pointer instance) { delete instance; }

    inline static ClassT* castInstance(Pointer ptr) { return ptr; }
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

int main(int /*argc*/, char * /*argv*/[])
{
    sqrew::Context context;
    context.initialize();
    context.setInterface<TestInterface>();

    sqrew::Class<ExposeTest>::expose(context, "ExposeTest")
        .setConstructor<int>()
        .setMethod("getF", &ExposeTest::getF)
        .setMethod("setF", &ExposeTest::setF)
        .setMethod("extendTest", extendTest)
        .setField("f", &ExposeTest::setF, &ExposeTest::getF);

    auto table = sqrew::Table::create(context, "com.package.name");
    auto table1 = sqrew::Table::create(context, "com.package.name");

    sqrew::Instance instance(context, "ExposeTest");

    auto call = instance.getMethod("setF");
    if (call.isValid())
        call(12);

    //auto callResult = instance.call<int>("getF");

    bool result = context.executeBuffer("local foo = ExposeTest(6464); \n ::print(foo.f); \n foo.f = 32; \n foo.extendTest(27.4); \n foo.setF(17); \n local f = foo.getF(); \n ::print(f);");
    int kp = 90;
    int nno = kp + 87;
    return 0;
}
