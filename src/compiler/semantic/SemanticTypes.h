#ifndef SEMANTICTYPES_H
#define SEMANTICTYPES_H


enum class DataType
{


    Integer,
    Float,
    Text,
    Logical,
    Character,
    Void,
    Unknown
};

enum class SymbolKind
{
    Variable,
    Vector,
    Parameter,
    Function
};

#endif // SEMANTICTYPES_H
