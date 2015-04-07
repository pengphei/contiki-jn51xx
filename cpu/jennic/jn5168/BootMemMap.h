/*****************************************************************************
 *
 * MODULE:              Boot loader
 *
*/
/****************************************************************************
*
* This software is owned by NXP B.V. and/or its supplier and is protected
* under applicable copyright laws. All rights are reserved. We grant You,
* and any third parties, a license to use this software solely and
* exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
* You, and any third parties must reproduce the copyright and warranty notice
* and any other legend of ownership on each copy or partial copy of the 
* software.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.

* Copyright NXP B.V. 2012. All rights reserved
*
***************************************************************************/

#ifndef  BOOT_MEM_MAP_INCLUDED
#define  BOOT_MEM_MAP_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Direct Wishbone access */
#define WRITE_REG32(A, B) (*(volatile uint32 *)(A)) = (B)
#define WRITE_REG16(A, B) (*(volatile uint16 *)(A)) = (B)
#define WRITE_REG8(A, B)  (*(volatile uint8 *)(A)) = (B)
#define READ_REG32(A)     (*(volatile uint32 *)(A))
#define READ_REG16(A)     (*(volatile uint16 *)(A))
#define READ_REG8(A)      (*(volatile uint8 *)(A))

/* Register addresses */
#define SC_START_ADDR      ((volatile uint32 *)0x2001000)
#define UART0_START_ADDR   ((volatile uint32 *)0x2003000)
#define UART1_START_ADDR   ((volatile uint32 *)0x2004000)
#define SPI_START_ADDR     ((volatile uint32 *)0x2008000)
#define SPI_MASTER_DATA    SPI_START_ADDR
#define SPI_MASTER_CONTROL (SPI_START_ADDR + 1)
#define SPI_MASTER_DIVIDER (SPI_START_ADDR + 2)
#define SPI_MASTER_SELECT  (SPI_START_ADDR + 3)
#if (!defined CHIP_RELEASE_1) && (!defined CHIP_RELEASE_2)
#define SPI_MASTER_SCRAM   (SPI_START_ADDR + 6)
#endif

#define SC_PWR_CTRL        SC_START_ADDR
#define SC_STAT            (SC_START_ADDR + 1)

/* Temporary stack space used by boot loader. TEMP_STACK_TOP is the memory
   location just after the end of a block of RAM allocated for use as a stack
   whilst the boot loader runs. Note that this stack space is only needed
   because the compiler insists on storing registers onto the stack at the
   start of the boot code, and restoring them afterwards. In practice the
   boot code never completes because it jumps to the 'proper' code so this
   temporary stack can readily be overwritten by the code that is being read
   from EEPROM without any ill effects. As the boot loader doesn't pull
   anything from this stack it has been set to point into ROM, thus not
   overwriting any RAM during a warm start yet also not causing any memory
   access errors */
#define TEMP_STACK_TOP (0x00004000)

#define BIT8

/* Base addresses */
#define BASE_ADDR_SYS       0x2001000
#define BASE_ADDR_UART0     0x2003000
#define BASE_ADDR_UART1     0x2004000
#define SYS_EN_UART0_M      0x00001000
#define SYS_EN_UART1_M      0x00002000

/* UARTs
 * Either define UART0 for UART 0 to be the download UART
 * Or define UART1 for UART 1 to be the download UART
 * Define UARTD if you want the other UART to be used for diagnostics
 */
#if defined(UART1)
#define BASE_ADDR_UART      BASE_ADDR_UART1
#define SYS_EN_UART_M       SYS_EN_UART1_M
#ifdef UARTD
#define BASE_ADDR_UARTD     BASE_ADDR_UART0
#define SYS_EN_UARTD_M      SYS_EN_UART0_M
#endif /* UARTD */
#elif defined(UART0)
#define BASE_ADDR_UART      BASE_ADDR_UART0
#define SYS_EN_UART_M       SYS_EN_UART0_M
#ifdef UARTD
#define BASE_ADDR_UARTD     BASE_ADDR_UART1
#define SYS_EN_UARTD_M      SYS_EN_UART1_M
#endif /* UARTD */
#else
#error "UART not defined"
#endif

/* System Controller Addresses */
#define ADDR_SYS_PWR_CTRL   BASE_ADDR_SYS
#define ADDR_SYS_PBSW       (BASE_ADDR_SYS + 0x1c)
#define ADDR_SYS_CHIP_ID    (BASE_ADDR_SYS + 0xFC)

//Note: from JN5142 onwards HDK builds are no different to chip builds
//No HDK flags will be defined in Jn5142 builds
#ifdef HDK_EFUSE_TEST
#define ADDR_SYS_EFUSE_VAL  (0x00000080)
#else
#define ADDR_SYS_EFUSE_VAL  (BASE_ADDR_SYS + 0x60)
#endif

/* UART Address Offsets */
#define OFFSET_UART_BUFDIV0 0x00
#define OFFSET_UART_INTEDIV1 0x04
#define OFFSET_UART_INT     0x08 // valid during reads
#define OFFSET_UART_FIFOC   0x08 // valid during writes
#define OFFSET_UART_LINEC   0x0c
#define OFFSET_UART_MODEM   0x10
#define OFFSET_UART_LINES   0x14
#define OFFSET_UART_MODEMS  0x18

#ifdef BIT8
#define BYTE_OFFSET 3
#define WRITE_UART WRITE_REG8
#define READ_UART READ_REG8
#else
#define BYTE_OFFSET 0
#define WRITE_UART WRITE_REG32
#define READ_UART READ_REG32
#endif

#define UART_FCR   (BASE_ADDR_UART + OFFSET_UART_FIFOC + BYTE_OFFSET)
#define UART_LCR   (BASE_ADDR_UART + OFFSET_UART_LINEC + BYTE_OFFSET)
#define UART_DATA  (BASE_ADDR_UART + OFFSET_UART_BUFDIV0 + BYTE_OFFSET)
#define UART_LSR   (BASE_ADDR_UART + OFFSET_UART_LINES + BYTE_OFFSET)
#ifdef UARTD
#define UARTD_FCR   (BASE_ADDR_UARTD + OFFSET_UART_FIFOC + BYTE_OFFSET)
#define UARTD_LCR   (BASE_ADDR_UARTD + OFFSET_UART_LINEC + BYTE_OFFSET)
#define UARTD_DATA  (BASE_ADDR_UARTD + OFFSET_UART_BUFDIV0 + BYTE_OFFSET)
#define UARTD_LSR   (BASE_ADDR_UARTD + OFFSET_UART_LINES + BYTE_OFFSET)
#endif /* UARTD */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* BOOT_MEM_MAP_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

