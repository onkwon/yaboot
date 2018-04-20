#ifndef __REGS_H__
#define __REGS_H__

#define SCB_BASE		(0xE000E000)
#define SCB_VTOR		(*(volatile unsigned int *)(SCB_BASE + 0xD08))
#define SCB_AIRCR		(*(volatile unsigned int *)(SCB_BASE + 0xD0C))
#define SCB_CCR 		(*(volatile unsigned int *)(SCB_BASE + 0xD14))

#endif /* __REGS_H__ */
