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
DMA_HandleTypeDef hdma_ram_tx;
static DMA_LinkDescriptor tx_desc[2];

int i, j;

void Error_Handler(void);
// flash-linked const
static char *fish = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
DMA_HandleTypeDef hdma_ram_tx;
DMA_HandleTypeDef hdma_ram_rx;

static void DMA_Init(void)
{
    __HAL_RCC_DMA_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA_Channel0_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel0_IRQn);

    HAL_NVIC_SetPriority(DMA_Channel1_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel1_IRQn);
}

int main(void)
{
    TIM_HandleTypeDef my_tim;
    uint32_t ticks0, ticks1;
    char *membuf1, *membuf2;

    SystemClock_Config(CPU_CLK_240M);
    printf("enter main\r\n");

    _psram.Init.Div = 3; // from 3 to 15 (check APBCLK)
    _psram.Init.Mode = PSRAM_MODE_QSPI;
    _psram.Instance = PSRAM;

    my_tim.Instance = TIM0;
    my_tim.Init.Unit = TIM_UNIT_US;
    my_tim.Init.AutoReload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    my_tim.Init.Period = 4000000; //each us will increase timer

    __HAL_RCC_TIM_CLK_ENABLE();	// enable timer clocking!
    HAL_TIM_Base_Init(&my_tim); //HAL_TIM_Base_Init

    /*mem2mem*/
    membuf1 = malloc(0x10000);
    membuf2 = malloc(0x10000);
    if(membuf1 == NULL || membuf2 == NULL){
        printf("malloc error!\n");
        while(1);
    }
    HAL_TIM_Base_Start(&my_tim);	// start counter (memset & memcpy)
    memset(membuf2, 'A', 0x10000);
    ticks0 = TIM->TIM0_CNT;
    memcpy(membuf1, membuf2, 0x10000); // 'A' to membuf1
    ticks1 = TIM->TIM0_CNT;

    printf("mem2mem @64k is %dus\n", ticks1-ticks0);
    printf("Calculated value is %3.3f MB/s\n", 1000000.0/((ticks1-ticks0)*16)); // 64-128-256-512-1024

    HAL_PSRAM_Init(&_psram);

    init_heap(); // PSRAM alloc init

    char *psblock = psalloc(0x10000);
    if(psblock == NULL){
       printf("psalloc error!\n");
       while(1);
    }
    //memset(psblock, 'R', 0x10000);
    ticks0 = TIM->TIM0_CNT;
    memcpy(psblock, membuf1, 0x10000);	// 'A' to psblock
    ticks1 = TIM->TIM0_CNT;

    printf("mem2psram @64k is %dus\n", ticks1-ticks0);
    printf("Calculated value is %3.3f MB/s\n", 1000000.0/((ticks1-ticks0)*16));

    DMA_Init();

    hdma_ram_tx.Instance = DMA_Channel0;
    hdma_ram_tx.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_ram_tx.Init.DestInc = DMA_DINC_ENABLE;
    hdma_ram_tx.Init.SrcInc = DMA_SINC_ENABLE;
    hdma_ram_tx.Init.DataAlignment = DMA_DATAALIGN_WORD;
    hdma_ram_tx.Init.Mode = DMA_MODE_NORMAL_SINGLE;
    hdma_ram_tx.Init.RequestSourceSel = 0; // ???

    hdma_ram_tx.LinkDesc = tx_desc;	// ???

    if (HAL_DMA_Init(&hdma_ram_tx) != HAL_OK)
    {
        Error_Handler();
    }
    //__HAL_LINKDMA(hi2s, hdmatx, hdma_i2s_tx);
    memset(membuf1, '5', 0x10000);
    HAL_DMA_Start(&hdma_ram_tx, (uint32_t)membuf1, (uint32_t)psblock, 0x8000); // 32 kWords (???) '5' to psblock

    if(HAL_OK == HAL_DMA_PollForTransfer(&hdma_ram_tx, HAL_DMA_FULL_TRANSFER, 2000)) { // timeout in ms?
        //HAL_DMA_Start(&hdma_ram_tx, (uint32_t)membuf1+0x8000, (uint32_t)psblock+0x8000, 0x8000);
        //if(HAL_DMA_PollForTransfer(&hdma_ram_tx, HAL_DMA_FULL_TRANSFER, 1000) == HAL_OK){
            printf("DMA Transfer OK!\n");
            printf("Last byte: %x\n", psblock[0x7FFF]);
            printf("Next byte: %x\n", psblock[0x8000]);
        //}
        //else{
        //    printf("Error: HAL_DMA_PollForTransfer(2)\n");
        //}
    }
    else{
        printf("Error: HAL_DMA_PollForTransfer(1)\n");
    }
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
    printf("Critical error!\n");
    while (1)
    {
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}
