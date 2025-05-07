/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <csi_config.h>
#include "csi_core.h"

#ifdef USE_PSRAM
#include "wm_psram.h"
PSRAM_HandleTypeDef __psram;
#endif

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
// init LED PB24
//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//    uint32_t f = 100000000;

    __set_VBR((uint32_t) & (irq_vectors));

#if defined(CONFIG_SEPARATE_IRQ_SP) && !defined(CONFIG_KERNEL_NONE)
    /* 801 not supported */
	extern int32_t g_top_irqstack;
    __set_Int_SP((uint32_t)&g_top_irqstack);
    __set_CHR(__get_CHR() | CHR_ISE_Msk);
    VIC->TSPR = 0xFF;
#endif

    __set_CHR(__get_CHR() | CHR_IAE_Msk);

    /* Clear active and pending IRQ */
    VIC->IABR[0] = 0x0;
    VIC->ICPR[0] = 0xFFFFFFFF;

#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif

#ifdef USE_PSRAM
    __psram.Init.Div = 3;
    __psram.Init.Mode = PSRAM_MODE_QSPI;
    __psram.Instance  = PSRAM;
    HAL_PSRAM_Init(&__psram);
#else
#warning "PSRAM OFF"
#endif

    //HAL_Delay(50); // такая пауза завешивает проц, попробуем большой цикл...
    //uint32_t f = 100000000;
    //while(f > 0) f--;		// 400 ms

    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_24 | GPIO_PIN_25 | GPIO_PIN_26, GPIO_PIN_RESET);	// LEDs off
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_24 | GPIO_PIN_25 | GPIO_PIN_26);
/*
    __HAL_RCC_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_24 | GPIO_PIN_25 | GPIO_PIN_26;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_24 | GPIO_PIN_25 | GPIO_PIN_26, GPIO_PIN_SET);

    while(f > 0) f--;		// 400 ms

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_24 | GPIO_PIN_25 | GPIO_PIN_26, GPIO_PIN_RESET);	// LEDs off
*/
}
