#include "app_main.h"
#include "contiki-conf.h"
#include "motor.h"

static stmotor_dev_t motor1 = {
    E_AHI_DEVICE_TIMER3,
    8,
};

void app_init()
{
    printf("app init\n");
    vAHI_TimerStop(E_AHI_DEVICE_TIMER0);
    vAHI_TimerDisable(E_AHI_DEVICE_TIMER0);
    //motor_init(&motor1);
    //motor_ctrl(&motor1, 0);
}


void app_loop()
{

}
