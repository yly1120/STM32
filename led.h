#ifndef _LED_H
#define _LED_H

#include "main.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include "stm32f1xx_hal_def.h"


typedef struct
{
    GPIO_TypeDef *GPIOx;          /* LED 所在 GPIO 端口 */
    uint16_t GPIO_Pin;            /* LED 引脚 */
    GPIO_PinState active_level;   /* LED 点亮电平：高电平亮 or 低电平亮 */
    uint32_t last_toggle_tick;    /* 上一次翻转时刻 */
} LED_HandleTypeDef;

void LED_InitHandle(LED_HandleTypeDef *hled,
                    GPIO_TypeDef *GPIOx,
                    uint16_t GPIO_Pin,
                    GPIO_PinState active_level);

void LED_On(LED_HandleTypeDef *hled);
void LED_Off(LED_HandleTypeDef *hled);
void LED_Toggle(LED_HandleTypeDef *hled, uint32_t interval_ms);



#endif /* _LED_H */
