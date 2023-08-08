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

#ifndef SOURCE_I2CMASTER_H_
#define SOURCE_I2CMASTER_H_

#include "cy_pdl.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define I2C_SUCCESS             (0UL)
#define I2C_FAILURE             (1UL)

#define TRANSFER_CMPLT          (0x00UL)
#define READ_CMPLT              (TRANSFER_CMPLT)
/* Packet positions */

/* Start and end of packet markers */
#define PACKET_SOP              (0x01UL)
#define PACKET_EOP              (0x17UL)

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Function Name: WriteData
****************************************************************************//**
*
* Summary:
*   Buffer is assigned with data to be sent to slave.
*   high level PDL library function is used to control I2C SCB to send data to
*   I2C slave. Errors are handled depend on the return value from the
*   appropriate function.
*
* Parameters:
*   writebuffer: Command packet buffer pointer
*   bufferSize: Size of the packet buffer
*
* Return:
*   Status after command is written to slave.
*   TRANSFER_ERROR is returned if any error occurs.
*   TRANSFER_CMPLT is returned if write is successful.
*
*******************************************************************************/
int WriteData(uint8_t[], int);

/*******************************************************************************
* Function Name: ReadData
****************************************************************************//**
*
* Summary:
*   Master initiates the read from the slave I2C buffer.
*   The status of the transfer is returned.
*
* Return:
*   Status of the transfer by checking packets read.
*   Note that if the status packet read is correct function returns TRANSFER_CMPLT
*   and if status packet is incorrect function returns TRANSFER_ERROR.
*
*******************************************************************************/
int ReadData(uint8_t[], int);

/*******************************************************************************
* Function Name: OpenConnection
********************************************************************************
*
* Summary:
*   This function initiates and enables master I2C
*
* Return:
*   Status of initialization
*
*******************************************************************************/
int OpenConnection(void);

/*******************************************************************************
* Function Name: CloseConnection
********************************************************************************
*
* Summary:
*   This function de-initializes and disables master I2C
*
* Return:
*   Status of I2C_SUCCESS
*
*******************************************************************************/
int CloseConnection(void);

#endif /* SOURCE_I2CMASTER_H_ */
