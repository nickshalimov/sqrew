#include "sqrew/Interface.h"

#include <sstream>

namespace sqrew {

void Interface::printError(const String& message)
{
    print(message);
}

void Interface::handleCompilerError(const String& error, const String& source, int line, int column)
{
    std::ostringstream oss;
    oss << "compile error: " << source.c_str() << ":" << line << ":" << column << ". " << error.c_str();
    printError(oss.str());
}

} // namespace sqrew
