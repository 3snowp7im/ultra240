#pragma once

#if __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN
#endif

EXTERN int ultra_run(const char* name, int argc, const char* argv[]);
