#pragma once
#ifndef SQREW_FORWARD_H
#define SQREW_FORWARD_H

#include <string>
#include <memory>

typedef struct SQVM* HSQUIRRELVM;

namespace sqrew {

class Context;
class Interface;

using String = std::string;
using Integer = int;
using Float = float;

} // namespace sqrew

#endif // SQREW_FORWARD_H
