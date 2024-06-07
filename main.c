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
#include <time.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"



#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------

/* Timers */
static TimerHandle_t xTimer1 = NULL;
static TimerHandle_t xTimer2 = NULL;
static TimerHandle_t xTimer3 = NULL;
static TimerHandle_t xTimer4 = NULL;

TaskHandle_t TX1_Handle ;
TaskHandle_t TX2_Handle ;
TaskHandle_t TX3_Handle ;
TaskHandle_t RX_Handle ;


BaseType_t xTimer1Started, xTimer2Started ,
           xTimer3Started, xTimer4Started ;

/* Semaphores */
SemaphoreHandle_t Service_TX1 ;
SemaphoreHandle_t Service_TX2 ;
SemaphoreHandle_t Service_TX3 ;
SemaphoreHandle_t Service_RX ;



/* functions callback for timers */
static void prvAutoReloadTimer1Callback( TimerHandle_t xTimer );
static void prvAutoReloadTimer2Callback( TimerHandle_t xTimer );
static void prvAutoReloadTimer3Callback( TimerHandle_t xTimer );
static void prvAutoReloadTimer4Callback( TimerHandle_t xTimer );



/* Global Queue */
QueueHandle_t test_Q ;

/* functions */
int Random ( int paramter1 , int parameter2);
void RsestSystem1 (void);
void RsestSystem2 (void);


/* tasks */
void Sender1( void *parameters );
void Sender2( void *parameters );
void Sender3( void *parameters );
void Receiver( void *parameters );



/* variables */
int     TX1_b_messages , TX1_t_messages ,
        TX2_b_messages , TX2_t_messages ,
		TX3_b_messages , TX3_t_messages ,
		rece_message ;

int start , end ;                          //borders of timer period

char lengthOfQueue[2] = {3,10} ;           //to determine the size of global queue
char Flag = 1 ;                            //to make size 2 of queue to equal 10
int i = 0 ;                                // index of border periods in array

/* to find new period in new iterations */
int Period_T1 , Period_T2 ,
			Period_T3 ;

/* to sum each period and divide it on total iterations to get the average period */
int Total_Period_T1 , Total_Period_T2 ,
			Total_Period_T3 ;
int iter1 = 1 ,
			iter2 = 1 , iter3 = 1 ;


/* define */
#define MAX_R_MESSAGES     1000   /* I ask you to try code with 100 max receive to be sure that code is true
                                      because it take many many many time with 1000 received messages */
#define FIXED_TRECIEVE     100    // fixed period of timer receiver



// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"





int main(int argc, char* argv[])
{
	/* create Queue */
	test_Q = xQueueCreate(lengthOfQueue[Flag] , 8*sizeof( char * ) );

	/* create semaphores */
	Service_TX1 = xSemaphoreCreateBinary();
	Service_TX2 = xSemaphoreCreateBinary();
	Service_TX3 = xSemaphoreCreateBinary();
	Service_RX  = xSemaphoreCreateBinary();



	/* create tasks  */
	xTaskCreate(Sender1 , "TX1" , 2000 , NULL , 1 ,  NULL ); /* duplicated four times to creat 4 tasks  */
	xTaskCreate(Sender2 , "TX2" , 2000 , NULL , 1 ,  NULL );
	xTaskCreate(Sender3 , "TX3" , 2000 , NULL , 2 ,  NULL );
	xTaskCreate(Receiver , "RX" , 2000 , NULL , 3 ,  NULL );


	/*at first in main we call reset function to make step 3,4,5 in system as required in pdf  */
	RsestSystem2();


	if( test_Q != NULL )
	{
		/* to get period of each timer in first iteration  */
		Period_T1 = Random(start,end) ;
		Period_T2 = Random(start,end) ;
		Period_T3 = Random(start,end) ;

		Total_Period_T1 = Period_T1 ;
		Total_Period_T2 = Period_T2 ;
		Total_Period_T3 = Period_T3 ;

		/* create 3 timers for senders with random periods and one with fixed period for receiver */
		xTimer1 = xTimerCreate("TX1" , ( pdMS_TO_TICKS(Period_T1) ), pdTRUE, ( void * ) 0, prvAutoReloadTimer1Callback) ;
		xTimer2 = xTimerCreate("TX2" , ( pdMS_TO_TICKS(Period_T2) ), pdTRUE, ( void * ) 0, prvAutoReloadTimer2Callback) ;
		xTimer3 = xTimerCreate("TX3" , ( pdMS_TO_TICKS(Period_T3) ), pdTRUE, ( void * ) 0, prvAutoReloadTimer3Callback) ;
		xTimer4 = xTimerCreate("RX"  , ( pdMS_TO_TICKS(FIXED_TRECIEVE) )   , pdTRUE, ( void * ) 0, prvAutoReloadTimer4Callback) ;



		if( ( xTimer1 != NULL ) && ( xTimer2 != NULL )
				&& ( xTimer3 != NULL ) && ( xTimer4 != NULL ))
		{
			xTimer1Started = xTimerStart( xTimer1, 0 );
			xTimer2Started = xTimerStart( xTimer2, 0 );
			xTimer3Started = xTimerStart( xTimer3, 0 );
			xTimer4Started = xTimerStart( xTimer4, 0 );

		}//to start timing

		if( xTimer1Started == pdPASS && xTimer2Started == pdPASS
				&& xTimer3Started == pdPASS && xTimer4Started == pdPASS )
		{
			/* then start scheduling */
			vTaskStartScheduler();
		}//to scheduling

	}// to check that Queue created true

	return 0;
}




#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------



int Random (int paramter1 , int parameter2){
	return (  (rand()%(parameter2-paramter1+1)  ) + paramter1  );
}


/* in each call back function for timers to senders we give semaphores only */
static void prvAutoReloadTimer1Callback( TimerHandle_t xTimer )
{
	xSemaphoreGive(Service_TX1);
}
static void prvAutoReloadTimer2Callback( TimerHandle_t xTimer )
{
	xSemaphoreGive(Service_TX2);
}
static void prvAutoReloadTimer3Callback( TimerHandle_t xTimer )
{
	xSemaphoreGive(Service_TX3);
}


/* in receiver call back we give semaphore and
 * ask if rece_message == MAX_R_MESSAGES to make reset function as required in pdf  */
static void prvAutoReloadTimer4Callback( TimerHandle_t xTimer )
{
	xSemaphoreGive(Service_RX);

	if (rece_message == MAX_R_MESSAGES )
	{
		RsestSystem1();
		RsestSystem2();
	}//
}



/* in each sender task we take semaphore , enqueue the strings , increment transmitted or blocked  */
void Sender1( void *parameters )
{
	while (1)
	{
		xSemaphoreTake(Service_TX1,(TickType_t)portMAX_DELAY);

		/* to get the next period of timer */
		Period_T1 = Random(start,end) ;

		/* to set the new random period */
		xTimerChangePeriod(xTimer1,pdMS_TO_TICKS(Period_T1), 0 );

		Total_Period_T1 = Total_Period_T1 + Period_T1 ;
		iter1++;

		/* to set the string message " Time is XYZ "  */
		char massage[40] ;
		TickType_t lValueToSend = xTaskGetTickCount() ;
		snprintf(massage, sizeof(massage) ,"Time is %lu",lValueToSend);
		BaseType_t Status;
		Status = xQueueSend( test_Q , (void *) & massage , 0 );

		/* to check that the send message is true remove the commented dash '/' in next  */
		//printf("%s   \n", massage );

		/* to check that there's space in Queue */
		if ( Status != pdPASS )
		{ //failed
			TX1_b_messages++;
		}
		else
		{
			TX1_t_messages++;
		}//

	}//end while
}
void Sender2( void *parameters )
{
	while (1)
	{
		xSemaphoreTake(Service_TX2,(TickType_t)portMAX_DELAY);

		/* to get the next period of timer */
		Period_T2 = Random(start,end) ;

		/* to set the new random period */
		xTimerChangePeriod(xTimer2,pdMS_TO_TICKS(Period_T2), 0 );

		Total_Period_T2 = Total_Period_T2 + Period_T2 ;
		iter2++;

		/* to set the string message " Time is XYZ "  */
		char massage[40] ;
		TickType_t lValueToSend = xTaskGetTickCount() ;
		snprintf(massage, sizeof(massage) ,"Time is %lu",lValueToSend);
		BaseType_t Status;
		Status = xQueueSend( test_Q , (void *) & massage , 0 );

		/* to check that the send message is true remove the commented dash '/' in next  */
		//printf("%s   \n", massage );

		/* to check that there's space in Queue */
		if ( Status != pdPASS )
		{ //failed
			TX2_b_messages++;
		}
		else
		{
			TX2_t_messages++;
		}//

	}
}
void Sender3( void *parameters )
{
	while (1)
	{
		xSemaphoreTake(Service_TX3,(TickType_t)portMAX_DELAY);

		/* to get the next period of timer */
		Period_T3 = Random(start,end) ;

		/* to set the new random period */
		xTimerChangePeriod(xTimer3,pdMS_TO_TICKS(Period_T3), 0 );

		Total_Period_T3 = Total_Period_T3 + Period_T3 ;
		iter3++;

		/* to set the string message " Time is XYZ "  */
		char massage[40] ;
		TickType_t lValueToSend = xTaskGetTickCount() ;
		snprintf(massage, sizeof(massage) ,"Time is %lu",lValueToSend);
		BaseType_t Status;
		Status = xQueueSend( test_Q , (void *) & massage , 0 );

		/* to check that the send message is true remove the commented dash '/' in next  */
		//printf("%s   \n", massage );

		/* to check that there's space in Queue */
		if ( Status != pdPASS )
		{ //failed
			TX3_b_messages++;
		}
		else
		{
			TX3_t_messages++;
		}//

	}
}

/* to dequeue and increment received messages */
void Receiver( void *parameters )
{
	while (1)
	{
		xSemaphoreTake(Service_RX,(TickType_t)portMAX_DELAY);

		/* to get the received message */
		char receivedValue[40];
		BaseType_t Status;
		Status = xQueueReceive( test_Q , & receivedValue , 0 );

		if ( Status == pdPASS )
		{
			rece_message++ ;
			/* to check that the send message is true remove the commented dash '/' in next  */
			//printf("%s   \n",receivedValue);
		}//
	}
}



/* then used reset function */
void RsestSystem1 (void)
{
	/*
	 1- Print the total number of successfully sent messages and the total number of blocked messages
	 */
	printf("Total successfully sent messages is %d\n", TX1_t_messages
			+TX2_t_messages+TX3_t_messages);
	printf("Total blocked messages is %d\n\n", TX1_b_messages+
			TX2_b_messages+TX3_b_messages);

	/*
	 2- Print the statistics per sender task (the high priority and the two lower priority tasks).
	 * */
	printf("- statistics for each task : \n");
	printf("average period of sender 1 is %d\n", Total_Period_T1 / iter1 );
	printf("successfully sent messages of sender 1 is %d\n", TX1_t_messages );
	printf("Blocked messages of sender 1 is %d\n\n", TX1_b_messages);

	printf("average period of sender 2 is %d\n", Total_Period_T2 / iter2);
	printf("successfully sent messages of sender 2 is %d\n", TX2_t_messages );
	printf("Blocked messages of sender 2 is %d\n\n", TX2_b_messages);

	printf("average period of sender 3 is %d\n", Total_Period_T3 / iter3);
	printf("successfully sent messages of sender 3 is %d\n", TX3_t_messages );
	printf("Blocked messages of sender 3 is %d\n", TX3_b_messages);


	printf("---------------------------------------------------------\n");
}

void RsestSystem2 (void)
{
	/*
	 3- Reset t h e total number of successfully sent messages, the total number of blocked messages
         and received messages
	 * */
	TX1_t_messages = 0 ;
	TX2_t_messages = 0 ;
	TX3_t_messages = 0 ;

	TX1_b_messages = 0 ;
	TX2_b_messages = 0 ;
	TX3_b_messages = 0 ;

	rece_message = 0 ;

	/* to get average period of each timer  */
	Total_Period_T1 = 0 ;
	Total_Period_T2 = 0 ;
	Total_Period_T3 = 0 ;
	iter1           = 0 ;
	iter2           = 0 ;
	iter3           = 0 ;


	/*
	 4- Clears the queue
	  */
	xQueueReset ( test_Q );

	/*
	 5- Configure the values controlling the sender timer period Tsender to the next values in two arrays
	specifying the lower and upper bound values of the uniformly distributed timer period. The first array
	holds the values {50, 80, 110, 140, 170, 200} and the second holds the values {150, 200, 250, 300,
	350, 400} expressing in msec the timer lower and upper bounds for a uniform distribution. When the
	system starts initially it starts with the values 50 and 150. If all values in the array are used, destroy
	the timers and print a message “Game Over”and stop execution
	 * */
	int arr_border_start[6] = { 50  , 80  , 110 , 140 , 170 , 200 };
	int arr_border_end[6]   = { 150 , 200 , 250 , 300 , 350 , 400 };

    if (i  == 6 )
	{
		printf("  GAME OVER \n");

		/* destroy timers */
		xTimerDelete(TX1_Handle, 0 );
		xTimerDelete(TX2_Handle, 0 );
		xTimerDelete(TX3_Handle, 0 );
		xTimerDelete(RX_Handle , 0 );

		/*  stop execution  */
		vTaskSuspendAll();
	}//

    /* to determine the borders of random period */
	start = arr_border_start[i] ;
	end = arr_border_end [i]    ;

	i++;

}

/*-----------------------------------------------------------*/


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
