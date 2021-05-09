#include "mbed.h"
#include "mbed_rpc.h"
#include "uLCD_4DGL.h"
#include "accelerometer_handler.h"

#include "config.h"
#include "magic_wand_model_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

RpcDigitalOut myled1(LED1,"myled1");
DigitalOut checkled1(LED1);
RpcDigitalOut myled2(LED2,"myled2");
DigitalOut checkled2(LED2);
BufferedSerial pc(USBTX, USBRX);
void LEDControl(Arguments *in, Reply *out);
RPCFunction rpcLED(&LEDControl, "mode");
double x, y;
InterruptIn button(USER_BUTTON);
uLCD_4DGL uLCD(D1, D0, D2);
int angle = 80;

EventQueue gesture_queue(32 * EVENTS_EVENT_SIZE);
EventQueue detection_queue(32 * EVENTS_EVENT_SIZE);

Thread gesture_thread(osPriorityLow);
Thread detection_thread(osPriorityLow);

void gesture_mode();
void detection_mode();
void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client);

int main() {
    uLCD.cls();
    uLCD.text_width(3); //3X size text
    uLCD.text_height(3);
    gesture_queue.call(gesture_mode);
    detection_queue.call(detection_mode);
    gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));
    detection_thread.start(callback(&detection_queue, &EventQueue::dispatch_forever));

    //The mbed RPC classes are now wrapped to create an RPC enabled version - see RpcClasses.h so don't add to base class


    // receive commands, and send back the responses

    char buf[256], outbuf[256];


    FILE *devin = fdopen(&pc, "r");

    FILE *devout = fdopen(&pc, "w");


    while(1) {

        memset(buf, 0, 256);

        for (int i = 0; ; i++) {

            char recv = fgetc(devin);

            if (recv == '\n') {

                printf("\r\n");

                break;

            }

            buf[i] = fputc(recv, devout);

        }

        //Call the static call method on the RPC class

        RPC::call(buf, outbuf);

        printf("%s\r\n", outbuf);

        
    }

}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client)
{
    // publish selected angle
    // led1 = 0
}

int an[4] = {20, 40, 60, 80};
int state = 3;
void gesture_mode()
{
    while (1) {
        if (checkled1) {
            // detect gesture

            switch (state) {
                case 0: if (gesture_index == 1) {
                    angle = an[++state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                }
                case 1: if (gesture_index == 1) {
                    angle = an[++state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                } else if (gesture_index == 0) {
                    angle = an[--state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                }
                case 2: if (gesture_index == 1) {
                    angle = an[++state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                } else if (gesture_index == 0) {
                    angle = an[--state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                }
                case 3: if (gesture_index == 0) {
                    angle = an[--state];
                    uLCD.cls();
                    uLCD.locate(0,0);
                    uLCD.printf("%d degree", angle);
                    gesture_index = -1;
                }
            } // end of switch

            // user bottom
            button.rise(&publish_message);
        }
    }
}

void detection_mode()
{
    while (1) {
        if (checkled2) {
            
            // if (n > 5) led2 = 0;
        }
    }
}

// Make sure the method takes in Arguments and Reply objects.
void LEDControl (Arguments *in, Reply *out)   {

    bool success = true;

    // In this scenario, when using RPC delimit the two arguments with a space.
    x = in->getArg<double>();
    y = in->getArg<double>();

    // Have code here to call another RPC function to wake up specific led or close it.

    char buffer[200], outbuf[256];

    char strings[20];

    int led = x;

    int on = y;

    sprintf(strings, "/myled%d/write %d", led, on);

    strcpy(buffer, strings);

    RPC::call(buffer, outbuf);

    if (success) {

        out->putData(buffer);

    } else {

        out->putData("Failed to execute LED control.");

    }

}