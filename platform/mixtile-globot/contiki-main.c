#include "contiki-net.h"
#include "ieee802.h"
#include "gdb2.h"
#include "init.h"
#include "jts.h"
#include "globot.h"

void
AppColdStart(void)
{
    app_setup();
    app_loop();
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
