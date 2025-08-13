#include "network_stack.h"
#include "Mylog.h"
extern bool Wake_PC_STATE;
extern bool Wake_PC_STATE_finished;

void NetworkStack::initTaskWol()
{
    if (handler == NULL)
    {
        xTaskCreate(runTaskWOL, // Function to run as a thread
                    "Wake PC ", // Name of the task
                    2048,       // Stack size in words
                    this,       // Task input parameter
                    1,          // Task priority
                    &handler    // Task handle (optional)
        );
    }
}
char macBuffer[255];

void NetworkStack::runTaskWOL(void *parameter)
{

    NetworkStack *self = static_cast<NetworkStack *>(parameter);
    MyLog(INFO, "WOL Task initialized .");
    snprintf(macBuffer, sizeof(macBuffer), "%02X:%02X:%02X:%02X:%02X:%02X", targetMAC[0], targetMAC[1], targetMAC[2], targetMAC[3], targetMAC[4], targetMAC[5]);
    // self->MacArrayToString(targetMAC, macBuffer, sizeof(macBuffer));
    MyLogF(INFO, "Target Mac addr %s", macBuffer);
    if (false == Wake_PC_STATE)
    {
        MyLog(INFO, "WOL Task  is Disabled, exiting ...");
    }

    while (true)
    {
        if (true == Wake_PC_STATE)
        {

            self->WakeOnLan(targetMAC, self->WoLRepetion);
            Wake_PC_STATE = false;
            Wake_PC_STATE_finished = true;
            MyLog(INFO, "WOL Task  Finished, exiting ...");
            vTaskDelete(NULL);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
