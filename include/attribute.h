
#ifndef ATTRIBUTE_H_INCLUDED
#define ATTRIBUTE_H_INCLUDED

struct AttributeInfo {
    bool is_public;
    bool is_external;
    bool is_static;
    bool is_packed;
    bool is_stdcall;

    AttributeInfo();
    AttributeInfo(bool);
    void reset();
};

#endif // ATTRIBUTE_H_INCLUDED
