/*
 * Copyright (c) 2008-2010
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
 *
 * Implementation of the ieee802.15.4 glue layer.
 *
 * CSMA/CA on jennic modules is blocking reception during the random backoff
 * sequence, trying to attenuate that situation by setting the number of retries
 * to 1 and using the bTACframeInProgress function to only post a send request
 * while no other frame sending is in progress.
 */
#include "ieee802.h"
#include <mac_sap.h>
#include <mac_pib.h>
#include <stdbool.h>
#include <stdint.h>
#include <AppHardwareApi.h>
#include "lib/assert.h"
#include "net/netstack.h"

#define DEBUG 0
#if DEBUG
# define PRINTF(...) printf(__VA_ARGS__)
#else
# define PRINTF(...)
#endif

/* also used by auxiliary functions defined in ieee802_aux.c */
static void (*lqicb)(const rimeaddr_t*, uint8_t) = NULL;

#include "net/rime.h"
#include "string.h"
#include "gdb2.h"

/* Ieee 802.15.4 mac layer functions */
#define istimeout(ev)         (ev==NULL)
#define asscan(ev)            ((ev)->uParam.sDcfmScan)
#define asassociate(ev)       ((ev)->uParam.sDcfmAssociate)
#define asinddisassociate(ev) ((ev)->uParam.sIndDisassociate)
#define asindassociate(ev)    ((ev)->uParam.sIndAssociate)

#define asdataframe(ev)       (((MAC_McpsDcfmInd_s*) ev)->uParam.sIndData.sFrame)
#define asdataind(ev)         ((ev)->uParam.sDcfmData)
#define asbeacon(ev)          ((ev)->uParam.sIndBeacon)

#ifndef JENNIC_CONF_TIMESYNC
# define USE_TS 0
#else
# define USE_TS JENNIC_CONF_TIMESYNC
#endif

#if USE_TS
hrclock_t current_timestamp = 0;
#endif

#include "ieee802_queue.c"

PROCESS(ieee_process, "Ieee 802.15.4 mac");

typedef enum { NONPRESENT, RESERVED, SHORT, LONG } addrmode;
#define BROADCAST_ADDR  (0xFFFF)
#define BROADCAST_PANID (0xFFFF)

#define MAC_CAPABILITY_ALTERNATE_COORD   (1<<0)
#define MAC_CAPABILITY_IS_FFD            (1<<1)
#define MAC_CAPABILITY_POWER_SOURCE      (1<<2)
#define MAC_CAPABILITY_RECV_ON_WHEN_IDLE (1<<3)
#define MAC_CAPABILITY_SECURITY          (1<<6)
#define MAC_CAPABILITY_ALLOCATE_ADDRESS  (1<<7)
#define UNALLOCATED_SHORT_ADDR           (0xFFFE)

void
ieee_register_lqi_callback(void (*func)(const rimeaddr_t*, uint8_t))
{
  lqicb = func;
}

static bool ieee_started      = false;
static mac_callback_t mac_cb  = NULL;
static void*      mac_cb_ptr  = NULL;

/* contiki mac driver functions */
void /* put packet into transmit buffer */
ieee_send(mac_callback_t cb, void *ptr)
{
  MAC_McpsReqRsp_s    req;
  MAC_McpsSyncCfm_s   cfm;

  if(!ieee_started)
    return;

  mac_cb = cb;
  mac_cb_ptr = ptr;

  /* check buffer size */
  if(packetbuf_datalen() > MAC_MAX_DATA_PAYLOAD_LEN)
    return;

  /* prepare packet request */
  req.u8Type = MAC_MCPS_REQ_DATA;
  req.u8ParamLength = sizeof(MAC_McpsReqData_s);
  req.uParam.sReqData.u8Handle = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
  req.uParam.sReqData.sFrame.u8TxOptions = packetbuf_attr(PACKETBUF_ATTR_RELIABLE) ? MAC_TX_OPTION_ACK : 0;

  /* fill in destination address. */
  if(rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &rimeaddr_null))
  {
    req.uParam.sReqData.sFrame.sDstAddr.u8AddrMode     = SHORT;
    req.uParam.sReqData.sFrame.sDstAddr.u16PanId       = BROADCAST_PANID;
    req.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = BROADCAST_ADDR;
  }
  else
  {
    req.uParam.sReqData.sFrame.sDstAddr.u8AddrMode      = LONG;
    req.uParam.sReqData.sFrame.sDstAddr.u16PanId        = IEEE802154_PANDID;
    req.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32H =
      ((MAC_ExtAddr_s*) packetbuf_addr(PACKETBUF_ADDR_RECEIVER))->u32L;
    req.uParam.sReqData.sFrame.sDstAddr.uAddr.sExt.u32L =
      ((MAC_ExtAddr_s*) packetbuf_addr(PACKETBUF_ADDR_RECEIVER))->u32H;
  }

  /* fill in source address */
  req.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode      = LONG;
  req.uParam.sReqData.sFrame.sSrcAddr.u16PanId        = IEEE802154_PANDID;
  req.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32H =
    ((MAC_ExtAddr_s*) ieee_get_mac())->u32L;
  req.uParam.sReqData.sFrame.sSrcAddr.uAddr.sExt.u32L =
    ((MAC_ExtAddr_s*) ieee_get_mac())->u32H;

//  printf("mac addr: 0x%x%x 0x%x lladdr: 0x%x%x%x%x%x%x%x%x 0x%x%x\n",
//                                    *((uint32_t*) pvAppApiGetMacAddrLocation()),
//                                    *((uint32_t*) pvAppApiGetMacAddrLocation()+1),
//                                    pvAppApiGetMacAddrLocation(),
//                                    (int) uip_lladdr.addr[0],
//                                    (int) uip_lladdr.addr[1],
//                                    (int) uip_lladdr.addr[2],
//                                    (int) uip_lladdr.addr[3],
//                                    (int) uip_lladdr.addr[4],
//                                    (int) uip_lladdr.addr[5],
//                                    (int) uip_lladdr.addr[6],
//                                    (int) uip_lladdr.addr[7],
//                                    *((uint32_t*) uip_lladdr.addr),
//                                    *((uint32_t*) uip_lladdr.addr+1));

  /* copy over payload */
  req.uParam.sReqData.sFrame.u8SduLength = packetbuf_datalen();
  memcpy( req.uParam.sReqData.sFrame.au8Sdu, packetbuf_dataptr(),
          MIN(packetbuf_datalen(), MAC_MAX_DATA_PAYLOAD_LEN) );

  while (bTACframeInProgress())
    ;

  vAppApiMcpsRequest(&req, &cfm);

  switch(cfm.u8Status) {
    case MAC_MCPS_CFM_OK:
      mac_call_sent_callback(mac_cb, mac_cb_ptr, MAC_TX_OK, 1);
      break;
    case MAC_MCPS_CFM_DEFERRED:
      mac_call_sent_callback(mac_cb, mac_cb_ptr, MAC_TX_DEFERRED, 1);
      break;
    default:
      mac_call_sent_callback(mac_cb, mac_cb_ptr, MAC_TX_ERR, 1);
      break;
  }

  return;
}

void /* input is handled in mcps thread */
ieee_recv()
{
}

int /* turn on radio */
ieee_on()
{
  return 0;
}

int /* turn off radio */
ieee_off(int keep_radio)
{
  return 0;
}

void ieee_init();

unsigned short
ieee_channel_check()
{
  return 0;
}

const struct mac_driver ieee_driver = {
  "Ieee 802.15.4 Mac Driver",
  ieee_init,
  ieee_send,
  ieee_recv,
  ieee_on,
  ieee_off,
  ieee_channel_check
};


/* main task of the ieee mac layer */
static void (*beaconrxcb)(MAC_MlmeIndBeacon_s*) = NULL;
static struct pt ieee_mlme, ieee_mcps;

void
ieee_register_beacon_callback(void (*func)(MAC_MlmeIndBeacon_s*))
{
  beaconrxcb = func;
}

static rimeaddr_t*
asrimeaddr(MAC_ExtAddr_s *addr, rimeaddr_t *rime)
{
  CTASSERT(sizeof(*rime) == sizeof(*addr));
  ((MAC_ExtAddr_s*) rime)->u32H = addr->u32L;
  ((MAC_ExtAddr_s*) rime)->u32L = addr->u32H;
  return rime;
}

static bool
req_reset(bool setDefaultPib)
{
  MAC_MlmeReqRsp_s  mlmereq;
  MAC_MlmeSyncCfm_s mlmecfm;

  mlmereq.u8Type        = MAC_MLME_REQ_RESET;
  mlmereq.u8ParamLength = sizeof(MAC_MlmeReqReset_s);
  mlmereq.uParam.sReqReset.u8SetDefaultPib = setDefaultPib;
  vAppApiMlmeRequest(&mlmereq, &mlmecfm);
  return mlmecfm.u8Status!=MAC_MLME_CFM_OK;
}

#define SCAN_ALL_CHANNELS 0x03FFF800UL

static void
req_scan(uint8_t scantype, uint8_t duration)
  /* scan duration is (2**n+1) * 960 symbols, maximum is 14
   * according to JN-RM-2002-802.15.4-Stack-API-1v7.pdf */
{
  MAC_MlmeReqRsp_s  mlmereq;
  MAC_MlmeSyncCfm_s mlmecfm;

  mlmereq.u8Type = MAC_MLME_REQ_SCAN;
  mlmereq.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
  mlmereq.uParam.sReqScan.u8ScanType      = scantype;
  mlmereq.uParam.sReqScan.u32ScanChannels = SCAN_ALL_CHANNELS;
  mlmereq.uParam.sReqScan.u8ScanDuration  = duration % 14;

  vAppApiMlmeRequest(&mlmereq, &mlmecfm);
}

static int
tx_status(MAC_McpsCfmData_s *cfm)
{
  switch (cfm->u8Status) {
    case MAC_ENUM_SUCCESS:                return MAC_TX_OK;
    case MAC_ENUM_TRANSACTION_OVERFLOW:   return MAC_TX_ERR;
    case MAC_ENUM_CHANNEL_ACCESS_FAILURE: return MAC_TX_COLLISION;
    case MAC_ENUM_INVALID_PARAMETER:      return MAC_TX_ERR_FATAL;
    case MAC_ENUM_NO_ACK:                 return MAC_TX_NOACK;
    default:                              return MAC_TX_ERR;
  }
}

static void
ieee_mcpspt(MAC_McpsDcfmInd_s *ev)
  /* packet input and output thread */
{
  rimeaddr_t rime;
  switch(ev->u8Type)
  {
    case MAC_MCPS_IND_DATA:
      /* new frame received */
      packetbuf_clear();
      packetbuf_copyfrom(asdataframe(ev).au8Sdu,
          asdataframe(ev).u8SduLength);
      packetbuf_set_datalen(asdataframe(ev).u8SduLength);
      packetbuf_set_addr(PACKETBUF_ADDR_SENDER,
          asrimeaddr(&asdataframe(ev).sSrcAddr.uAddr.sExt, &rime));

      if(asdataframe(ev).sDstAddr.u8AddrMode==LONG) {
        /* addressed frame */
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,
            asrimeaddr(&asdataframe(ev).sDstAddr.uAddr.sExt, &rime));
      } else if(asdataframe(ev).sDstAddr.u8AddrMode==SHORT &&
         asdataframe(ev).sDstAddr.u16PanId==BROADCAST_PANID &&
         asdataframe(ev).sDstAddr.uAddr.u16Short==BROADCAST_ADDR) {
        /* broadcast frame */
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &rimeaddr_null);
      }

      /* update lqi stuff and call lqi callback */
      packetbuf_set_attr(PACKETBUF_ATTR_RSSI, asdataframe(ev).u8LinkQuality);

      if (asdataframe(ev).sSrcAddr.u8AddrMode == LONG && lqicb)
        lqicb(asrimeaddr(&asdataframe(ev).sSrcAddr.uAddr.sExt, &rime),
              asdataframe(ev).u8LinkQuality);
      else if (lqicb)
        lqicb(NULL, asdataframe(ev).u8LinkQuality);

#if USE_TS
      current_timestamp = asdataframe(ev).timestamp;
#endif

      //{
      //  static char buf[512];
      //  uint8_t i;
      //  uint16_t j;
      //  printf("delay:%d len:%d data:", (int32_t) (clock_hrtime()-asdataframe(ev).timestamp), asdataframe(ev).u8SduLength);
      //  for (i=0,j=0; i<asdataframe(ev).u8SduLength; i++)
      //    j+=snprintf(buf+j,sizeof(buf)-j,"0x%x ",asdataframe(ev).au8Sdu[i]);
      //  buf[j]='\n';

      //  puts(buf);
      //}

      /* call upper layer */
      NETSTACK_NETWORK.input();
      break;
    case MAC_MCPS_DCFM_DATA:
      mac_call_sent_callback(mac_cb, mac_cb_ptr, tx_status(&asdataind(ev)), 1);
      break;
    case MAC_MCPS_DCFM_PURGE:
    default:
      HAL_BREAKPOINT();
      break;
  }
}

/* this defines the ieee_mlmehandler protothread */
#ifdef JENNIC_CONF_COORDINATOR
# include "ieee802_coord.c"
#else
# include "ieee802_ed.c"
#endif
#include "ieee802_serial.c"

static void
ieee_process_poll(void *p)
{
  rxq_tail_complete();
  process_poll(&ieee_process);
}

void
ieee_init()
{
  void *mac;
  MAC_Pib_s *pib;

  if (process_is_running(&ieee_process))
    return;

  /* initialize ieee_eventhandler and event queue*/
  rxq_init();

  /* setup mac <-> app interface */
  u32AppApiInit((PR_GET_BUFFER) rxq_mlme_alloc, (PR_POST_CALLBACK) ieee_process_poll, NULL,
                (PR_GET_BUFFER) rxq_mcps_alloc, (PR_POST_CALLBACK) ieee_process_poll, NULL);

  /* get mac and pib handles */
  mac   = pvAppApiGetMacHandle();
  pib   = MAC_psPibGetHandle(mac);

  /* do a full reset */
  req_reset(true);

  /* set panid and default parameters */
  MAC_vPibSetPanId(mac, IEEE802154_PANDID);
  MAC_vPibSetRxOnWhenIdle(mac, true, false);

  /* allocate an event for this process */
  ieee_event = process_alloc_event();
  pib->bAutoRequest = true;

  /* bandwidth control, smaller interframe gap and higher data rate,
   * this is not standard conform! */
#if defined(__BA2__) && defined(JENNIC_CONF_JN5148_FASTDATARATE)
  vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_1_MBPS);
  vAHI_BbcSetInterFrameGap(48);
#endif

  process_start(&ieee_process, NULL);
}

void*
ieee_get_mac()
{
#ifdef __BA1__
  return (void*) 0x4001000;
#elif defined(__BA2__)
  return (void*) 0x4000d00;
#else
# error "unsupported processor or compiler"
#endif
}


PROCESS_THREAD(ieee_process, ev, data)
{
  static void (*ieee_mlmehandler)(MAC_MlmeDcfmInd_s*);
  static void (*ieee_mcpshandler)(MAC_McpsDcfmInd_s*);

  PROCESS_BEGIN();
  PRINTF("ieee_process: starting\n");

  ieee_init();
  //ieee_serial_init();

  PT_INIT(&ieee_mlme); ieee_mlmehandler = ieee_mlmept;
  PT_INIT(&ieee_mcps); ieee_mcpshandler = ieee_mcpspt;

  /* start the mlme thread by requesting a scan. */
  req_scan(MAC_MLME_SCAN_TYPE_ACTIVE,0);

  PRINTF("ieee_process: started\n");

  /* run until this process is exiting */
  while(true)
  {
    size_t i;
    MAC_DcfmIndHdr_s *macev;

    for(i=0; i<RX_QUEUE_SIZE && (macev=rxq_peek())!=NULL; i++)
    {
      if(rxq_peektype()==MLME) {
        //ieee_serial_mlme((MAC_MlmeDcfmInd_s*) macev);
        ieee_mlmehandler((MAC_MlmeDcfmInd_s*) macev);
      }
      else if(rxq_peektype()==MCPS) {
        //ieee_serial_mcps((MAC_McpsDcfmInd_s*) macev);
        ieee_mcpshandler((MAC_McpsDcfmInd_s*) macev);
      }

      rxq_dequeue();
    }

    if (ev==ieee_event && data == IEEE_STARTED)
      ieee_started = true;

    PROCESS_YIELD();
  }

  PRINTF("ieee_process: exiting\n");
  PROCESS_END();
}

#if USE_TS
hrclock_t ieee_get_last_timestamp()
{
  return current_timestamp;
}
#endif
