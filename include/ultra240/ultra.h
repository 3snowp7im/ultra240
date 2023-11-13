#pragma once

#if __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN
#endif

/** Framework entry point. */
EXTERN int ultra_run(const char* name, int argc, const char* argv[]);
