
#ifndef __OSTYPEDEF_H__
#define __OSTYPEDEF_H__

#include <limits.h>
#include <sys/types.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Typedefs */
typedef unsigned char       UInt8;
typedef signed char         SInt8;
typedef unsigned short      UInt16;
typedef signed short        SInt16;
typedef unsigned int	    UInt32;
typedef signed int		    SInt32;
typedef signed long long 	SInt64;
typedef unsigned long long 	UInt64;
typedef float               Float32;
typedef double              Float64;
typedef UInt16              Bool16;
typedef UInt8               Bool8;

#endif
