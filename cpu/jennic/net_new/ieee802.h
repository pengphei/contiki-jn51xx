/*
 * Copyright (c) 2008
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

#ifndef __JEN_IEEE802_H__
#define __JEN_IEEE802_H__
# include "contiki.h"
# include "contiki-conf.h"
# include "mac.h"
# include "uip.h"
# include "rime.h"
# include <mac_sap.h>
#include  <mac_pib.h>

#ifndef JENNIC_CONF_TIMESYNC
# define USE_TS 0
#else
# define USE_TS JENNIC_CONF_TIMESYNC
#endif

#if USE_TS
# include "hrclock.h"
#endif

# ifndef IEEE802154_CONF_PANDID
#  define IEEE802154_PANDID 0xBEEF
# else
#  define IEEE802154_PANDID IEEE802154_CONF_PANDID
# endif

# ifndef JENNIC_CONF_COORD_BEACON_ORDER
#  define BEACON_ORDER MAC_PIB_BEACON_ORDER_MAX
# else
#  define BEACON_ORDER JENNIC_CONF_COORD_BEACON_ORDER
# endif

# ifndef JENNIC_CONF_COORD_SUPERFRAME_ORDER
#  define SUPERFRAME_ORDER MAC_PIB_SUPERFRAME_ORDER_MAX
# else
#  define SUPERFRAME_ORDER JENNIC_CONF_COORD_SUPERFRAME_ORDER
# endif

# if defined(JENNIC_CONF_COORD_FIXED_CHANNEL) && (JENNIC_CONF_COORD_FIXED_CHANNEL>=0) && (JENNIC_CONF_COORD_FIXED_CHANNEL<=11)
#  error "Fixed Channel configuration for jennic needs to be from 0 to 11!"
#  define RADIO_CHANNEL 0
# elif !defined(JENNIC_CONF_COORD_FIXED_CHANNEL)
#  define RADIO_CHANNEL 0
# else
#  define RADIO_CHANNEL JENNIC_CONF_COORD_FIXED_CHANNEL
# endif

PROCESS_NAME(ieee_process);
const struct mac_driver ieee_driver;

struct ieee_callbacks {
  void (*mlme)(MAC_MlmeDcfmInd_s*);
  void (*mcps)(MAC_McpsDcfmInd_s*);
};

/* register a callback function called on beacon reception */
void ieee_register_beacon_callback(void (*func)(MAC_MlmeIndBeacon_s*));

/* register a callback function called when on each new rssi value */
void ieee_register_lqi_callback(void (*func)(const linkaddr_t*, uint8));

process_event_t ieee_event;
enum ieee_events { IEEE_STARTED, IEEE_STOPPED, IEEE_PAUSE, IEEE_UNPAUSE };

/* get the location of the mac address */
void *ieee_get_mac();

#if USE_TS
/* get the timestamp of the last received packet, since the packetbuf_attr
 * interface only supports 16bit we don't use it for this purpose. */
hrclock_t ieee_get_last_timestamp();
#endif

#endif
