#include "sqrew/Instance.h"

namespace sqrew {

struct Instance::Impl
{

};

Instance::Instance(const Context& context, const String& className)
    : impl_(new Impl())
{

}

Instance::~Instance()
{

}

} // namespace sqrew
