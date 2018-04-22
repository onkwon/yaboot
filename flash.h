#ifndef __FLASH_H__
#define __FLASH_H__

#include "bsp/flash/flash.h"
#include <stddef.h>

size_t flash_program(void * const addr, const void * const buf, size_t len);

#endif /* __FLASH_H__ */
