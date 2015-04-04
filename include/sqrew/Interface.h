#ifndef SQREW_INTERFACE_H
#define SQREW_INTERFACE_H

#include "sqrew/Forward.h"

namespace sqrew {

class Interface
{
public:
    virtual ~Interface() {}

    virtual void print(const String& message) {}

    virtual void printError(const String& message);

    virtual void handleCompilerError(const String& error,
                                     const String& source,
                                     int line,
                                     int column);

    //virtual bool readFile(const String& fileName,
    //                      String& buffer) = 0;
};

} // namespace sqrew

#endif // SQREW_INTERFACE_H
