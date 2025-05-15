#include "wm_hal.h"

void HAL_MspInit(void)
{

}

void Error_Handler(void)
{
    printf("Critical error!\n");
    while (1)
    {
    }
}

void HAL_DMA_MspInit(DMA_HandleTypeDef *hdma){
    __HAL_RCC_DMA_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA_Channel0_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel0_IRQn);
    hdma->Instance = DMA_Channel0;
    hdma->Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma->Init.DestInc = DMA_DINC_ENABLE;
    hdma->Init.SrcInc = DMA_SINC_ENABLE;
    hdma->Init.DataAlignment = DMA_DATAALIGN_WORD;
    hdma->Init.Mode = DMA_MODE_NORMAL_SINGLE;
    //hdma->LinkDesc = tx_desc;

    if (HAL_DMA_Init(hdma) != HAL_OK)
    {
        Error_Handler();
    }

}

void HAL_PWM_MspInit(PWM_HandleTypeDef *hpwm)
{
    __HAL_RCC_PWM_CLK_ENABLE();
    __HAL_AFIO_REMAP_PWM0(GPIOB, GPIO_PIN_0);
    __HAL_AFIO_REMAP_PWM1(GPIOB, GPIO_PIN_1);
    __HAL_AFIO_REMAP_PWM2(GPIOB, GPIO_PIN_2);
}

void HAL_PWM_MspDeInit(PWM_HandleTypeDef *hpwm)
{
    __HAL_RCC_PWM_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
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
    if (huart->Instance == UART2){
		__HAL_RCC_UART2_CLK_ENABLE();
		__HAL_RCC_GPIO_CLK_ENABLE();
		__HAL_AFIO_REMAP_UART2_TX(GPIOA, GPIO_PIN_2);
		__HAL_AFIO_REMAP_UART2_RX(GPIOA, GPIO_PIN_3);
		HAL_NVIC_SetPriority(UART2_5_IRQn, 0);
		HAL_NVIC_EnableIRQ(UART2_5_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
	if (huart->Instance == UART1)
	{
		__HAL_RCC_UART1_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
	}
	if (huart->Instance == UART2)
	{
		__HAL_RCC_UART2_CLK_DISABLE();
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
	}
}

void HAL_PMU_MspInit(PMU_HandleTypeDef *hpmu)
{
    HAL_NVIC_EnableIRQ(PMU_IRQn);
}

void HAL_PMU_MspDeInit(PMU_HandleTypeDef *hpmu)
{
    HAL_NVIC_DisableIRQ(PMU_IRQn);
}