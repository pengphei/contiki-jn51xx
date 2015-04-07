/*
 * Copyright (c) 2009, 2010
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

static bool
rsp_associate(MAC_MlmeIndAssociate_s *ind)
{
  static uint16_t node_addr = 1;
  MAC_MlmeReqRsp_s  mlmereq;
  MAC_MlmeSyncCfm_s mlmecfm;

  mlmereq.u8Type = MAC_MLME_RSP_ASSOCIATE;
  mlmereq.u8ParamLength = sizeof(MAC_MlmeRspAssociate_s);
  mlmereq.uParam.sRspAssociate.sDeviceAddr.u32H  = ind->sDeviceAddr.u32H;
  mlmereq.uParam.sRspAssociate.sDeviceAddr.u32L  = ind->sDeviceAddr.u32L;
  mlmereq.uParam.sRspAssociate.u8SecurityEnable  = false;
  mlmereq.uParam.sRspAssociate.u8Status          = 0; /* assocication successful */
  if(ind->u8Capability & MAC_CAPABILITY_ALLOCATE_ADDRESS)
    mlmereq.uParam.sRspAssociate.u16AssocShortAddr = node_addr++;
  else
    mlmereq.uParam.sRspAssociate.u16AssocShortAddr = UNALLOCATED_SHORT_ADDR;

  vAppApiMlmeRequest(&mlmereq, &mlmecfm);
  return mlmecfm.u8Status == MAC_MLME_CFM_NOT_APPLICABLE;
}

static void
req_start(uint8_t chan, bool coord)
  /* this function configures the coordinator.
   * NOTE: when in periodic mode, active scan will NOT be answered immediatly.
   */
{
  void      *mac = pvAppApiGetMacHandle();
  MAC_Pib_s *pib = MAC_psPibGetHandle(mac);

  MAC_MlmeReqRsp_s  mlmereq;
  MAC_MlmeSyncCfm_s mlmecfm;

  MAC_vPibSetShortAddr(mac, 0x0000);
  pib->bAssociationPermit = true;
  memcpy(&pib->sCoordExtAddr, ieee_get_mac(), sizeof(MAC_ExtAddr_s));

  mlmereq.u8Type = MAC_MLME_REQ_START;
  mlmereq.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
  mlmereq.uParam.sReqStart.u16PanId          = IEEE802154_PANDID;
  mlmereq.uParam.sReqStart.u8Channel         = chan;
  mlmereq.uParam.sReqStart.u8BeaconOrder     = BEACON_ORDER;
  mlmereq.uParam.sReqStart.u8SuperframeOrder = SUPERFRAME_ORDER;
  mlmereq.uParam.sReqStart.u8PanCoordinator  = coord;
  mlmereq.uParam.sReqStart.u8BatteryLifeExt  = false;
  mlmereq.uParam.sReqStart.u8Realignment     = false;
  mlmereq.uParam.sReqStart.u8SecurityEnable  = false;

  vAppApiMlmeRequest(&mlmereq, &mlmecfm);
}

static uint8_t
ieee_findsilentchan(MAC_MlmeDcfmInd_s *ind, uint32_t channelmask)
{
  MAC_MlmeCfmScan_s *scan = &ind->uParam.sDcfmScan;
  uint8_t i, minchan=0;

  if(ind->u8Type      != MAC_MLME_DCFM_SCAN ||
     scan->u8ScanType != MAC_MLME_SCAN_TYPE_ENERGY_DETECT ||
     scan->u8Status   != MAC_ENUM_SUCCESS)
    return 0;

  /* find most silent unmasked channel */
  for(i=0; i<scan->u8ResultListSize; i++)
  {
    if(scan->uList.au8EnergyDetect[i] < scan->uList.au8EnergyDetect[minchan]
       && (channelmask & (1<<i)))
      minchan=i;
  }

  return minchan+11;
}

#define SCAN_DURATION 4

PT_THREAD(ieee_mlmept(MAC_MlmeDcfmInd_s *ev))
  /* mac management thread */
{
  PT_BEGIN(&ieee_mlme);
  PRINTF("ieee_task: starting as coord\n");

#if (RADIO_CHANNEL==0)
  uint32_t channels = SCAN_ALL_CHANNELS;

  do
  {
    uint8_t i=0;

    PRINTF("ieee_task: requesting active scan\n");
    req_scan(MAC_MLME_SCAN_TYPE_ACTIVE, SCAN_DURATION);

    PT_YIELD(&ieee_mlme);

    if (ev->u8Type != MAC_MLME_DCFM_SCAN ||
        (asscan(ev).u8Status != MAC_ENUM_SUCCESS &&
         asscan(ev).u8Status != MAC_ENUM_NO_BEACON) ||
        asscan(ev).u8ScanType != MAC_MLME_SCAN_TYPE_ACTIVE)
      continue;

    if (asscan(ev).u8Status == MAC_ENUM_NO_BEACON)
      break;

    /* for each found panid, delete the allocated channel from the
     * freechannel list by unsetting the correspondig bit */
    for (i=0; i<asscan(ev).u8ResultListSize; i++)
      channels &= ~(1<<(asscan(ev).uList.asPanDescr[i].u8LogicalChan));

   } while(asscan(ev).u8ResultListSize == MAC_MAX_SCAN_PAN_DESCRS);

  do /* energy scan on all unallocated channels */
  {
    PRINTF("ieee_task: requesting energy scan\n");
    req_scan(MAC_MLME_SCAN_TYPE_ENERGY_DETECT, SCAN_DURATION);
    PT_YIELD(&ieee_mlme);
  } while(ev->u8Type != MAC_MLME_DCFM_SCAN ||
          asscan(ev).u8Status != MAC_ENUM_SUCCESS ||
          asscan(ev).u8ScanType != MAC_MLME_SCAN_TYPE_ENERGY_DETECT);

  PRINTF("ieee_task: got energy scan result\n");
  req_start(ieee_findsilentchan(ev, channels), true);
  //PRINTF("ieee_task: started as coord on silent channel %d\n", ieee_findsilentchan(ev));
#else
  req_start(RADIO_CHANNEL, true);
  PRINTF("ieee_task: started as coord on fixed channel %d\n", RADIO_CHANNEL);
#endif

  /* we are started now */
  process_post(PROCESS_BROADCAST, ieee_event, IEEE_STARTED);
  PRINTF("ieee_task: start broadcast\n");

  while(1)
  {
    PT_YIELD(&ieee_mlme);

    switch(ev->u8Type)
    {
      case MAC_MLME_IND_ASSOCIATE:
        while(!rsp_associate(&asindassociate(ev)))
          ;
        PRINTF("ieee_task: new node joined\n");
        break;
      case MAC_MLME_IND_COMM_STATUS:
        PRINTF("ieee_task: comm status rcvd\n");
        break;
      case MAC_MLME_IND_BEACON_NOTIFY:
        //PRINTF("ieee_task: beacon notify\n");
        break;
      default:
        HAL_BREAKPOINT();
        break;
    }
  }

  PT_END(&ieee_mlme);
}
