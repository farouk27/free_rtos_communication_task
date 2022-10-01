/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"


#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------

/*-----------------------------------------------------------*/
// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

//global variables
int total_number_of_blocked_messages = 0;
int total_number_of_trasmitted_messages = 0;
int total_number_of_recived_messages = 0;
int current_iteration = 0;
//char message_by_sender1[50];
//char message_by_sender2[50];
//char recived_message[50] ;
// init lower and upper
int lower = 50;
int upper = 150;

// Task handler
TaskHandle_t xTask1Handle = NULL ;
TaskHandle_t xTask2Handle = NULL ;
TaskHandle_t xTask3Handle = NULL ;
// semaphore handler
SemaphoreHandle_t xBinarySemaphore1;
SemaphoreHandle_t xBinarySemaphore2;
SemaphoreHandle_t xBinarySemaphore3;

// timers handler
static TimerHandle_t xSenderTimer1 = NULL;
static TimerHandle_t xSenderTimer2 = NULL;
static TimerHandle_t xReciverTimer = NULL;
// queue handler
QueueHandle_t xQueue;

// base type for timers
BaseType_t xTimer1Started, xTimer2Started, xTimer3Started;




// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

BaseType_t xKeyPressesStopApplication = pdTRUE;


#define sender1_TIMER_PERIOD		( pdMS_TO_TICKS( 50UL ) )
#define sender2_TIMER_PERIOD	( pdMS_TO_TICKS( 150UL ) )
#define reciver_TIMER_PERIOD	( pdMS_TO_TICKS( 100UL ) )


/* Declare a variable of type QueueHandle_t.  This is used to store the queue
that is accessed by all three tasks. */
void Reset(void);
void vSenderTask1(void *pvParameters);
void vSenderTask2(void *pvParameters);
void vReceiverTask3(void *pvParameters);

int randbetween(int, int);
// sender one and sender two realse different semaphores so these differnt functions
static void prvSender1TimerCallback(TimerHandle_t xTimer);
static void prvSender2TimerCallback(TimerHandle_t xTimer);
static void prvReciverTimerCallback(TimerHandle_t xTimer);



// these two variables are printed in the reset function
//total number of successfully sent messages
//total number of blocked messages
/*-----------------------------------------------------------*/


/* this part of code related to generating random period */


int main(int argc, char* argv[])
{

	xBinarySemaphore1 = xSemaphoreCreateBinary();
	xBinarySemaphore2 = xSemaphoreCreateBinary();
	xBinarySemaphore3 = xSemaphoreCreateBinary();


	xSenderTimer1 = xTimerCreate("sender1AutoReaload",
		sender1_TIMER_PERIOD,
		pdTRUE,
		0,
		prvSender1TimerCallback);
	xSenderTimer2 = xTimerCreate("sender2AutoReaload",
		sender2_TIMER_PERIOD,
		pdTRUE,
		0,
		prvSender2TimerCallback);

	xReciverTimer = xTimerCreate("ReciverAutoReaload",
		reciver_TIMER_PERIOD,
		pdTRUE,
		0,
		prvReciverTimerCallback);
	// check if all timers created successfully
	if ((xSenderTimer1 != NULL) && (xSenderTimer2 != NULL) && (xReciverTimer != NULL))
	{
		// start of the three timers
		xTimer1Started = xTimerStart(xSenderTimer1, 0);
		xTimer2Started = xTimerStart(xSenderTimer2, 0);
		xTimer3Started = xTimerStart(xReciverTimer, 0);

//		if ((xTimer1Started == pdPASS) && (xTimer2Started == pdPASS) && (xTimer3Started == pdPASS))
//		{
//			// Start the scheduler.
//			vPrintString("start timers.\r\n");
//		}
	}

	xQueue = xQueueCreate(20, sizeof(TickType_t));
	if (xQueue != NULL)
	{

		xTaskCreate(vSenderTask1, "Sender1", 1000, NULL, 1, &xTask1Handle);
		xTaskCreate(vSenderTask2, "Sender2", 1000, NULL, 1, &xTask2Handle);
		xTaskCreate(vReceiverTask3, "Receiver", 1000, NULL, 2, &xTask3Handle);
		vTaskStartScheduler();
	}

	for (;; );
	return 0;
}

void vSenderTask1(void *pvParameters)
{

	BaseType_t xStatus;
	for (;; )
	{
			// we need to send message on the queue (string not int )
			static TickType_t xTimeNow;
			xTimeNow = xTaskGetTickCount();
			xSemaphoreTake(xBinarySemaphore1, portMAX_DELAY);
			xStatus = xQueueSendToBack(xQueue, &xTimeNow, 0);
			if (xStatus != pdPASS)
			{
				total_number_of_blocked_messages = total_number_of_blocked_messages + 1;
//				printf("total_number_of_blocked_messages = %d \r\n", total_number_of_blocked_messages );
			}
			else {
				total_number_of_trasmitted_messages = total_number_of_trasmitted_messages + 1;
			}
	}
}
/*-----------------------------------------------------------*/

void vSenderTask2(void *pvParameters)
{
	BaseType_t xStatus;

	for (;; )
	{
			static TickType_t xTimeNow;
			xTimeNow = xTaskGetTickCount();
			xSemaphoreTake(xBinarySemaphore2, portMAX_DELAY);
			xStatus = xQueueSendToBack(xQueue, &xTimeNow, 0);
			if (xStatus != pdPASS)
			{
//				printf("coudldn't send to queue\r\n");
				total_number_of_blocked_messages = total_number_of_blocked_messages + 1;
//				printf("total_number_of_blocked_messages = %d \r\n", total_number_of_blocked_messages );

			}
			else {
				total_number_of_trasmitted_messages = total_number_of_trasmitted_messages + 1;
			}
	}
}

/*
 -if there are messages in the queue he reads it and  increments total number of
received messages and sleeps again
- if the recived messages exceed 500 message you need to call reset function
 */
 ///////////////
void vReceiverTask3(void *pvParameters)
{

	TickType_t lReceivedValue;
	BaseType_t xStatus;

	for (;; )
	{
			xSemaphoreTake( xBinarySemaphore3, portMAX_DELAY );
			xStatus = xQueueReceive(xQueue, &lReceivedValue, 0);
			if (xStatus == pdPASS)
			{
				// Data was successfully received from the queue, print out the received value.
//			   char str[80];
//			   sprintf(str, "Time is = %d\r\n", lReceivedValue);
//			   puts(str) ;
//				printf("Received= %d \r\n ",lReceivedValue  );
				total_number_of_recived_messages++ ;
//				printf("total_number_of_recived_messages %d \r\n",total_number_of_recived_messages  );
			}
//			else
//			{
//
//				printf("Could not receive from the queue.\r\n");
//			}
			if (total_number_of_recived_messages>=500){
				Reset();
			}

	}
}


void Reset(void)
{
	printf("total_number_of_trasmitted_messages %d \r\n",total_number_of_trasmitted_messages  );
	printf("total_number_of_recived_messages %d \r\n",total_number_of_recived_messages  );
	printf("total_number_of_blocked_messages %d \r\n",total_number_of_blocked_messages  );
	total_number_of_trasmitted_messages=0;
	total_number_of_recived_messages=0;
	total_number_of_blocked_messages=0 ;
	xQueueReset(xQueue);
	current_iteration = current_iteration + 1;
	printf("the current iteration %d \r\n",current_iteration  );

	if (current_iteration == 1) {
		lower = 80;
		upper = 200;
	}
	if (current_iteration == 2) {
		lower = 110;
		upper = 250;
	}
	if (current_iteration == 3) {
		lower = 140;
		upper = 300;
	}
	if (current_iteration == 4) {
		lower = 170;
		upper = 350;
	}
	if (current_iteration == 5) {
		lower = 200;
		upper = 400;
	}
	if (current_iteration == 6) {
		// this means that you used all values in the array destroy the timers and print message game over and stop execution
		printf("GAME OVER \r\n");
		vTaskDelete(xTask1Handle);
		vTaskDelete(xTask2Handle);
		vTaskDelete(xTask3Handle);
		xTimerDelete( xSenderTimer1,0 ) ;
		xTimerDelete( xSenderTimer2 ,0) ;
		xTimerDelete( xReciverTimer ,0) ;

	}
    printf("lower = %d, upper = %d\n", lower , upper );
}
/* the Implementaion of call back function */
// it need to relases semaphore make sender1 able to send message
static void prvSender1TimerCallback(TimerHandle_t xTimer)
{
//	static TickType_t xTimeNow;
//	xTimeNow = xTaskGetTickCount();
//	printf("timer1 callback executing %d\r\n ",xTimeNow  );
	xSemaphoreGive(xBinarySemaphore1);
	// every iteration we need to change the limits
	TickType_t new_Timer_period = randbetween(lower, upper);
	TickType_t new_Timer_period_in_Ticks = pdMS_TO_TICKS(new_Timer_period);
	xTimerChangePeriod(xTimer, new_Timer_period_in_Ticks, 0);

}
/*-----------------------------------------------------------*/

// relases semaphore make task2 send messages
static void prvSender2TimerCallback(TimerHandle_t xTimer)
{
	// you give semaphore and change period

	/* Obtain the current tick count. */
//	static TickType_t xTimeNow;
//	xTimeNow = xTaskGetTickCount();

	/* Output a string to show the time at which the callback was executed. */
//	printf("timer2 callback executing %d \r\n",xTimeNow  );
	xSemaphoreGive(xBinarySemaphore2);
	TickType_t new_Timer_period = randbetween(lower, upper);
	TickType_t new_Timer_period_in_Ticks = pdMS_TO_TICKS(new_Timer_period);
	xTimerChangePeriod(xTimer, new_Timer_period_in_Ticks, 0);

}

// here you will call reset if the messages exceed 500
static void prvReciverTimerCallback(TimerHandle_t xTimer)
{

	/* Obtain the current tick count. */
//	static TickType_t xTimeNow;
//	xTimeNow = xTaskGetTickCount();

	/* Output a string to show the time at which the callback was executed. */
//	printf("reciver callback executing %d \r\n",xTimeNow  );
	// check if the total recived message is 500 message you will call reset function
	xSemaphoreGive(xBinarySemaphore3);

}
int randbetween(int low, int high) {

	int num = (rand() % (low - high + 1)) + low;
	return num;
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

