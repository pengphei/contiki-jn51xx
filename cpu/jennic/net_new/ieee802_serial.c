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
 *
 * This is linux-zigbee compatible packet driver that can be attached to the
 * serial line.
 *
 * Protocol is a extended version of
 * http://sourceforge.net/apps/trac/linux-zigbee/wiki/SerialV1:
 *
 * Open: Power up tranceiver, etc.
 *  host: 'z' 'b' 0x01
 *  response: 'z' 'b' 0x81 <status>
 * Close: Power down tranceiver, shut down current operations, etc.
 *  host: 'z' 'b' 0x02
 *  response: 'z' 'b' 0x82 <status>
 * Set Channel: Change used channel. chan# ranges from 1 to 16, which corresponds to IEEE 802.15.4 page 0 channels 11 to 26.
 *  host: 'z' 'b' 0x04 <chan#>
 *  response: 'z' 'b' 0x84 <status>
 * ED: Start ED measurement.
 *  host: 'z' 'b' 0x05
 *  response: 'z' 'b' 0x85 <status> <level>
 * CCA: Perform CCA on the channel. Result is returned as a part of the status
 *  host: 'z' 'b' 0x06
 *  response: 'z' 'b' 0x86 <status>
 * Set State: Set the tranceiver to specified <state>.
 *  host: 'z' 'b' 0x07 <state>
 *  response: 'z' 'b' 0x87 <status>
 * Transmit Block: Transmit a block of data. <len> is the length of <data> block. <data> is the MAC frame w/o FCS (it should be autocalculated by dongle).
 *  host: 'z' 'b' 0x09 <len> <data * len>
 *  response: 'z' 'b' 0x89 <status>
 * Receive block: The only message that is initiated by the dongle. Indicates received block. <lqi> is LQI measured during reception, <len> is the length of <data> block. <data> is the MAC frame w/o FCS
 *  dongle: 'z' 'b' 0x8b <lqi> <len> <data * len>
 *  response: 'z' 'b' 0x0b <status>
 * Get 64-bit address: Request a 64-bit address from the dongle.
 *  host: 'z' 'b' 0x0d
 *  response: 'z' 'b' 0x8d <status> <8 bytes of hw address>
 *
 * <status> (as per 802.15.4)
 *  SUCCESS = 0x00
 *  RX_ON = 0x01
 *  TX_ON = 0x02
 *  TRX_OFF = 0x03
 *  IDLE = 0x04
 *  BUSY = 0x05
 *  BUSY_RX = 0x06
 *  BUSY_TX = 0x07
 *  ERR = 0x08
 *
 * <state>
 *  RX_MODE = 0x02
 *  TX_MODE = 0x03
 *  FORCE_TRX_OFF = 0xF0
 */

typedef enum {
  SUCCESS = 0x00,
  RX_ON,
  TX_ON,
  TRX_OFF,
  IDLE,
  BUSY,
  BUSY_RX,
  BUSY_TX,
  ERR
} status;

typedef enum {
  RX_MODE = 0x02,
  TX_MODE = 0x03,
  FORCE_TRX_OFF = 0xF0
} state;

static struct pt ieee_serial_pt;

/* controls if packets will be sent on the serial line */
static bool      ieee_serial_open=false;

#include <dev/uart0.h>
#include <pt.h>

PT_THREAD(ieee_serial_input(u8_t c))
{
  PT_BEGIN(&ieee_serial_pt);

  /* head of command rx'd */
  PT_WAIT_UNTIL(&ieee_serial_pt,c=='z');
  PT_WAIT_UNTIL(&ieee_serial_pt,c=='b');
  PT_YIELD(&ieee_serial_pt); /* wait for next character */

  /* no case statements inside of protothreads! */
  if (c==0x01)      /* open */
  {
    ieee_serial_open = true;
    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x81);
    uart0_writeb(SUCCESS);
  }
  else if (c==0x02) /* close */
  {
    ieee_serial_open = false;
    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x82);
    uart0_writeb(SUCCESS);
  }
  else if (c==0x04) /* set channel */
  {
    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x84);
    PT_YIELD(&ieee_serial_pt); /* wait for next character */
    uart0_writeb(
      eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL,(10+c)==PHY_ENUM_SUCCESS)
      ? SUCCESS : ERR);
  }
  else if (c==0x05) /* ed scan */
  {
    /* anwer will be deferred and sent through ieee_serial_event! */
    uint32_t cur_channel;
    eAppApiPlmeGet(PHY_PIB_ATTR_CURRENT_CHANNEL, &cur_channel);
    cur_channel -= 10;
    cur_channel %= 16;
    {
      MAC_MlmeReqRsp_s  mlmereq;
      MAC_MlmeSyncCfm_s mlmecfm;
      mlmereq.u8Type = MAC_MLME_REQ_SCAN;
      mlmereq.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
      mlmereq.uParam.sReqScan.u8ScanType      = MAC_MLME_SCAN_TYPE_ENERGY_DETECT;
      mlmereq.uParam.sReqScan.u32ScanChannels = (1<<cur_channel);
      mlmereq.uParam.sReqScan.u8ScanDuration  = 4;
      vAppApiMlmeRequest(&mlmereq, &mlmecfm);
    }
  }
  else if (c==0x06) /* cca */
  {
    /* no-op, there is no support for that, perhaps at least BUSY_RX or BUSY_TX? */
    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x86);
    uart0_writeb(SUCCESS);
  }
  else if (c==0x07) /* set transceiver state */
  {
    /* no-op, got no control over that */
    PT_YIELD(&ieee_serial_pt); /* rx one byte */
    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x87);
    uart0_writeb(SUCCESS);
  }
  else if (c==0x09) /* transmit data block */
  {
    uint8_t i;
    MAC_McpsReqRsp_s    req;
    MAC_McpsSyncCfm_s   cfm;

    /* some values for the mac request */
    req.u8Type = MAC_MCPS_REQ_DATA;
    req.u8ParamLength = sizeof(MAC_McpsReqData_s);
    req.uParam.sReqData.u8Handle = 0;

    /* TODO: decode frame and put into request */
    PT_YIELD(&ieee_serial_pt);
    req.uParam.sReqData.sFrame.u8SduLength = c;

    for(i=0; i<req.uParam.sReqData.sFrame.u8SduLength; i++)
    {
      PT_YIELD(&ieee_serial_pt); /* wait for next char */
      req.uParam.sReqData.sFrame.au8Sdu[i]=c;
    }

    /* XXX: this should be safe, because there is only ?one? irq-level on the
     * jennic platform? */
    vAppApiMcpsRequest(&req, &cfm);

    if (cfm.u8Status==MAC_MCPS_CFM_OK)
    {
      uart0_writeb('z');
      uart0_writeb('b');
      uart0_writeb(0x89);
      uart0_writeb(SUCCESS);
    }
    else if (cfm.u8Status==MAC_MCPS_CFM_DEFERRED)
    {
      // confirm is deferred
      // will be handled in ieee_serial_mcps()
    }
    else
    {
      uart0_writeb('z');
      uart0_writeb('b');
      uart0_writeb(0x89);
      uart0_writeb(ERR);
    }
  }
  else if (c==0x0d) /* request address */
  {
    uint8_t i, *addr = ieee_get_mac();

    uart0_writeb('z');
    uart0_writeb('b');
    uart0_writeb(0x8d);
    uart0_writeb(SUCCESS);

    for(i=0; i<8; i++)
      uart0_writeb(addr[i]);
  }
  else              /* unknown req */
  {}

  PT_END(&ieee_serial_pt);
}

static void
ieee_serial_mlme(MAC_MlmeDcfmInd_s *ev)
  /* this functions gets called on a new MCPS event from the main handler in
   * ieee802.c. Primarily when new packets arrive. */
{
  if(!ieee_serial_open)
    return;

  switch(ev->u8Type)
  {
    case MAC_MLME_DCFM_SCAN:
      uart0_writeb('z');
      uart0_writeb('b');
      uart0_writeb(0x85);
      if (asscan(ev).u8Status == MAC_ENUM_SUCCESS) {
        uart0_writeb(SUCCESS);
        uart0_writeb(asscan(ev).uList.au8EnergyDetect[0]);
      }
      else {
        uart0_writeb(ERR);
        uart0_writeb(0x00);
      }
      break;
    default:
      break;
  }

}

static void
ieee_serial_mcps(MAC_McpsDcfmInd_s *ev)
  /* this funtion gets called on a MLME event from the main handler in
   * ieee802.c. Primarily after scan requests. */
{
  uint8_t i;

  if(!ieee_serial_open)
    return;

  switch(ev->u8Type)
  {
    case MAC_MCPS_IND_DATA:   /* new packet rx'd */
      uart0_writeb('z');
      uart0_writeb('b');
      uart0_writeb(0x86);
      uart0_writeb(asdataframe(ev).u8LinkQuality);
      uart0_writeb(asdataframe(ev).u8SduLength);

      /* data frame without fcs */
      for (i=0; i<asdataframe(ev).u8SduLength; i++)
       uart0_writeb(asdataframe(ev).au8Sdu[i]);
      break;
    case MAC_MCPS_DCFM_DATA:  /* deferred confirm of transmission */
      uart0_writeb('z');
      uart0_writeb('b');
      uart0_writeb(0x89);
      if (asdataind(ev).u8Status==MAC_ENUM_SUCCESS)
        uart0_writeb(SUCCESS);
      else
        uart0_writeb(ERR);
    case MAC_MCPS_DCFM_PURGE: /* deferred confirm of purge request */
    default:
      break;

  }
}

static void
ieee_serial_init()
  /* open up the serial line, ieee stack is getting initialized in ieee802.c
   * baudrate is set to 2MBaud to make sure we can fit the theoretical maximum
   * bandwidth of the ieee802.15.4 stack (666kbps ~= 84 kb/sec). */
{
  //uart0_init(2000000);
  //uart0_init(115200);
  uart0_init(57600);
  uart0_set_input(ieee_serial_input);
}
