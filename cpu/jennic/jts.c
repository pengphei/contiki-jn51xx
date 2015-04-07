/*
 * Copyright (c) 2009
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

#include "jts.h"
#include "hrclock.h"
#include "ieee802.h"
#include "AppApi.h"
#include "gdb2.h"
#include "string.h"
#include <AppHardwareApi.h>

#if __BA1__
# include "BeaconTxNotification.h"
#endif

#define FREE_RUNNING_SYMBOL_CLOCK *((volatile uint32 *)(0x10000400UL+0x0024UL))
#define SYMBOL_CLOCK_FREQ (1000*1000/16) // = 625 kHz
#define NANOSECONDS       (1000*1000*1000)

static volatile struct { int64_t offset; bool synchronized; } global_offset;

/**
 * Returns the baseband free running timer.  This timer increments every
 * 16uS.
 *
 * @note
 * THE MAC MUST BE INITIALISED BEFORE THIS IS CALLED OTHERWISE A BUS ERROR
 * EXCEPTION WILL BE GENERATED.  THIS DUE TO THE PROTOCOL POWER DOMAIN WILL
 * NOT BE POWERED UP.  */
static uint32_t
MAC_u32GetFreeRunningSymbolClock(void)
{
    return FREE_RUNNING_SYMBOL_CLOCK;
}

/**
 * Called when a beacon has just been transmitted.
 * To read what time the beacon was transmitted use
 * MAC_psGetPIB()->u32BeaconTxTime.
 *
 * This is the time after the start-of-frame delimiter, so the actual
 * radio started transmitting at u32BeaconTxTime-10;
 *
 * @note
 * THIS IS CALLED WITHIN INTERRUPT CONTEXT, DO NOT BLOCK OR ISSUE ANY
 * MAC COMMANDS. ONLY USE TO SIGNAL TO THE MAIN LOOP THAT THE EVENT 
 * HAS HAPPEND.  */
static void
beacontx()
{
  static MAC_Pib_s *pib = NULL;
  uint32_t u32NextBeacon;
  hrclock_t timestamp;

  if(pib==NULL) pib=MAC_psPibGetHandle(pvAppApiGetMacHandle());

 /**
  * use MAC_psGetPIB()->u32BeaconTxTime and
  * MAC_u32GetFreeRunningSymbolClock() to
  * for beacon timing

  u32SuperFrameEnd = pib->u32BeaconTxTime-10;
  u32SuperFrameEnd += MAC_BASE_SUPERFRAME_DURATION * (1UL <<
                      pib->u8SuperframeOrder);
  u32SuperFrameEnd -= MAC_u32GetFreeRunningSymbolClock();
  */

  /* u32NextBeacon is given in 16uS intervals */
  u32NextBeacon = pib->u32BeaconTxTime-10;
  u32NextBeacon += MAC_BASE_SUPERFRAME_DURATION * (1UL <<
                   pib->u8BeaconOrder);
  u32NextBeacon -= MAC_u32GetFreeRunningSymbolClock();

  /* calculate next beacon timestamp on local clock */
  timestamp = clock_hrtime();
  timestamp += (hrclock_t) u32NextBeacon * (NANOSECONDS/SYMBOL_CLOCK_FREQ);

  /* TODO: probably correct the time by the last instructions */
  /* copy timestamp into beacon payload so it gets send in the next beacon */
  pib->u8BeaconPayloadLength = sizeof(timestamp);
  memcpy(pib->au8BeaconPayload, &timestamp, sizeof(timestamp));

  /* this node is tx'ing beacons, so all others synchronize to this clock */
  global_offset.synchronized = true;
  global_offset.offset = 0;
}

static void
beaconrx(MAC_MlmeIndBeacon_s *b)
{
  static hrclock_t tx_timestamp, now;
  static uint32_t  rx_delay;

  /* TODO: need a better validation */
  if(b->u8SDUlength != sizeof(tx_timestamp))
    return;

  /* get rx_delay (in 16uS intervals)*/
  rx_delay = (MAC_u32GetFreeRunningSymbolClock() -
    (b->sPANdescriptor.u32TimeStamp));
  now = clock_hrtime();

  /* get tx_timestamp from beacon and correct by rx_delay */
  memcpy(&tx_timestamp, b->u8SDU, sizeof(tx_timestamp));

  /* correct tx timestamp by rx_delay */
  tx_timestamp += (hrclock_t) rx_delay * (NANOSECONDS/SYMBOL_CLOCK_FREQ);

  /* calculate offset */
  global_offset.offset = now - tx_timestamp;
  global_offset.synchronized = true;
}

void
jts_init()
{
  global_offset.synchronized = false;
  global_offset.offset = 0;

#ifdef __BA2__
# warning "jts not been ported to JN5148"
#else
  ieee_register_beacon_callback(beaconrx);
  MAC_vRegisterBeaconTxCallBack(beacontx);
#endif
}

hrclock_t    /* returns synchronized time in nano-seconds */
jts_hrtime()
{
  if(!global_offset.synchronized)
    return clock_hrtime();

  return clock_hrtime() - global_offset.offset;
}

bool         /* returns true if clocks are synchronized */
jts_synced()
{
  return global_offset.synchronized;
}

clock_time_t /* returns synchronized time in milli-seconds */
jts_time()
{
  return jts_hrtime() / (1000*1000);
}
