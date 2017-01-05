 /***********************************************************
  *   Program:    AVRGCC - LCD test
  *   Created:    10.10.99 17:23
  *   Author:     Bray++
  *   Comments:   printf is in very Alfa stage...:)
  ************************************************************/

#include <avr/io.h>
#include <avr/signal.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>

#include "usart.h"

#define Devi2c  0xA2
#define MAX_ITER	200
#define PAGE_SIZE	8


uint8_t twst;


