#include "motor.h"
#include "contiki-conf.h"

void motor_init(pstmotor_dev_t dev)
{
    printf("motor init\n");
    vAHI_TimerEnable(dev->u8Timer, dev->u8Prescale, FALSE, FALSE, TRUE);
    //vAHI_TimerClockSelect(dev->u8Timer, FALSE, FALSE);
    vAHI_TimerDIOControl(dev->u8Timer, TRUE);
    vAHI_TimerConfigureOutputs(dev->u8Timer, FALSE, TRUE);

    vAHI_TimerSetLocation(dev->u8Timer, FALSE, TRUE);
}


int motor_speed(pstmotor_dev_t dev)
{

}


void motor_ctrl(pstmotor_dev_t dev, int bON)
{
    printf("motor ctrl\n");
    debug_blink();
    vAHI_TimerStartRepeat(dev->u8Timer, 4, 12);
    //vAHI_TimerStartSingleShot(dev->u8Timer, 4, 8);
}

void motor_fini(pstmotor_dev_t dev)
{
    vAHI_TimerStop(dev->u8Timer);
    vAHI_TimerDisable(dev->u8Timer);
}
