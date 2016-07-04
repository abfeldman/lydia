#ifndef DEFS_H
#define DEFS_H

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#if !defined(WIN32) || defined(__STDC__)
# define min(x, y) ((x) < (y) ? (x) : (y))
# define max(x, y) ((x) > (y) ? (x) : (y))
#endif
#define memdup(buf, n) (memcpy(malloc((n)), (buf), (n)))
#define swapint(x) (((x) << 24) | (((unsigned int)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

/* Some missed Tm stuff: */
#define isequal_double(a, b) (a == b)

#define i2p(x) ((void *)(long)(x))
#define p2i(x) ((int)(long)(x))
#define ui2p(x) ((void *)(long)(x))
#define p2ui(x) ((unsigned int)(long)(x))

#define EPSILON 10e-10

#endif
