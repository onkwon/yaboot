#ifndef __REGS_H__
#define __REGS_H__

#define SCB_BASE		(0xE000E000)
#define SCB_VTOR		(*(volatile unsigned int *)(SCB_BASE + 0xD08))
#define SCB_AIRCR		(*(volatile unsigned int *)(SCB_BASE + 0xD0C))
#define SCB_CCR 		(*(volatile unsigned int *)(SCB_BASE + 0xD14))

/* Embedded Flash memory */
#if defined(stm32f1) || defined(stm32f3)
#define FLASH_BASE		(0x40022000)
#define FLASH_ACR		(*(volatile unsigned int *)FLASH_BASE)
#define FLASH_KEYR		(*(volatile unsigned int *)(FLASH_BASE + 0x4))
#define FLASH_OPTKEYR		(*(volatile unsigned int *)(FLASH_BASE + 0x8))
#define FLASH_SR		(*(volatile unsigned int *)(FLASH_BASE + 0xc))
#define FLASH_CR		(*(volatile unsigned int *)(FLASH_BASE + 0x10))
#define FLASH_AR		(*(volatile unsigned int *)(FLASH_BASE + 0x14))
#define FLASH_OBR		(*(volatile unsigned int *)(FLASH_BASE + 0x1c))
#define FLASH_WRPR		(*(volatile unsigned int *)(FLASH_BASE + 0x20))

#define FLASH_OPT_BASE		(0x1ffff800)
#define FLASH_OPT_RDP		(*(volatile unsigned short int *)FLASH_OPT_BASE)
#elif defined(stm32f4)
#define FLASH_BASE		(0x40023c00)
#define FLASH_ACR		(*(volatile unsigned int *)FLASH_BASE)
#define FLASH_KEYR		(*(volatile unsigned int *)(FLASH_BASE + 0x4))
#define FLASH_OPTKEYR		(*(volatile unsigned int *)(FLASH_BASE + 0x8))
#define FLASH_SR		(*(volatile unsigned int *)(FLASH_BASE + 0xc))
#define FLASH_CR		(*(volatile unsigned int *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR		(*(volatile unsigned int *)(FLASH_BASE + 0x14))

#define FLASH_OPT_BASE		(0x1fffc000)
#define FLASH_OPT_RDP		(*(volatile unsigned short int *)FLASH_OPT_BASE)
#else
#error undefined machine
#endif

#endif /* __REGS_H__ */
