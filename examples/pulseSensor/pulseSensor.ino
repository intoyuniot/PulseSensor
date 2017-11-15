// 接线说明
// 心率计
// +                3.3V
// -                GND
// S                A2

#include <PulseSensor.h>


PulseSensor  pulseSensor = PulseSensor(A0); // 脉冲输入信号管脚A2

IntoRobotIntervalTimer pulseTimer;

void pulseISRFun(void)
{
    pulseSensor.PulseISR();
}

void setup()
{
    Serial.begin(115200);
    pulseSensor.begin();
    pulseTimer.begin(pulseISRFun, 2000, uSec);  // 设定中断程序并闪烁LED灯，2ms一次
}

void loop()
{
    Serial.println(pulseSensor.GetBPM());
    delay(1000);
}
