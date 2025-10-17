#ifndef LINKAGE_H
#define LINKAGE_H

#define SYMBOL_NAME(name) name
#define SYMBOL_NAME_STR(name) #name
#define SYMBOL_NAME_LABEL(name) name##:

#define ENTRY(name) \
        .global SYMBOL_NAME(name); \
        SYMBOL_NAME_LABEL(name)

#endif