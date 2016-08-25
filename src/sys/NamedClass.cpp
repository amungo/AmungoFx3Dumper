#include "NamedClass.h"

NamedClass::NamedClass(std::string name) : class_name( name ) {
}

NamedClass::~NamedClass() {
}

const std::string &NamedClass::GetClassName() {
    return class_name;
}

const char *NamedClass::GetClassNameCstr() {
    return class_name.c_str();
}
