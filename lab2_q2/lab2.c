#include "driver/gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdio.h"
#include "stdlib.h"

int gpio = 3; // set the gpio pin
const int task1_priority = 3;
const int task2_priority = 2;
const int task3_priority = 1;

SemaphoreHandle_t xSemaphore = NULL;// we create the mutex handler here
//StaticSemaphore_t xMutexBuffer;

SemaphoreHandle_t xMutex = NULL;// we create the mutex handler here

SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer );

// we have to use xSemaphoreTake(), and xSemaphoreGive() to give and take the mutex

void vATask( void * pvParameters )
{
    /* Create the semaphore to guard a shared resource. As we are using
       the semaphore for mutual exclusion we create a mutex semaphore
       rather than a binary semaphore. */
    xSemaphore = xSemaphoreCreateMutex();
}


void Task1(void *pvParameters)// TASK 1
{
	xSemaphore = xSemaphoreCreateMutex();
	int x=0;
	//xSemaphore = x
	for(;;)
	{
		const TickType_t half_sec = pdMS_TO_TICKS(500);
		if(xSemaphore != NULL)// sees if we can access the mutex
		{
				if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )// 10 tells us the amount of time to wait before we can get the mutex
			  {	
			  
				  	TickType_t start = xTaskGetTickCount();
					printf("Task 1 is running");
					gpio_set_level(gpio,1);// this sets the output as high on the pin
					//set level high
					// he used a while loop for the active delay
					printf("x : %d",x);	
					while(xTaskGetTickCount() - (start) < half_sec)
					{
						// checks if the active delay is less than five seconds
					}	
		            /* We were able to obtain the semaphore and can now access the
		               shared resource. */
		
		            /* ... */
		
		            /* We have finished accessing the shared resource. Release the
		               semaphore. */
					   x++;
	            	xSemaphoreGive( xSemaphore );
	        }
		}
		
	}
	
	vTaskDelete(NULL);
}

void Task2(void *pvParameters)//TASK 2
{
	for(;;)
	{
		if(xSemaphore != NULL )
		{
		
			if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
			{
				/* We were able to obtain the semaphore and can now access the
				shared resource. */

				/* ... */

				/* We have finished accessing the shared resource. Release the
				semaphore. */
				printf("Task 2 is running");
				gpio_set_level(gpio,0);// this sets the output as low on the pin
				// we give after 
				vTaskDelay(pdMS_TO_TICKS(1000));//i second
				xSemaphoreGive( xSemaphore );
			}
			else
			{
				/* We could not obtain the semaphore and can therefore not access
				the shared resource safely. */
			}
		}
		
		
	}
	
	vTaskDelete(NULL);
}

void Task3(void *pvParameters)//TASK 3
{
	for(;;)
	{
		printf("Task 3 is running");
		if(gpio_get_level(gpio))
		{
			printf("The led is on");
		}
		else{
			printf("The led is off");
		}
		
		vTaskDelay(pdMS_TO_TICKS(1000));// delay for 1 second
	}
	vTaskDelete(NULL);
}

void app_main()
{
	xMutex = xSemaphoreCreateMutex();
	//xSemaphore = xSemaphoreCreateMutex();

	//checks if the semephore was created succesfully before creating the tasks
	//if(xMutex != NULL)
	//{
		xTaskCreate(Task1,
					"Task 1",
					1000,
					NULL,
					1,
					NULL);
		
		xTaskCreate(Task2,
					"Task 2",
					1000,
					NULL,
					2,
					NULL);
					
		xTaskCreate(Task3,
					"Task 3",
					1000,
					NULL,
					3,
					NULL);
		
		vTaskStartScheduler();
	//}
	
	/*
		if all is well main() will not reach here because the scheduler will
		now be running the created tasks. If main() does reach here then thre was
		not enough memory to create the idle or timer tasks
	*/
	for(;;);
	
}
