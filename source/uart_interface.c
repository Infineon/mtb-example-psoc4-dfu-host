/*
 * Copyright 2011-2022 Cypress Semiconductor Corporation (an Infineon company)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "uart_interface.h"
#include "stdio.h"

void println(const char str[])
{
    Cy_SCB_UART_PutString(UART_HW, str);
    Cy_SCB_UART_PutString(UART_HW, "\n\r");
}

void print(const char str[])
{
    Cy_SCB_UART_PutString(UART_HW, str);
}

void printHex(uint8_t buf[], uint32_t len)
{
    char str[4];
    for(int i=0;i<len;i++)
    {
        sprintf(str, "%.2X ", buf[i]);
        Cy_SCB_UART_PutString(UART_HW, str);
    }
}

void printInt(int num)
{
    char str[20];
    sprintf(str, "%d", num);
    Cy_SCB_UART_PutString(UART_HW, str);
}


