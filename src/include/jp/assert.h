#ifndef __ASSERT_H__
#define __ASSERT_H__

void assertion_failure(char *exp, char *file, const char *func, int line);

#define assert(exp) do { \
            if (!(exp)) assertion_failure(#exp, __FILE__, __func__, __LINE__); \
        }while(0)

#define static_assert(exp) (void)(sizeof(char[(exp)?1:-1]))
void panic(const char *fmt, ...);

#endif
