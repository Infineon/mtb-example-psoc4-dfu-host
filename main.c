/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the dfu host
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "string.h"
#include "stdio.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include "communication_api.h"
#include "cybtldr_api.h"
#include "cybtldr_command.h"
#include "cybtldr_parse.h"
#include "cybtldr_utils.h"
#include "uart_interface.h"
#include "string_Image.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define BTLDR_I2C_ADDRESS 0x0CU            // Bootloader's I2C address
#define PRODUCT_ID 0x01020304U            // default product ID
#define MAX_TRANSFER_BUFFER_SIZE 64U    // max transfer data on each I2C transaction
#define TRUE 1U
#define FALSE 0U
#define DEBOUNCE_DELAY 50                // debounce delay to debounce the button press
#define TIME_ELAPSED_MAX 20                // count till this macro for a long press detection
#define RETRY_DELAY 1000U                // delay before re attempting the process
#define LINE_START 0U
/*
 * This enum defines the different operations that can be performed
 * by the bootloader host.
 */
typedef enum {
    /* Perform a Program operation*/
    PROGRAM,
    /* Perform an Erase operation */
    ERASE,
    /* Perform a Verify operation */
    VERIFY,
} CyBtldr_Action;


/*******************************************************************************
* Global variables
*******************************************************************************/
/* Allocate context for UART operation */
cy_stc_scb_uart_context_t uartContext;

/* This structure contains function pointers to the four communication layer functions
   contained in the communication_api.c / .h */
CyBtldr_CommunicationsData comm1;

/* This structure contains the pointer to the String_Image present in the flash
 * which is used in cybtldr_parse.c / .h */
Firmware_Image_t image1 = {
    .imageBuffer = stringImage,        //present in string_Image.h
    .line_count = LINE_CNT            //present in string_Image.h
};

CyBtldr_Action action_t;    //to hold the action to be performed

/*
 * I2C slave address to communicate with
 * It is used in communication_api.c
 */
extern uint8_t I2C_SLAVE_ADDR;

/*******************************************************************************
* Function Declaration
*******************************************************************************/
int CyBtldr_RunAction(CyBtldr_Action, CyBtldr_CommunicationsData);
int ProcessDataRow(CyBtldr_Action, uint32_t, char*);

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the bootloder host main function.
* It initializes the Firmware Image, I2C address of the Bootloader and the communication structure.
* It checks for a button click and a long button press of 1s.
*     if the button press is short, like a click invoke a Program action and then verify the programmed data.
*     and if the button press is long, then invoke an erase action.
* It enables a uart block for printing statuses.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/

int main(void)
{

    int btldr_err;                // variable to store the error occurred after the bootloading operation
    uint32_t time_elapsed=0;    // counter counting for how long the button was pressed
    uint8_t button_pressed = 0;    // flag to store whether the button was pressed or not
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Configure UART to operate */
    (void) Cy_SCB_UART_Init(UART_HW, &UART_config, &uartContext);
    /* Enable UART */
    Cy_SCB_UART_Enable(UART_HW);

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize the firmware_image structure element*/
    initFirmwareImage(image1);

    /*initialize the i2c slave address to communicate with the bootloader
     * I2C_SLAVE_ADDR is used in communication_api.c
     */
    I2C_SLAVE_ADDR = BTLDR_I2C_ADDRESS;

    /* Initialize the communication structure element -comm1 */
    comm1.OpenConnection = &OpenConnection;
    comm1.CloseConnection = &CloseConnection;
    comm1.ReadData = &ReadData;
    comm1.WriteData = &WriteData;
    comm1.MaxTransferSize = MAX_TRANSFER_BUFFER_SIZE;

    for (;;)
    {
        /* look for a button press, whether it is a single click or a long press
         * We are using a click to demonstrate Program and a 1s long press for Erase
         */
        while(!Cy_GPIO_Read(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN)){
            action_t = PROGRAM;        //initialize the action to be performed with PROGRAM
            button_pressed = TRUE;

            Cy_SysLib_Delay(DEBOUNCE_DELAY);    //debounce
            time_elapsed++;

            if(time_elapsed > TIME_ELAPSED_MAX){
                action_t = ERASE;    //change the action to be performed with ERASE if long button press
                time_elapsed = 0U;
                break;
            }
        }

        if(button_pressed){
            button_pressed = FALSE;        //reset the button_pressed flag to account next button presses

            switch (action_t)
            {
            case ERASE:        print("\n\rErase Started\n");
                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 0);    //Turn the LED ON to indicate execution (the LED is Active LOW)

                            btldr_err = CyBtldr_RunAction(action_t, comm1);    // Execute ERASE action
                            if(CYRET_SUCCESS != btldr_err)
                                print("\n\rError, ERASE Failed !");        //print to the UART
                            else
                                print("\n\rERASE Successful !");        //print to the UART

                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 1);    //Turn the LED OFF to indicate execution completed
                            break;

            case PROGRAM:    print("\n\rProgramming Started\n");
                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 0);    //Turn the LED ON to indicate execution (the LED is Active LOW)

                            btldr_err = CyBtldr_RunAction(action_t, comm1);    // Execute PROGRAM action

                            if(CYRET_SUCCESS != btldr_err)
                                print("\n\rError, Programming Failed !");        //print to the UART
                            else
                                print("\n\rProgram Successful !");                //print to the UART

                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 1);    //Turn the LED OFF to indicate execution completed

                            action_t = VERIFY;        //change the action to verify

                            print("\n\rVerification Started\n");    //print to the UART console

                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 0);    //Turn the LED ON to indicate execution (the LED is Active LOW)

                            btldr_err = CyBtldr_RunAction(action_t, comm1);    // Execute VERIFY action

                            if(CYRET_SUCCESS != btldr_err)
                                print("\n\rError, Verify Failed !");        //print to the UART
                            else
                                print("\n\rVerification Successful !");        //print to the UART

                            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN, 1);    //Turn the LED OFF to indicate execution completed

                            break;

            default:         break;
            }

            Cy_SysLib_Delay(RETRY_DELAY);    // wait a while before two consecutive actions
        }
    }
}

/*******************************************************************************
 * Function Name: CyBtldr_RunAction
 ********************************************************************************
 * Summary: Run the appropriate function for setting up the meta-data and then start
 * program, verify or erase operation on the flash.
 *
 *
 * Parameters:
 *   action      - The action to execute
 *   comm         - The communication structure
 *
 * Returns:
 *   CYRET_SUCCESS        - The action was executed  successfully
 *   error_code            - error occurred and operation aborted
 *
 *******************************************************************************/
int CyBtldr_RunAction(CyBtldr_Action action, CyBtldr_CommunicationsData comm) {

    uint32_t lineLen;
    char line[MAX_BUFFER_SIZE * 2];  // 2 hex characters per byte
    uint32_t err = 0;

    uint32_t blVer = 0;
    uint32_t siliconId = 0;
    uint8_t siliconRev = 0;
    uint8_t chksumtype = SUM_CHECKSUM;
    uint8_t appId = 0;
    uint32_t applicationStartAddr = 0xffffffff;
    uint32_t applicationSize = 0;
    uint32_t applicationDataLines = 0;
    uint32_t productId = PRODUCT_ID;

    /*read one row from the string array and store into the buffer line*/
    err = CyBtldr_ReadLine(&lineLen, line);

    /* Parse the first line(header) of cyacd2 file to extract siliconID, siliconRev and packetChkSumType, appID, productID */
    err = CyBtldr_ParseHeader(lineLen, line, &siliconId, &siliconRev, &chksumtype, &appId, &productId);

    if (CYRET_SUCCESS == err) {
        /* Set the packet checksum type for communicating with bootloader.*/
        CyBtldr_SetCheckSumType((CyBtldr_ChecksumType)chksumtype);

        /* Start Bootloader operation by opening the I2C communication interface and send an Enter DFU command */
        err = CyBtldr_StartBootloadOperation(&comm, siliconId, siliconRev, &blVer, productId);

        /* parse the meta-data i.e the application size and the  start address for the application*/
        if (err == CYRET_SUCCESS) err = CyBtldr_ParseAppStartAndSize(&applicationStartAddr, &applicationSize, &applicationDataLines, line);
        /* Set the meta-data */
        if (err == CYRET_SUCCESS) err = CyBtldr_SetApplicationMetaData(appId, applicationStartAddr, applicationSize);

    }

    while (err == CYRET_SUCCESS) {

        /*read one row from the string array and store into the buffer line*/
        err = CyBtldr_ReadLine(&lineLen, line);
        if (CYRET_SUCCESS == err) {

            /* start programming, erasing or verifying the each row */
            err = ProcessDataRow(action, lineLen, line);

            /*print the progress of the action*/
            print("\r");
            printInt((int)(((float)getLineCounter()/(float)LINE_CNT)*100.0));    //print the progress of the action in percentage
            print(" %");

        } else if (CYRET_ERR_EOF == err) {
            err = CYRET_SUCCESS;
            break;
        }
    }

    if (err == CYRET_SUCCESS && (VERIFY == action)) {
        /* verify the Application checksum upon finishing the row by row verification */
        err = CyBtldr_VerifyApplication(appId);
        /*end the Bootloader operation and jump to the valid application thereafter*/
        CyBtldr_EndBootloadOperation();
    }
    setLineCounter(LINE_START);        //reset the line counter to point at the beginning of the string array again

    return err;
}


/*******************************************************************************
 * Function Name: ProcessDataRow
 ********************************************************************************
 * Summary:    This function processes each row by parsing the appropriate fields from
 * the buffer and starts the I2C communication to program, verify or erase.
 *
 * Parameters:
 *   action      - The action to execute
 *   rowSize     - the size of each row
 *   rowData     - buffer contains the row data
 *
 * Returns:
 *   CYRET_SUCCESS        - The action was executed  successfully
 *   error_code            - error occurred and operation aborted
 *
 *******************************************************************************/
int ProcessDataRow(CyBtldr_Action action, uint32_t rowSize, char* rowData) {
    uint8_t buffer[MAX_BUFFER_SIZE];
    uint16_t bufSize;
    uint32_t address;
    uint8_t checksum;

    int err = CyBtldr_ParseRowData(rowSize, rowData, &address, buffer, &bufSize, &checksum);

    if (CYRET_SUCCESS == err) {

        switch (action) {
            case ERASE:
                err = CyBtldr_EraseRow(address);
                break;
            case PROGRAM:
                err = CyBtldr_ProgramRow(address, buffer, bufSize);
                break;
            case VERIFY:
                err = CyBtldr_VerifyRow(address, buffer, bufSize);
                break;
        }
    }
    return err;
}




/* [] END OF FILE */
