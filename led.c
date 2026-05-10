#include "led.h"


/**
 * @brief 获取 LED 熄灭时对应的 GPIO 电平
 * @param hled LED 句柄指针，active_level 表示 LED 点亮时的有效电平
 * @retval GPIO_PinState LED 熄灭时应输出的电平
 * @note
 *         若 LED 为高电平点亮，则熄灭电平为 GPIO_PIN_RESET；
 *         若 LED 为低电平点亮，则熄灭电平为 GPIO_PIN_SET。
 *         本函数用于统一适配高电平有效和低电平有效两种 LED 硬件连接方式。
 */
static GPIO_PinState LED_GetInactiveLevel(const LED_HandleTypeDef *hled)
{
    if(hled->active_level == GPIO_PIN_SET)
    {
        return GPIO_PIN_RESET;
    }
    else
    {
        return GPIO_PIN_SET;
    }
}

/**
 * @brief  初始化 LED 句柄，将指定 LED 的硬件信息和运行参数绑定到句柄中
 *
 * @param  hled: 指向 LED 句柄的指针
 *               该句柄用于保存当前 LED 的控制信息，包括 GPIO 端口、引脚号、
 *               点亮有效电平以及非阻塞翻转所需的时间戳
 *
 * @param  GPIOx: LED 所连接的 GPIO 端口
 *
 * @param  GPIO_Pin: LED 所连接的 GPIO 引脚
 *
 * @param  active_level: LED 点亮时对应的有效电平
 *                       - GPIO_PIN_SET   : 高电平点亮
 *                       - GPIO_PIN_RESET : 低电平点亮
 *
 * @retval None
 *
 * @note
 *         1. 本函数只负责“初始化 LED 软件句柄”，不负责 GPIO 外设本身的初始化。
 *            因此在调用本函数之前，应确保对应 GPIO 已经由 MX_GPIO_Init()
 *            或其他底层初始化代码正确配置为输出模式。
 *
 *         2. 本函数内部会保存当前系统时基 HAL_GetTick() 到 last_toggle_tick，
 *            作为后续 LED_Toggle() 非阻塞翻转的初始参考时间。
 *            这样做可以避免句柄刚初始化后立即发生一次翻转。
 *
 *         3. 本函数对输入参数做了基本有效性检查：
 *            - 若 hled 为 NULL，说明句柄指针无效
 *            - 若 GPIOx 为 NULL，说明 GPIO 端口无效
 *            在这两种情况下函数将直接返回，不执行初始化操作。
 *
 *         4. 本函数不会主动点亮或熄灭 LED，只是建立 LED 与句柄之间的映射关系。
 *            LED 的实际输出状态仍需通过 LED_On()、LED_Off() 或 LED_Toggle() 控制。
 *
 *         5. 若系统尚未完成 HAL_Init()，则 HAL_GetTick() 的时基可能未正常工作，
 *            会影响非阻塞翻转的时间行为。因此建议在 HAL_Init() 之后调用本函数。
 */
void LED_InitHandle(LED_HandleTypeDef *hled, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState active_level)
{
    if(hled == NULL || GPIOx == NULL)
    {
        return;
    }

    hled->GPIOx = GPIOx;
    hled->GPIO_Pin = GPIO_Pin;
    hled->active_level = active_level;
    hled->last_toggle_tick = HAL_GetTick();
}

/**
 * @brief  点亮指定的 LED
 *
 * @param  hled: 指向 LED 句柄的指针
 *               该句柄中保存了 LED 对应的 GPIO 端口、引脚号以及点亮有效电平
 *
 * @retval None
 *
 * @note
 *         1. 本函数通过读取句柄中的 active_level 成员，向对应 GPIO 输出
 *            LED 的“点亮电平”，从而实现对不同硬件接法 LED 的统一控制。
 *
 *         2. 若 LED 为高电平有效，则 active_level 应为 GPIO_PIN_SET，
 *            本函数会输出高电平点亮 LED。
 *
 *         3. 若 LED 为低电平有效，则 active_level 应为 GPIO_PIN_RESET，
 *            本函数会输出低电平点亮 LED。
 */
void LED_On(LED_HandleTypeDef *hled)
{
    if(hled == NULL || hled->GPIOx == NULL)
    {
        return;
    }

    HAL_GPIO_WritePin(hled->GPIOx, hled->GPIO_Pin, hled->active_level);

}

/**
 * @brief  熄灭 LED
 *
 * @param  hled: LED 句柄指针，内部保存 LED 的 GPIO 端口、引脚和点亮有效电平
 *
 * @retval None
 * @note
 *         1. 本函数通过调用 LED_GetInactiveLevel() 获取 LED 的非激活电平，
 *            并将该电平输出到对应 GPIO 引脚，从而实现熄灭 LED。
 *         2. 非激活电平是“点亮有效电平”的相反状态：
 *            - 若 LED 为高电平点亮（active_level = GPIO_PIN_SET），
 *              则熄灭时输出 GPIO_PIN_RESET
 *            - 若 LED 为低电平点亮（active_level = GPIO_PIN_RESET），
 *              则熄灭时输出 GPIO_PIN_SET
 *         3. 这种写法可以统一兼容高电平有效和低电平有效两种 LED 硬件连接方式，
 *            避免在上层代码中写死 GPIO_PIN_SET 或 GPIO_PIN_RESET。
 *         4. 调用前应确保：
 *            - hled 不是空指针
 *            - hled->GPIOx 已正确初始化
 *            - 对应 GPIO 已配置为输出模式
 */
void LED_Off(LED_HandleTypeDef *hled)
{
    if(hled == NULL || hled->GPIOx == NULL)
    {
        return;
    }

    HAL_GPIO_WritePin(hled->GPIOx, hled->GPIO_Pin, LED_GetInactiveLevel(hled));

}

/**
 * @brief  以非阻塞方式按指定时间间隔翻转 LED 状态
 * @param  hled: LED 句柄指针，内部保存 LED 的 GPIO 端口、引脚以及上次翻转时间戳
 * @param  interval_ms: LED 翻转时间间隔，单位为毫秒
 * @retval None
 * @note
 *         1. 本函数采用基于系统节拍 HAL_GetTick() 的非阻塞定时方式，
 *            不使用 HAL_Delay()，因此不会阻塞 CPU，可在主循环中反复调用。
 *
 *         2. 函数每次被调用时，都会读取当前系统时间 now_tick，
 *            并与上一次翻转时刻 last_toggle_tick 作差：
 *            - 若时间差大于等于 interval_ms，则执行一次翻转
 *            - 若时间未到，则直接返回
 *
 *         3. 该函数适用于需要周期性闪烁 LED 的场景，例如状态指示、心跳灯、
 *            故障告警灯等，且不会影响主循环中其他任务的执行实时性。
 *
 *         4. 当满足翻转条件后，函数会更新 last_toggle_tick = now_tick，
 *            用于记录本次翻转时间，作为下一次翻转的参考基准。
 *
 *         5. 调用前应确保：
 *            - GPIO 已正确初始化为输出模式
 *            - LED 句柄已通过 LED_InitHandle() 完成初始化
 *            - 系统时基 HAL_GetTick() 已正常工作（通常要求先执行 HAL_Init()）
 *
 *         6. 本函数本质上是“查询式软件定时”实现，不会自动后台运行，
 *            必须在主循环或周期任务中持续调用，LED 才会按照设定周期翻转。
 */
void LED_Toggle(LED_HandleTypeDef *hled, uint32_t interval_ms)
{
    uint32_t now_tick;

    if(hled == NULL || hled->GPIOx == NULL)
    {
        return;
    }

    if(interval_ms ==0U)
    {
        return;
    }

    now_tick = HAL_GetTick();

    if((now_tick - hled->last_toggle_tick) >= interval_ms)
    {
        HAL_GPIO_TogglePin(hled->GPIOx, hled->GPIO_Pin);
        hled->last_toggle_tick = now_tick;
    }

}

// 使用示例
LED_HandleTypeDef g_led1;

int main()
{
    ...
    LED_InitHandle(&g_led1, LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    LED_Off(&g_led1);

    while(1)
    {
        LED_Toggle(&g_led1, 500U);
    }

    return 0;
}