/*
 * Copyright (c) 2010, 2011
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

#include "AppHardwareApi.h"
#include "contiki.h"
#include "contiki-net.h"
#include "dev/irq.h"
#include "dev/i2c.h"
#include "dev/leds.h"
#include "jts.h"
#include "gdb2.h"

#if defined(__BA1__)
# define BUS_ERROR           *((volatile uint32 *)(0x4000008))
# define UNALIGNED_ACCESS    *((volatile uint32 *)(0x4000018))
# define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x400001C))
# define UNALIGNED_ACCESS_HANDLER 0x00012D70

#elif defined(__BA2__)

#ifdef JENNIC_CHIP_FAMILY_JN516x
# define BUS_ERROR           *((volatile uint32 *)(0x4000008))
# define UNALIGNED_ACCESS    *((volatile uint32 *)(0x4000014))
# define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x400001A))
# define HARDWARE_INTERRUPT  *((volatile uint32 *)(0x4000020))

# define TICK_TIMER          *((volatile uint32 *)(0x400000E))
# define SYS_CALL            *((volatile uint32 *)(0x4000026))
# define SYS_TRAP            *((volatile uint32 *)(0x400002C))
# define SYS_RESET           *((volatile uint32 *)(0x4000038))
# define STACK_OVERFLOW      *((volatile uint32 *)(0x400003E))
#else
# define BUS_ERROR           *((volatile uint32 *)(0x4000000))
# define UNALIGNED_ACCESS    *((volatile uint32 *)(0x4000008))
# define ILLEGAL_INSTRUCTION *((volatile uint32 *)(0x400000C))

# define TICK_TIMER          *((volatile uint32 *)(0x4000004))
# define SYS_CALL            *((volatile uint32 *)(0x4000014))
# define SYS_TRAP            *((volatile uint32 *)(0x4000018))
# define STACK_OVERFLOW      *((volatile uint32 *)(0x4000020))
#endif

#else
# error "unkown arch"
#endif

#ifdef __BA2__

# define SR_REGISTER  17 /* supervision register           */
# define EPC_REGISTER 32 /* exception program counter      */
# define EEA_REGISTER 48 /* exception effective address    */
# define ESR_REGISTER 64 /* exception supervision register */
# define DSX_BIT (1<<13) /* instruction in delay slot      */

static uint32_t
mfspr(uint32_t reg)
{
  uint32_t val;
  asm ("b.mfspr %0, %1, 0;" :"=r"(val) : "r"(reg) );
  return val;
}

static void
mtspr(uint32_t reg, uint32_t val)
{
  asm volatile ("b.mtspr %0, %1, 0;" :: "r"(reg), "r"(val) );
}

void
panic(char *msg)
{
  char buf[1024];
  uint32_t EPC = mfspr(EPC_REGISTER), EEA = mfspr(EEA_REGISTER),
           ESR = mfspr(ESR_REGISTER), SR  = mfspr(SR_REGISTER), n, i, *sp;

  leds_init();

  //vAHI_UartEnable(E_AHI_UART_0);
  //vAHI_UartReset(E_AHI_UART_0, TRUE, TRUE);
  //vAHI_UartReset(E_AHI_UART_0, FALSE, FALSE);
  //vAHI_UartSetClockDivisor(E_AHI_UART_0, E_AHI_UART_RATE_115200);
  //vAHI_UartSetControl(E_AHI_UART_0, E_AHI_UART_EVEN_PARITY,
  //                    E_AHI_UART_PARITY_DISABLE, E_AHI_UART_WORD_LEN_8,
  //                    E_AHI_UART_1_STOP_BIT, E_AHI_UART_RTS_LOW);

  n = snprintf(buf, sizeof(buf), "%s EPC=0x%x, EEA=0x%x, ESR=0x%x SR=0x%x\n", msg, EPC, EEA, ESR, SR);

  /* stack trace */
  for (i=0, *sp=__builtin_frame_address(0); sp+i < 0x4020000; i++)
    n += snprintf(buf+n, sizeof(buf)-n, "%d=0x%x\n", i, *(sp+i));

  for(;;)
  {
    uint32_t i;
    uint32_t delay = 1;

    for (i=0; i<(n+1); i++)
    {
      while (!(u8AHI_UartReadLineStatus(E_AHI_UART_0) & E_AHI_UART_LS_THRE))
        ;
      vAHI_UartWriteData(E_AHI_UART_0, buf[i]);
    }

    leds_toggle(LEDS_ALL);

    while (delay++)
      ;
  }
}

#ifdef GDB
# define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
# define PRINTF(fmt, ...)
#endif

void
unaligned_access()
{
#ifdef GDB
# define ENTRI_REGS       7 /* the number of regs stored by the prologue, find'em with objdump! */
# define ENTRI_EXTRASTACK 2 /* number of extra space on the stack */
#else
# define ENTRI_REGS       6 /* the number of regs stored by the prologue, find'em with objdump! */
# define ENTRI_EXTRASTACK 0 /* number of extra space on the stack */
#endif

  uint32_t  EEA = mfspr(EEA_REGISTER),
            ESR = mfspr(ESR_REGISTER),
           *sp  = __builtin_frame_address(0),
           *NPC = sp+ENTRI_REGS+9;                                          /* NPC is the pointer to the next instr stored
                                                                     in link register (r9)          */
  uint8_t  *EPC = (uint8_t*) mfspr(EPC_REGISTER),                 /* get the failing instructions   */
           *eea = (uint8_t*) EEA,                                 /* byte-access to failing address */
           *reg = (uint8_t*) &sp[((*EPC&0x03) + (*(EPC+1)>>5)) + ENTRI_REGS],  /* store or load to/from this reg */
            num = 0;                                                  /* for multiple instr             */

  /* only able to handle those, because rom instructions are non-readable. And
   * if happening in a delay slot (i.e. after a branch or jump), we're unable to
   * calculate the address of the next instruction. */
  if (mfspr(EPC_REGISTER) < 0x4000000L || (ESR & DSX_BIT))
    panic("unaligned access outside ram or in delay slot");

  /* load current EPC into NPC and calculate next instruction address */
  *NPC = mfspr(EPC_REGISTER);

  /* read opcode, see ba-isa-ba2.c for instructions */
  switch(*EPC >> 2)
  {
  case (0x8<<2|0x2): /* half-word instruction */
    *NPC += 6;
  case (0xc<<2|0x2):
    *NPC += 4;
  case (0x2<<2|0x2):
    *NPC += 3;

    if (*(EPC+2) & (1<<7)) /* load instruction  */
    {
      PRINTF("half word load from 0x%x into r%d EPC=0x%x\n", EEA,
          ((*EPC&0x03) + (*(EPC+1)>>5)), mfspr(EPC_REGISTER));

      *reg     = 0x00;
      *(reg+1) = 0x00;
      *(reg+2) = *eea;
      *(reg+3) = *(eea+1);
    }
    else /* store instruction */
    {
      PRINTF("half word store from r%d=0x%x into 0x%x EPC=0x%x\n",
          ((*EPC&0x03) + (*(EPC+1)>>5)), *((uint32_t*) reg), EEA, 
          mfspr(EPC_REGISTER));

      *eea     = *(reg+2);
      *(eea+1) = *(reg+3);
    }
    break;

  case (0x8<<2|0x3): /* word instruction */
    *NPC += 6;
  case (0xc<<2|0x3):
    *NPC += 4;
  case (0x2<<2|0x3):
    *NPC += 3;

    if (*(EPC+2)&(1<<7) || *(EPC+2)&(1<<6)) /* load with sign/zeros */
    {
      PRINTF("word load from 0x%x=0x%x%x%x%x into r%d EPC=0x%x\n", EEA, (int) *(eea), (int) *(eea+1), (int) *(eea+2), (int) *(eea+3),
          ((*EPC&0x03) + (*(EPC+1)>>5)), mfspr(EPC_REGISTER));

      *reg     = *eea;
      *(reg+1) = *(eea+1);
      *(reg+2) = *(eea+2);
      *(reg+3) = *(eea+3);
    }
    else /* store instruction */
    {
      PRINTF("word store from r%d=0x%x into 0x%x EPC=0x%x\n",
          ((*EPC&0x03) + (*(EPC+1)>>5)), *((uint32_t*) reg),
          EEA, mfspr(EPC_REGISTER));

      *eea     = *reg;
      *(eea+1) = *(reg+1);
      *(eea+2) = *(reg+2);
      *(eea+3) = *(reg+3);
    }
    break;

  case (0x5<<2): /* multiple load/store word  */
  case (0x5<<2|0x01):
    *NPC += 3;
     num  = *(EPC+2)>>6;

    if (num==0) num=2;
    else if (num==1) num=3;
    else if (num==2) num=4;
    else if (num==3) num=8;

    PRINTF("multiple instr r%d=0x%x into 0x%x, %d vals\n",
        ((*EPC&0x03) + (*(EPC+1)>>5)), *((uint32_t*) reg),
        EEA, num, mfspr(EPC_REGISTER));

    if ((*EPC>>2)&0x03 == 0) /* load  */
      while (num--)
      {
        *(reg+num*4)   = *(eea+num*4);
        *(reg+num*4+1) = *(eea+num*4+1);
        *(reg+num*4+2) = *(eea+num*4+2);
        *(reg+num*4+3) = *(eea+num*4+3);
      }
    else                     /* store */
      while (num--)
      {
        *(eea+num*4)   = *(reg+num*4);
        *(eea+num*4+1) = *(reg+num*4+1);
        *(eea+num*4+2) = *(reg+num*4+2);
        *(eea+num*4+3) = *(reg+num*4+3);
      }
    break;

  default:
    panic("unaligned access with unknown instruction");
  }

  /* restore SR context */
  mtspr(SR_REGISTER, ESR);

  /* restore saved regs, somehow they end up behind the frame of this function,
   * we restore r2-r15 at the end of this function. r1 holds the stack pointer,
   * r0 is always zero. */
  asm volatile(
      "b.rtnei  %0,%1\n"            // remove current functions frame
      "b.mlwz   r2, 8(r1), 0x3\n"   // restore regs
      "b.mlwz   r8, 32(r1), 0x3\n"
      "b.reti   0, 20\n"            // jump to NPC via r9
      :: "i"(ENTRI_REGS), "i"(ENTRI_EXTRASTACK));
}

void
illegal_instr()
{
  panic("illegal instruction");
}

void
bus_error()
{
  panic("bus error");
}

void
misalign_test()
{
  static uint8_t  __attribute__((aligned(4))) arr[2+4+4*2+1];
  static uint8_t *p = arr;
  uint16_t v16 = 0xcafe;
  uint32_t v32 = 0xdeadbeef;

  {
    static char buf[1024];
    static size_t i,j;
    uint32_t *sp = __builtin_frame_address(0);

    for (i=0,j=0; i<32 && sp+i < 0x4020000; i++)
      j += snprintf(buf+j, sizeof(buf)-j, "%d=0x%x\n", i, *(sp+i));

    PRINTF(buf);
  }

  asm volatile(
      "l.sh   0(%2), %0\n"
      "l.sw   0(%3), %1\n"
      "l.lhz  %0, 0(%2)\n"
      "l.lwz  %1, 0(%3)\n"
      "l.msw  0(%4), %0, 0\n"
      "l.mlwz %0, 0(%4), 0\n"
      :: "r"(v16), "r"(v32), "r"(p+1), "r"(p+3), "r"(p+7));

  {
    static char buf[1024];
    static size_t i,j;
    uint32_t *sp = __builtin_frame_address(0);

    PRINTF("v16: %x, v32: %x\n", v16, v32);

    for (i=0,j=0; i<32 && sp+i < 0x4020000; i++)
      j += snprintf(buf+j, sizeof(buf)-j, "%d=0x%x\n", i, *(sp+i));

    PRINTF(buf);
  }


  {
    static char buf[512];
    size_t i,n;

    for (i=0,n=0; i<sizeof(arr); i++)
      n+=snprintf(buf+n, sizeof(buf)-n, "0x%x ", (uint32_t) arr[i]);

    n += snprintf(buf+n, sizeof(buf)-n, "\n");
    PRINTF(buf);
  }
}

# define UNALIGNED_ACCESS_HANDLER unaligned_access
#endif

static
void sys_irs_init()
{
#ifdef __BA2__
#ifndef JENNIC_CHIP_FAMILY_JN516x
  BUS_ERROR           = bus_error;
  ILLEGAL_INSTRUCTION = illegal_instr;
  UNALIGNED_ACCESS    = UNALIGNED_ACCESS_HANDLER;
#endif
#endif
}

void
init_hardware()
{
    u32AHI_Init();
    watchdog_stop();
    sys_irs_init();
    sys_baudrate_init(E_AHI_UART_RATE_38400);
    leds_init();
    irq_init();
    clock_init();
    ctimer_init();
}

void
sys_baudrate_init(uint16 baudrate)
{
#ifdef __BA2__
#ifdef JENNIC_CHIP_FAMILY_JN516x
    /* Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed */
    while (bAHI_GetClkSource() == true);
    /* Now we are running on the XTAL, optimise the flash memory wait states */
    vAHI_OptimiseWaitStates();
#endif
#endif

#ifdef GDB
    GDB2_STARTUP(E_AHI_UART_0, E_AHI_UART_RATE_38400);
# ifdef __BA1__
    uart0_set_br(baudrate);
    HAL_BREAKPOINT();
# endif
# else
    uart0_init(baudrate);
#endif
}
