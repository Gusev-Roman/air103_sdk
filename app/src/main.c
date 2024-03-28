/******************************************************************************
** 
 * \file        main.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Demo code of PSRAM read/write
 * \note        
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: Air103
 *
******************************************************************************/

#include <stdio.h>
#include "wm_hal.h"
#include "wm_psram.h"
#include "psalloc.h"

PSRAM_HandleTypeDef _psram;

int i, j;

void Error_Handler(void);
// flash-linked const
static char *fish = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

int main(void)
{
    TIM_HandleTypeDef my_tim;
    uint32_t ticks0, ticks1;
    char *membuf1, *membuf2;
    
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");

    _psram.Init.Div = 4; // from 3 to 15 (check APBCLK)
    _psram.Init.Mode = PSRAM_MODE_QSPI;
    _psram.Instance = PSRAM;

    my_tim.Instance = TIM0;
    my_tim.Init.Unit = TIM_UNIT_US;
    my_tim.Init.AutoReload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    my_tim.Init.Period = 1000000; //each us will increase timer
    //__HAL_TIM_ENABLE(&my_tim);
    HAL_TIM_Base_Init(&my_tim); //HAL_TIM_Base_Init
    HAL_TIM_Base_Start(&my_tim);
    /*mem2mem*/
    membuf1 = malloc(0x10000);
    membuf2 = malloc(0x10000);
    if(membuf1 == NULL || membuf2 == NULL){
        printf("malloc error!\n");
        while(1);
    }
    uint32_t *ptimer = (uint32_t *)(TIM_BASE + 0x20);

    ticks0 = *ptimer;
    ticks0 = TIM->TIM0_CNT;
    //ticks0 = HAL_GetTick();
    memcpy(membuf1, membuf2, 0x10000);
    ticks1 = *ptimer;
    ticks1 = TIM->TIM0_CNT;
    //ticks1 = HAL_GetTick();
    printf("mem2mem @64k is %d %d us\n", ticks1, ticks0);
    
    HAL_PSRAM_Init(&_psram);

    init_heap(); // PSRAM alloc init
    char **q = malloc(64 * sizeof(char *));		// array of 64 char * in regular RAM
    for(i=0; i<64; i++){
        q[i] = (char *)psalloc(strlen(fish)+1);
        if(!q){
            printf("psalloc error!\n");
            continue;
        }
        else{
            strcpy(q[i], fish);
            heap_walk();
            printf("q[%d]=[%s]\n", i, q[i]);
        }
        HAL_Delay(1000);        // 1s delay
    }
    for(i=2;i<64;i++) psfree(q[i]);
    heap_walk();
    free(q);
    while(1);	// loop forewer
}

void Error_Handler(void)
{
    while (1)
    {
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}
