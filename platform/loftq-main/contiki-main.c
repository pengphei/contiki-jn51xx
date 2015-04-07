#include "contiki-net.h"
#include "net/ieee802.h"
#include "gdb2.h"
#include "init.h"
#include "jts.h"

PROCINIT(&etimer_process);

void
init_net(void)
{
  /* load link-layer address */
  memcpy(uip_lladdr.addr,ieee_get_mac(), sizeof(uip_lladdr.addr));
  netstack_init();
}

void
AppColdStart(void)
{
  /* default startup */
  init_hardware();
  process_init();
  procinit_init();
  init_net();
  jts_init();

  /* application startup */
  autostart_start(autostart_processes);

  /* enable watchdog on JN5148, there is none on JN5139 */
#ifdef __BA2__
  watchdog_start();
#endif

  /* default main loop */
  while(1)
  {
    process_run();
    etimer_request_poll();

#ifdef __BA2__
    watchdog_periodic();
#endif
  }
}

void
AppWarmStart(void)
{
  AppColdStart();
}

void
uip_log(char *msg)
{
  printf(msg);
}
