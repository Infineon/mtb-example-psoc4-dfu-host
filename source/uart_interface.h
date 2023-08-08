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

#include "stdint.h"
#include "cy_pdl.h"
#include "cybsp.h"

#ifndef SOURCE_UART_INTERFACE_H_
#define SOURCE_UART_INTERFACE_H_

/*print a string using the UART and put a newline after it*/
void println(const char str[]);
/*print a string using the UART*/
void print(const char str[]);
/*print a buffer in Hex*/
void printHex(uint8_t*, uint32_t);
/*print an integer*/
void printInt(int);

#endif /* SOURCE_UART_INTERFACE_H_ */
