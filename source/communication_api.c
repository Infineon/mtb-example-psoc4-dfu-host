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

/* Header file includes */
#include "communication_api.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* I2C master interrupt macros */
#define I2C_INTR_NUM        Host_I2C_IRQ
#define I2C_INTR_PRIORITY   (3UL)

/* Command valid status */
#define TRANSFER_ERROR      (0xFFUL)
#define READ_ERROR          (TRANSFER_ERROR)

/* Timeout */
#define LOOP_FOREVER        (0UL)

/* Combine master error statuses in single mask  */
#define MASTER_ERROR_MASK   (CY_SCB_I2C_MASTER_DATA_NAK | CY_SCB_I2C_MASTER_ADDR_NAK   | \
                            CY_SCB_I2C_MASTER_ARB_LOST | CY_SCB_I2C_MASTER_ABORT_START | \
                            CY_SCB_I2C_MASTER_BUS_ERR)

/* I2C slave address to communicate with */
uint8_t I2C_SLAVE_ADDR;

/*******************************************************************************
* Global variables
*******************************************************************************/
/* Structure for master transfer configuration */
cy_stc_scb_i2c_master_xfer_config_t masterTransferCfg =
{
    .slaveAddress = 0U,
    .buffer       = NULL,
    .bufferSize   = 0U,
    .xferPending  = false
};

/** The instance-specific context structure.
 * It is used by the driver for internal configuration and
 * data keeping for the I2C. Do not modify anything in this structure.
 */
cy_stc_scb_i2c_context_t Host_I2C_context;

/*******************************************************************************
* Function Declaration
*******************************************************************************/
void CYBSP_I2C_Interrupt(void);
/*******************************************************************************
* Function Name: CYBSP_I2C_Interrupt
****************************************************************************//**
*
* Summary:
*   Invokes the Cy_SCB_I2C_MasterInterrupt() PDL driver function.
*
*******************************************************************************/
void CYBSP_I2C_Interrupt(void)
{
    Cy_SCB_I2C_MasterInterrupt(Host_I2C_HW, &Host_I2C_context);
}


int WriteData(uint8_t writebuffer[], int byteCnt)
{
    uint8_t status = TRANSFER_ERROR;
    cy_en_scb_i2c_status_t  errorStatus;
    uint32_t masterStatus;
    /* Timeout 1 sec (one unit is us) */
    uint32_t timeout = 1000000UL;

    /* Setup transfer specific parameters */
    masterTransferCfg.buffer     = writebuffer;
    masterTransferCfg.bufferSize = byteCnt;
    masterTransferCfg.slaveAddress = I2C_SLAVE_ADDR;

    /* Initiate write transaction */
    errorStatus = Cy_SCB_I2C_MasterWrite(Host_I2C_HW, &masterTransferCfg, &Host_I2C_context);
    if(errorStatus == CY_SCB_I2C_SUCCESS)
    {
        /* Wait until master complete read transfer or time out has occured */
        do
        {
            masterStatus  = Cy_SCB_I2C_MasterGetStatus(Host_I2C_HW, &Host_I2C_context);
            Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
            timeout--;

        } while ((0UL != (masterStatus & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));

        if (timeout <= 0)
        {
            /* Timeout recovery */
            Cy_SCB_I2C_Disable(Host_I2C_HW, &Host_I2C_context);
            Cy_SCB_I2C_Enable(Host_I2C_HW, &Host_I2C_context);
        }
        else
        {
            if ((0u == (MASTER_ERROR_MASK & masterStatus)) &&
                (byteCnt == Cy_SCB_I2C_MasterGetTransferCount(Host_I2C_HW, &Host_I2C_context)))
            {
                status = TRANSFER_CMPLT;
            }
        }
    }

    return (status);
}

int ReadData(uint8_t readBuffer[], int byteCnt)
{
    uint8_t status = TRANSFER_ERROR;
    cy_en_scb_i2c_status_t errorStatus;
    uint32_t masterStatus;
    /* Timeout 1 sec (one unit is us) */
    uint32_t timeout = 1000000UL;

    /* Setup transfer specific parameters */
    masterTransferCfg.buffer     = readBuffer;
    masterTransferCfg.bufferSize = byteCnt;
    masterTransferCfg.slaveAddress = I2C_SLAVE_ADDR;

    /* Initiate read transaction */
    errorStatus = Cy_SCB_I2C_MasterRead(Host_I2C_HW, &masterTransferCfg, &Host_I2C_context);
    if(errorStatus == CY_SCB_I2C_SUCCESS)
    {
        /* Wait until master complete read transfer or time out has occurred */
        do
        {
            masterStatus  = Cy_SCB_I2C_MasterGetStatus(Host_I2C_HW, &Host_I2C_context);
            Cy_SysLib_DelayUs(CY_SCB_WAIT_1_UNIT);
            timeout--;

        } while ((0UL != (masterStatus & CY_SCB_I2C_MASTER_BUSY)) && (timeout > 0));

        if (timeout <= 0)
        {
            /* Timeout recovery */
            Cy_SCB_I2C_Disable(Host_I2C_HW, &Host_I2C_context);
            Cy_SCB_I2C_Enable(Host_I2C_HW, &Host_I2C_context);
        }
        else
        {
            /* Check transfer status */
            if (0u == (MASTER_ERROR_MASK & masterStatus))
            {
                    status = TRANSFER_CMPLT;
            }
        }
    }
    return (status);

}


int OpenConnection(void)
{
    cy_en_scb_i2c_status_t initStatus;
    cy_en_sysint_status_t sysStatus;
    cy_stc_sysint_t CYBSP_I2C_SCB_IRQ_cfg =
    {
            /*.intrSrc =*/ Host_I2C_IRQ,
            /*.intrPriority =*/ I2C_INTR_PRIORITY
    };

    /*Initialize and enable the I2C in master mode*/
    initStatus = Cy_SCB_I2C_Init(Host_I2C_HW, &Host_I2C_config, &Host_I2C_context);
    if(initStatus != CY_SCB_I2C_SUCCESS)
    {
        return I2C_FAILURE;
    }

    /* Hook interrupt service routine */
    sysStatus = Cy_SysInt_Init(&CYBSP_I2C_SCB_IRQ_cfg, &CYBSP_I2C_Interrupt);
    if(sysStatus != CY_SYSINT_SUCCESS)
    {
        return I2C_FAILURE;
    }
    NVIC_EnableIRQ((IRQn_Type) CYBSP_I2C_SCB_IRQ_cfg.intrSrc);
    Cy_SCB_I2C_Enable(Host_I2C_HW, &Host_I2C_context);
    return I2C_SUCCESS;
}


int CloseConnection(void)
{
    Cy_SCB_I2C_Disable(Host_I2C_HW, &Host_I2C_context);
    Cy_SCB_I2C_DeInit(Host_I2C_HW);

    return I2C_SUCCESS;
}
