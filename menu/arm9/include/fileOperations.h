/*-----------------------------------------------------------------
 Not Copyright (ɔ) 2019 - 2020
    FlameKat53
    Pk11
    RocketRobz
    StackZ
------------------------------------------------------------------*/
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H
#include <optional>

extern bool sdMount(void);
extern bool flashcardMount(void);
extern bool bStyleMount(const char *__path, std::optional<bool> __boolStatus);

#endif
