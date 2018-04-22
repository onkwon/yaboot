#ifndef __BSP_H__
#define __BSP_H__

#include "regs.h"

#define sei()								\
	__asm__ __volatile__(						\
			"cpsie i	\n\t"				\
			"isb		\n\t"				\
			::: "cc", "memory")
#define cli()								\
	__asm__ __volatile__("cpsid i" ::: "cc", "memory")

#define dmb()			__asm__ __volatile__("dmb" ::: "memory")
#define dsb()			__asm__ __volatile__("dsb" ::: "memory")
#define isb()			__asm__ __volatile__("isb" ::: "memory")

#define setsp(sp)							\
	__asm__ __volatile__("mov sp, %0" :: "r"(sp))

#define debug(msg...)

#define min(a, b)		({ \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b; \
})

#define BASE_ALIGN(x, a)		((x) & ~((typeof(x))(a) - 1UL))

#endif /* __BSP_H__ */
