#ifndef _named_class_h_
#define _named_class_h_

#include <string>

class NamedClass {
public:
    NamedClass ( std::string name = "noname");
    virtual  ~NamedClass();
    const std::string& GetClassName();
    const char* GetClassNameCstr();

private:
    std::string class_name;
};

#endif
