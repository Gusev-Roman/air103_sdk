#include "wm_hal.h"

void HAL_MspInit(void)
{

}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
	if (huart->Instance == UART1)
	{
		__HAL_RCC_UART1_CLK_ENABLE();
		__HAL_RCC_GPIO_CLK_ENABLE();
		
	//	PB6: UART1_TX
	//	PB7: UART1_RX
		__HAL_AFIO_REMAP_UART1_TX(GPIOB, GPIO_PIN_6);
		__HAL_AFIO_REMAP_UART1_RX(GPIOB, GPIO_PIN_7);
		HAL_NVIC_SetPriority(UART1_IRQn, 0);
		HAL_NVIC_EnableIRQ(UART1_IRQn);
	}
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    __HAL_RCC_TIM_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM_IRQn, 0);
    HAL_NVIC_EnableIRQ(TIM_IRQn);
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
    // 由于所有的TIM共用一个时钟和中断，所以如果只用了一路TIM，在DeInit时可以关闭时钟和中断，但如果使用了多路TIM，在某一路DeInit时，如果
    // 关闭时钟和中断，会影响其他在运行的TIM
//    __HAL_RCC_TIM_CLK_DISABLE();
//    HAL_NVIC_DisableIRQ(TIM_IRQn);
}