
#ifndef ATTRIBUTE_H_INCLUDED
#define ATTRIBUTE_H_INCLUDED

struct AttributeInfo {
    bool is_public;
    bool is_static;
    bool is_packed;

    AttributeInfo();
    AttributeInfo(bool);
    AttributeInfo(bool, bool);
};

#endif // ATTRIBUTE_H_INCLUDED
