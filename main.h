#ifndef __MAIN_H__
#define __MAIN_H__

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

#endif /* __MAIN_H__ */
