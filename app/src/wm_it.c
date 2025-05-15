
#include "wm_hal.h"
extern PMU_HandleTypeDef hpmu;

#define readl(addr) ({unsigned int __v = (*(volatile unsigned int *) (addr)); __v;})
__attribute__((isr)) void CORET_IRQHandler(void)
{
    readl(0xE000E010);
    HAL_IncTick();
}


extern UART_HandleTypeDef huart1;

__attribute__((isr)) void UART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

extern UART_HandleTypeDef huart2;

__attribute__((isr)) void UART2_5_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

__attribute__((isr)) void PMU_IRQHandler(void)
{
    HAL_PMU_IRQHandler(&hpmu);
}