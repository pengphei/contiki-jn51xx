#ifndef MOTOR0_H
#define MOTOR0_H

#include "contiki-conf.h"

typedef struct
{
    //timer params
    uint8 u8Timer;
    uint8 u8Prescale;
}stmotor_dev_t, *pstmotor_dev_t;

void motor_init(pstmotor_dev_t dev);
int motor_speed(pstmotor_dev_t dev);
void motor_ctrl(pstmotor_dev_t dev, int bON);


#endif // MOTOR0_H
