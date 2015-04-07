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
 *
 * This module defines the mac management thread. It tries to do a short active
 * scan. Depending on the scan result the network will be joined or the mac will
 * work unassociated until a beacon will be received for a network that can be
 * joined.
 */
static void
req_associate(MAC_PanDescr_s *pan)
{
  MAC_MlmeReqRsp_s  mlmereq;
  MAC_MlmeSyncCfm_s mlmecfm;
  MAC_Pib_s        *pib = MAC_psPibGetHandle(pvAppApiGetMacHandle());

  mlmereq.u8Type = MAC_MLME_REQ_ASSOCIATE;
  mlmereq.u8ParamLength = sizeof(MAC_MlmeReqAssociate_s);
  mlmereq.uParam.sReqAssociate.u8LogicalChan    = pan->u8LogicalChan;
  mlmereq.uParam.sReqAssociate.u8Capability     = MAC_CAPABILITY_IS_FFD|MAC_CAPABILITY_RECV_ON_WHEN_IDLE;
  mlmereq.uParam.sReqAssociate.u8SecurityEnable = pan->u8SecurityUse;
  memcpy(&mlmereq.uParam.sReqAssociate.sCoord, &pan->sCoord, sizeof(pan->sCoord));

  vAppApiMlmeRequest(&mlmereq, &mlmecfm);

  /* needed to receive beacon transfers */
  pib->u16CoordShortAddr = pan->sCoord.uAddr.u16Short;
  pib->bAutoRequest = false;
}

static uint8_t
ieee_findpan(MAC_MlmeDcfmInd_s *ind, MAC_PanDescr_s **pan)
{
  MAC_MlmeCfmScan_s *scan = &ind->uParam.sDcfmScan;
  bool found = false;
  uint8_t i=0;

  *pan = NULL;

  //printf("u8Type: %d, u8ParamLength: %d, u16PanId: %d\n", ind->u8Type, ind->u8ParamLength,
  //    ind->u16Pad);
  //printf("u8Status: %d, u8ScanType: %d, u8ResultListSize: %d, u8Pad: %d, u32UnscannedChannels: %d\n",
  //       scan->u8Status, scan->u8ScanType, scan->u8ResultListSize, scan->u8Pad, scan->u32UnscannedChannels);

  if(ind->u8Type      != MAC_MLME_DCFM_SCAN ||
     scan->u8ScanType != MAC_MLME_SCAN_TYPE_ACTIVE ||
     scan->u8Status   != MAC_ENUM_SUCCESS)
  {
    PRINTF("scan unsuccesful status:%d\n", (int) scan->u8Status);
    return scan->u8Status;
  }

  for(i=0; i<scan->u8ResultListSize; i++)
  {
    /* check if there is a network with the same PANID and if
     * it allows joining
     * TODO: handle the case where we are not allowed to join the
     * network.  */
    *pan = &scan->uList.asPanDescr[i];

    PRINTF("panid: 0x%x\n", (int) ((*pan)->sCoord.u16PanId));

    if( (*pan)->sCoord.u16PanId==IEEE802154_PANDID &&
       ((*pan)->u16SuperframeSpec&0x8000) )
    {
      found=true;
      break;
    }
    else
      *pan = NULL;
  }

  return found;
}

PT_THREAD(ieee_mlmept(MAC_MlmeDcfmInd_s *ev))
  /* mac management thread */
{
  static void *data = NULL;
  static bool associated = false;

  PT_BEGIN(&ieee_mlme);

  do
  {
    PRINTF("ieee_task: requesting active scan\n");

    req_scan(MAC_MLME_SCAN_TYPE_ACTIVE, 4);
    PT_YIELD_UNTIL( &ieee_mlme, ev->u8Type==MAC_MLME_DCFM_SCAN &&
                    (asscan(ev).u8Status==MAC_ENUM_NO_BEACON ||
                    (asscan(ev).u8Status==MAC_ENUM_SUCCESS &&
                     asscan(ev).u8ScanType==MAC_MLME_SCAN_TYPE_ACTIVE) ));

    PRINTF("ieee_task: got active scan result\n");

    /* find a pan to join */
    ieee_findpan(ev, (MAC_PanDescr_s**) &data);
  } while(data==NULL);

  /* if a pan descriptor was found, post a join request */
  PRINTF("ieee_task: pan found -> trying to associate\n");
  req_associate(data);

  /* we are started now */
  process_post(PROCESS_BROADCAST, ieee_event, IEEE_STARTED);

  while(1)
  {
    PT_YIELD(&ieee_mlme);

    switch(ev->u8Type)
    {
      case MAC_MLME_DCFM_ASSOCIATE:
        PRINTF("ieee_task: associated\n");
        associated = true;
        break;
      case MAC_MLME_IND_DISASSOCIATE:
        PRINTF("ieee_task: disasociated\n");
        associated = false;
        break;
      case MAC_MLME_IND_BEACON_NOTIFY:
        PRINTF("ieee_task: beacon notify\n");
        //if(!associated)
        //{
        //  req_associate(&asbeacon(ev).sPANdescriptor);
        //  PRINTF("ieee_task: trying to associate\n");
        //}
        if(beaconrxcb!=NULL)
        {
          beaconrxcb(&asbeacon(ev));
        }

        {
          rimeaddr_t rime;

          if (asbeacon(ev).sPANdescriptor.sCoord.u8AddrMode == LONG && lqicb)
          {
            lqicb(asrimeaddr(&asbeacon(ev).sPANdescriptor.sCoord.uAddr.sExt, &rime),
                  asbeacon(ev).sPANdescriptor.u8LinkQuality);
          }
          else if (lqicb)
          {
            memset(&rime, 0xff, sizeof(rime));
            lqicb(&rime, asbeacon(ev).sPANdescriptor.u8LinkQuality);
          }
        }
        break;
      default:
        HAL_BREAKPOINT();
        break;
    }
  }

  PT_END(&ieee_mlme);
}
