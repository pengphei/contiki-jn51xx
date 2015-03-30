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
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/* Now building all code at once, so include vectors. MAC address is embedded
   in build for now */


EXTERN(_vException_BusError)
EXTERN(_vException_UnalignedAccess)
EXTERN(_vException_IllegalInstruction)
EXTERN(_vException_SysCall)
EXTERN(_vException_Trap)
EXTERN(_vException_StackOverflow)
EXTERN(__exception_external_interrupt)
PROVIDE(_vException_BusError           = restore_state);
PROVIDE(_vException_UnalignedAccess    = restore_state);
PROVIDE(_vException_IllegalInstruction = restore_state);
PROVIDE(_vException_SysCall            = restore_state);
PROVIDE(_vException_Trap               = restore_state);
PROVIDE(_vException_StackOverflow      = restore_state);


SECTIONS
{

        . = ABSOLUTE(__exception_vsr_table);
        
        .vsr_table :
        {
        LONG(ABSOLUTE(_vException_BusError))
        LONG(ABSOLUTE(_vException_UnalignedAccess))
        LONG(ABSOLUTE(_vException_IllegalInstruction))
        LONG(ABSOLUTE(_vException_SysCall))
        LONG(ABSOLUTE(_vException_Trap))
        LONG(ABSOLUTE(_vException_StackOverflow))
        } > flash

        .vsr_handlers :
        {
            KEEP(*(.vsr_handlers))
        } > flash

}
