#pragma once
#ifndef SQREW_FORWARD_H
#define SQREW_FORWARD_H

#include <string>
#include <memory>

typedef struct SQVM* HSQUIRRELVM;
//#define SQREW_STR(a) a

namespace sqrew {

class Context;
class Interface;
class Table;

using String = std::string;
using Integer = int;
using Float = float;

} // namespace sqrew

#endif // SQREW_FORWARD_H
