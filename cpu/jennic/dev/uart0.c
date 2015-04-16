/*
 * Copyright (c) 2011
 * Telecooperation Office (TecO), Universitaet Karlsruhe (TH), Germany.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. Neither the name of the Universitaet Karlsruhe (TH) nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author(s): Philipp Scholl <scholl@teco.edu>
 */

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <stdbool.h>
#include "dev/uart0.h"
#include "lib/ringbuf.h"

#define UART_MCR_OFFSET   0x10
#define UART_LCR_OFFSET   0x0C
#define UART_DLM_OFFSET   0x04
#define UART_EFR_OFFSET   0x20

#ifdef __BA1__ /* jn5139 core */
# define UART0_START_ADDR 0x30000000UL
# define UART1_START_ADDR 0x40000000UL
#else          /* jn5148 core */
# define UART0_START_ADDR 0x02003000UL
# define UART1_START_ADDR 0x02004000UL
# define UART_AFC_OFFSET  0x2C
#endif

#define TXBUFSIZE 64

static struct ringbuf txbuf;
static uint8_t txbuf_data[TXBUFSIZE];
static volatile uint8_t transmitting;

void
uart0_set_br(uint16 br)
{
    if((E_AHI_UART_RATE_19200 == br)||(E_AHI_UART_RATE_4800 == br)
        ||(E_AHI_UART_RATE_9600 == br)||(E_AHI_UART_RATE_38400 == br)
        ||(E_AHI_UART_RATE_76800 == br)||(E_AHI_UART_RATE_115200 == br))
    {
        vAHI_UartSetBaudRate(E_AHI_UART_0, br);
    }
    else
    {
        uint8 *pu8Reg;
        uint8  u8TempLcr;
        uint16 u16Divisor;
        uint32 u32Remainder;
        uint32 UART_START_ADR;

        UART_START_ADR=UART0_START_ADDR;

        /* Put UART into clock divisor setting mode */
        pu8Reg    = (uint8 *)(UART_START_ADR + UART_LCR_OFFSET);
        u8TempLcr = *pu8Reg;
        *pu8Reg   = u8TempLcr | 0x80;

        /* Write to divisor registers:
           Divisor register = 16MHz / (16 x baud rate) */
        u16Divisor = (uint16)(16000000UL / (16UL * br));

        /* Correct for rounding errors */
        u32Remainder = (uint32)(16000000UL % (16UL * br));

        if (u32Remainder >= ((16UL * br) / 2))
        {
            u16Divisor += 1;
        }

        pu8Reg  = (uint8 *)UART_START_ADR;
        *pu8Reg = (uint8)(u16Divisor & 0xFF);
        pu8Reg  = (uint8 *)(UART_START_ADR + UART_DLM_OFFSET);
        *pu8Reg = (uint8)(u16Divisor >> 8);

        /* Put back into normal mode */
        pu8Reg    = (uint8 *)(UART_START_ADR + UART_LCR_OFFSET);
        u8TempLcr = *pu8Reg;
        *pu8Reg   = u8TempLcr & 0x7F;
    }
}

static volatile int (*uart0_input)(unsigned char c);

static void irq(uint32 irqsrc, uint32 map)
{
  if (map==E_AHI_UART_INT_RXDATA)
  {
    if (uart0_input)
      uart0_input(u8AHI_UartReadData(E_AHI_UART_0));
    else
      u8AHI_UartReadData(E_AHI_UART_0);
  }

  if (map==E_AHI_UART_INT_TX)
  {
    if (ringbuf_elements(&txbuf)==0)
      transmitting=0;
    else
      vAHI_UartWriteData(E_AHI_UART_0, ringbuf_get(&txbuf));
  }
}

void uart0_set_input(int (*input)(unsigned char c))
{
  uart0_input = input;
}

void uart0_writeb(unsigned char c)
{
  /* push into rinbuf until there is space */
  while (ringbuf_put(&txbuf,c)==0)
    ;

  if (transmitting==0)
  {
    transmitting=1;
    vAHI_UartWriteData(E_AHI_UART_0, ringbuf_get(&txbuf));
  }
}

void uart0_init(uint16 br)
{
  transmitting = 0;
  ringbuf_init(&txbuf, txbuf_data, sizeof(txbuf_data));

  vAHI_UartSetRTSCTS(E_AHI_UART_0, false);
  vAHI_UartEnable(E_AHI_UART_0);

  vAHI_UartReset(E_AHI_UART_0, true, true);

  uart0_set_br(br);
  //vAHI_UartSetRTSCTS(E_AHI_UART_0, false);

  vAHI_Uart0RegisterCallback(irq);
  vAHI_UartSetInterrupt(E_AHI_UART_0, false,  /* modem status         */
                              false,  /* rx line error status */
                              true,   /* tx fifo empty        */
                              true,   /* rx data there        */
                              E_AHI_UART_FIFO_LEVEL_1);

  //vAHI_UartSetRTSCTS(E_AHI_UART_0, false);
  vAHI_UartReset(E_AHI_UART_0, false, false);
}
