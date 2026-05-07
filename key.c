uint8_t Key_Val,Key_Down,Key_Old,Key_Up;
uint8_t Key_Slow_Down;         // 按键减速处理

uint8_t Key_Read()
{
  uint8_t temp = 0;
  if(HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) temp = 1;

  return temp;
}

void key_task()
{
  if(Key_Slow_Down) return;
  Key_Slow_Down = 10;

  Key_Val = Key_Read();
  Key_Down = Key_Val & (Key_Old ^ Key_Val);
  Key_Old = Key_Val;

  switch(Key_Down)
  {
    case 1:

      break;

    default:

      break;
  } 
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{  
  // TIMx 中断处理 (约1ms触发一次)
  if(htim->Instance == TIMx)
  {
    if(Key_Slow_Down) Key_Slow_Down--;
    
  }
}
