#pragma once

#define RETURN_IF_ERROR(expr, msg)                                             \
    do                                                                         \
    {                                                                          \
        int _ret = (expr);                                                     \
        if (_ret < 0)                                                          \
        {                                                                      \
            std::perror(msg);                                                  \
            return _ret;                                                       \
        }                                                                      \
    } while (0)
