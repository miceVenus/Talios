#ifndef LINKAGE_H
#define LINKAGE_H

#define SYMBOL_NAME(name) name
#define SYMBOL_NAME_STR(name) #name
#define SYMBOL_NAME_LABEL(name) name##:



#define ENTRY(name) \
        .global SYMBOL_NAME(name); \
        SYMBOL_NAME_LABEL(name)

#define GET_CURRENT(reg)                \
        movq   %rsp,       reg;         \
        andq   $-32768,    reg;

#endif