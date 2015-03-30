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

INCLUDE AppBuildNone_JN5168.ld

EXTERN(_TSV_eOpen)
EXTERN(_TSV_eStart)
EXTERN(_TSV_eStop)
EXTERN(_pvAppApiDefGetMacHandle)
EXTERN(_u32AppApiDefInit)
EXTERN(_vAppApiDefMlmeRequest)
EXTERN(_vAppApiDefMcpsRequest)
EXTERN(_eAppApiDefPlmeSet)
EXTERN(_eAppApiDefPlmeGet)
EXTERN(_pvAppApiDefGetMacAddrLocation)
EXTERN(_vAppApiDefSaveMacSettings)
EXTERN(_vAppApiDefRestoreMacSettings)

/* Following map patchable functions to their API calls, for TSV and MAC */
_TSV_eOpenVector            = _TSV_eOpen;
_TSV_eStartVector           = _TSV_eStart;
_TSV_eStopVector            = _TSV_eStop;
_pvAppApiGetMacHandle       = _pvAppApiDefGetMacHandle;
_u32AppApiInit              = _u32AppApiDefInit;
_vAppApiMlmeRequest         = _vAppApiDefMlmeRequest;
_vAppApiMcpsRequest         = _vAppApiDefMcpsRequest;
_eAppApiPlmeSet             = _eAppApiDefPlmeSet;
_eAppApiPlmeGet             = _eAppApiDefPlmeGet;
_pvAppApiGetMacAddrLocation = _pvAppApiDefGetMacAddrLocation;
_vAppApiSaveMacSettings     = _vAppApiDefSaveMacSettings;
_vAppApiRestoreMacSettings  = _vAppApiDefRestoreMacSettings;
