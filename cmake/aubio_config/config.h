/* Minimal aubio config for bundled static builds (no optional I/O or FFTW). */

#ifndef AUBIO_CONFIG_H
#define AUBIO_CONFIG_H

#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_MATH_H 1
#define HAVE_STRING_H 1
#define HAVE_ERRNO_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDARG_H 1
#define HAVE_MEMCPY_HACKS 1

#endif // AUBIO_CONFIG_H
