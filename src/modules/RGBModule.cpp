#include "RGBModule.h"
#include "Mylog.h"
#include <Arduino.h>
RGBModule::RGBModule(int _rpin, int _gpin, int _bpin)
{
	rpin = _rpin;
	gpin = _gpin;
	bpin = _bpin;
}
void RGBModule::setBlinkingStatus(LED_STATUS_COLOR s)
{
	BlinkingStatus = s;
}

void RGBModule::RGBInit()
{
	MyLogF(INFO,"INITIALIZING RGB Module : RED %d , GREEN %d , BLUE : %d",rpin,gpin,bpin);
	pinMode(rpin, OUTPUT);
	pinMode(bpin, OUTPUT);
	pinMode(gpin, OUTPUT);
}
void RGBModule::setColor(int redValue, int greenValue, int blueValue)
{
	analogWrite(rpin, redValue);
	analogWrite(gpin, greenValue);
	analogWrite(bpin, blueValue);
}

void RGBModule::toggleLEDTask(void *parameter)
{
	// Set up the RGB pins as outputs
	MyLog(INFO, "TOOGLE TASK Started ...");
	RGBModule* self = static_cast<RGBModule *>(parameter); 
	while (true)
	{
		// Turn on the LED based on the currentColor variable
		switch (self->BlinkingStatus)
		{
			
			MyLogF(DEBUG, "Current RGB Color status %d ", self->BlinkingStatus);

		case 0: // green
			self->setColor(0, 255, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break;
		case 1:
			self->setColor(0, 0, 255);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break; // Blue
		case 2:
			self->setColor(255, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break; // Red
		case 3:
			self->setColor(0, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break; // No color
		case 4:
			self->setColor(255, 255, 0); // Yellow
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0); // Off
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break;

		case 5:
			self->setColor(173, 216, 230); // Light Blue
			vTaskDelay(500 / portTICK_PERIOD_MS);
			self->setColor(0, 0, 0); // Off
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break;
		default:
			break;
		}

		// vTaskDelay(500 / portTICK_PERIOD_MS); // Delay for 500ms
	}
}
void RGBModule::runRGBTask()
{
  	MyLog(DEBUG,"Initialized RGB TASK ..."); 
	if (RGBTaskHandle == NULL)
	{
		xTaskCreate(toggleLEDTask,	   // Function to run as a thread
					"Toggle LED Task", // Name of the task
					1024,			   // Stack size in words
					this,			   // Task input parameter
					3,				   // Task priority
					&RGBTaskHandle	   // Task handle (optional)
		);
	}
}