#include <iostream>
using namespace std;
#include <cmath>
#include "mbed.h"
using namespace std::chrono;
#include "mbed_rpc.h"
#include "uLCD_4DGL.h"

#include "accelerometer_handler.h"
#include "stm32l475e_iot01_accelero.h"
#include "tfconfig.h"
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
DigitalOut checkled3(LED3);
BufferedSerial pc(USBTX, USBRX);
void LEDControl(Arguments *in, Reply *out);
RPCFunction rpcLED(&LEDControl, "mode");
double x, y;
InterruptIn btn2(USER_BUTTON);
uLCD_4DGL uLCD(D1, D0, D2);
int angle = 60;
int an[4] = {20, 40, 60, 80};
int state = 2;
int uLCD_print = 0;
double detect_angle;
// mqtt
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;
const char* topic = "Mbed";
MQTT::Client<MQTTNetwork, Countdown>* global_client;
WiFiInterface *wifi;

EventQueue gesture_queue(32 * EVENTS_EVENT_SIZE);
EventQueue detection_queue(32 * EVENTS_EVENT_SIZE);
EventQueue mqtt_queue(32 * EVENTS_EVENT_SIZE);
EventQueue mqtt_main_queue(32 * EVENTS_EVENT_SIZE);

Thread gesture_thread;
Thread detection_thread;
Thread mqtt_thread(osPriorityHigh);
Thread mqtt_main_thread;

void gesture_mode();
void detection_mode();
void mqtt_main();

void messageArrived(MQTT::MessageData& md) {

    MQTT::Message &message = md.message;

    char msg[300];

    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);

    printf(msg);

    ThisThread::sleep_for(1000ms);

    char payload[300];

    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);

    printf(payload);

    ++arrivedcount;

}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {

    if (checkled1 == 1) {

        MQTT::Message message;

        char buff[100];

        sprintf(buff, "threshold angle is : %d", angle);

        message.qos = MQTT::QOS0;

        message.retained = false;

        message.dup = false;

        message.payload = (void*) buff;

        message.payloadlen = strlen(buff) + 1;

        int rc = client->publish(topic, message);


        printf("rc:  %d\r\n", rc);

        printf("Puslish message: %s\r\n", buff);

        //checkled1 = 0;
    }
    else if (checkled2 == 1) {
        message_num++;

        MQTT::Message message;

        char buff[100];

        sprintf(buff, "overtilt for %d times and the current angle is : %.1f", message_num, (float)detect_angle);
        
        message.qos = MQTT::QOS0;

        message.retained = false;

        message.dup = false;

        message.payload = (void*) buff;

        message.payloadlen = strlen(buff) + 1;

        int rc = client->publish(topic, message);


        printf("rc:  %d\r\n", rc);

        printf("Puslish message: %s\r\n", buff);
    }
    if (message_num >= 5) {
      message_num = 0;
      //checkled2 = 0;
    }

}

void close_mqtt() {

    closed = true;

}

// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Return the result of the last prediction

int PredictGesture(float* output) {

  // How many times the most recent gesture has been matched in a row

  static int continuous_count = 0;

  // The result of the last prediction

  static int last_predict = -1;


  // Find whichever output has a probability > 0.8 (they sum to 1)

  int this_predict = -1;

  for (int i = 0; i < label_num; i++) {

    if (output[i] > 0.8) this_predict = i;

  }


  // No gesture was detected above the threshold

  if (this_predict == -1) {

    continuous_count = 0;

    last_predict = label_num;

    return label_num;

  }


  if (last_predict == this_predict) {

    continuous_count += 1;

  } else {

    continuous_count = 0;

  }

  last_predict = this_predict;


  // If we haven't yet had enough consecutive matches for this gesture,

  // report a negative result

  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {

    return label_num;

  }

  // Otherwise, we've seen a positive result, so clear all our variables

  // and report it

  continuous_count = 0;

  last_predict = -1;


  return this_predict;

}

int main() {
  /*
  //////// start mqtt_main
    if (!wifi) {

            printf("ERROR: No WiFiInterface found.\r\n");

            return ;

    }



    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);

    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);

    if (ret != 0) {

            printf("\nConnection error: %d\r\n", ret);

            return ;

    }



    //NetworkInterface* net = wifi;

    //MQTTNetwork mqttNetwork(net);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);


    //TODO: revise host to your IP

    const char* host = "192.168.31.176";

    printf("Connecting to TCP network...\r\n");


    SocketAddress sockAddr;

    sockAddr.set_ip_address(host);

    sockAddr.set_port(1883);


    printf("address is %s/%d\r\n", (sockAddr.get_ip_address() ? sockAddr.get_ip_address() : "None"),  (sockAddr.get_port() ? sockAddr.get_port() : 0) ); //check setting


    int rc = mqttNetwork.connect(sockAddr);//(host, 1883);

    if (rc != 0) {

            printf("Connection error.");

            return ;

    }

    printf("Successfully connected!\r\n");


    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;

    data.clientID.cstring = "Mbed";


    if ((rc = client.connect(data)) != 0){

            printf("Fail to connect MQTT\r\n");

    }

    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){

            printf("Fail to subscribe\r\n");

    }


    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    btn2.rise(mqtt_queue.event(&publish_message, &client));
    

    //btn3.rise(&close_mqtt);


    int num = 0;

    while (num != 5) {

            client.yield(100);

            ++num;

    }


    while (1) {

            if (closed) break;

            client.yield(500);

            ThisThread::sleep_for(500ms);

    }


    printf("Ready to close MQTT Network......\n");


    if ((rc = client.unsubscribe(topic)) != 0) {

            printf("Failed: rc from unsubscribe was %d\n", rc);

    }

    if ((rc = client.disconnect()) != 0) {

    printf("Failed: rc from disconnect was %d\n", rc);

    }
    /////////// end of mqtt_main
    */
    BSP_ACCELERO_Init();
    gesture_queue.call(gesture_mode);
    detection_queue.call(detection_mode);
    mqtt_main_thread.start(callback(&mqtt_main_queue, &EventQueue::dispatch_forever));
    gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));
    detection_thread.start(callback(&detection_queue, &EventQueue::dispatch_forever));
    mqtt_main_queue.call(mqtt_main);

    // The mbed RPC classes are now wrapped to create an RPC enabled version - see RpcClasses.h so don't add to base class


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

        /*if (uLCD_print == 1) {
            uLCD.background_color(BLACK);
            uLCD.cls();
            uLCD.color(GREEN);
            uLCD.text_width(2); //2X size text
            uLCD.text_height(2);
            uLCD.locate(0,0);
            uLCD.printf("%d", angle);
            cout << "100\n";
        }
        uLCD_print = 0;*/

        
    }

}



void gesture_mode()
{
uLCD.cls();
uLCD.text_width(2); //2X size text
uLCD.text_height(2);
uLCD.locate(0,0);
uLCD.printf("threshold\ndegree:%d", angle);
while (1) {
 if (checkled1 == 1) {
  
  // detect gesture
  // Whether we should clear the buffer next time we fetch data
  bool should_clear_buffer = false;
  bool got_data = false;

  // The gesture index of the prediction
  int gesture_index;


  // Set up logging.
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {

    error_reporter->Report(

        "Model provided is schema version %d not equal "

        "to supported version %d.",

        model->version(), TFLITE_SCHEMA_VERSION);

    return ;

  }


  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.

  static tflite::MicroOpResolver<6> micro_op_resolver;

  micro_op_resolver.AddBuiltin(

      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,

      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,

                               tflite::ops::micro::Register_MAX_POOL_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,

                               tflite::ops::micro::Register_CONV_2D());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,

                               tflite::ops::micro::Register_FULLY_CONNECTED());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,

                               tflite::ops::micro::Register_SOFTMAX());

  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,

                               tflite::ops::micro::Register_RESHAPE(), 1);


  // Build an interpreter to run the model with

  static tflite::MicroInterpreter static_interpreter(

      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);

  tflite::MicroInterpreter* interpreter = &static_interpreter;


  // Allocate memory from the tensor_arena for the model's tensors

  interpreter->AllocateTensors();


  // Obtain pointer to the model's input tensor

  TfLiteTensor* model_input = interpreter->input(0);

  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||

      (model_input->dims->data[1] != config.seq_length) ||

      (model_input->dims->data[2] != kChannelNumber) ||

      (model_input->type != kTfLiteFloat32)) {

    error_reporter->Report("Bad input tensor parameters in model");

    return ;

  }


  int input_length = model_input->bytes / sizeof(float);


  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);

  if (setup_status != kTfLiteOk) {

    error_reporter->Report("Set up failed\n");

    return ;

  }


  error_reporter->Report("Set up successful...\n");


  while (checkled1 == 1) {


    // Attempt to read new data from the accelerometer
    got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                 input_length, should_clear_buffer);


    // If there was no new data,
    // don't try to clear the buffer again and wait until next time
    if (!got_data) {
      should_clear_buffer = false;
      continue;
    }


    // Run inference, and report any error
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on index: %d\n", begin_index);
      continue;
    }


    // Analyze the results to obtain a prediction
    gesture_index = PredictGesture(interpreter->output(0)->data.f);


    // Clear the buffer next time we read data
    should_clear_buffer = gesture_index < label_num;

    if (gesture_index == 1) {
        if (state > 0) {
            state--;
        }
        else {
            state = 0;
        }
        uLCD_print = 1;
        cout << "state = " << state << "\n";
        angle = an[state];
        cout << angle << " degree\n";
        uLCD.background_color(BLACK);
        uLCD.cls();
        uLCD.color(GREEN);
        uLCD.text_width(2); //2X size text
        uLCD.text_height(2);
        uLCD.locate(0,0);
        uLCD.printf("threshold\ndegree:%d", angle);
    }
    else if (gesture_index == 0) {
        if (state < 3) {
            state++;
        }
        else {
            state = 3;
        }
        uLCD_print = 1;
        cout << state << "\n";
        angle = an[state];
        cout << angle << " degree\n";
        uLCD.background_color(BLACK);
        uLCD.cls();
        uLCD.color(GREEN);
        uLCD.text_width(2); //2X size text
        uLCD.text_height(2);
        uLCD.locate(0,0);
        uLCD.printf("threshold\ndegree:%d", angle);
    }
    

    // Produce an output
    if (gesture_index < label_num) {
      error_reporter->Report(config.output_message[gesture_index]);
    }

    

    // user bottom
    //btn2.rise(mqtt_queue.event(&publish_message, &client));
  }
    
  }
 }
}

void mqtt_main()
{
  wifi = WiFiInterface::get_default_instance();

    if (!wifi) {

            printf("ERROR: No WiFiInterface found.\r\n");

            return ;

    }



    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);

    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);

    if (ret != 0) {

            printf("\nConnection error: %d\r\n", ret);

            return ;

    }



    NetworkInterface* net = wifi;

    MQTTNetwork mqttNetwork(net);

    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    global_client = &client;


    //TODO: revise host to your IP

    const char* host = "192.168.31.176";

    printf("Connecting to TCP network...\r\n");


    SocketAddress sockAddr;

    sockAddr.set_ip_address(host);

    sockAddr.set_port(1883);


    printf("address is %s/%d\r\n", (sockAddr.get_ip_address() ? sockAddr.get_ip_address() : "None"),  (sockAddr.get_port() ? sockAddr.get_port() : 0) ); //check setting


    int rc = mqttNetwork.connect(sockAddr);//(host, 1883);

    if (rc != 0) {

            printf("Connection error.");

            return ;

    }

    printf("Successfully connected!\r\n");


    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    data.MQTTVersion = 3;

    data.clientID.cstring = "Mbed";


    if ((rc = client.connect(data)) != 0){

            printf("Fail to connect MQTT\r\n");

    }

    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){

            printf("Fail to subscribe\r\n");

    }


    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    
    btn2.rise(mqtt_queue.event(&publish_message, &client));
    
    

    //btn3.rise(&close_mqtt);


    int num = 0;

    while (num != 5) {

            client.yield(100);

            ++num;

    }


    while (1) {

            if (closed) break;

            client.yield(500);

            ThisThread::sleep_for(500ms);

    }


    printf("Ready to close MQTT Network......\n");


    if ((rc = client.unsubscribe(topic)) != 0) {

            printf("Failed: rc from unsubscribe was %d\n", rc);

    }

    if ((rc = client.disconnect()) != 0) {

    printf("Failed: rc from disconnect was %d\n", rc);

    }


    mqttNetwork.disconnect();

    printf("Successfully closed!\n");


    return ;
}

void detection_mode()
{
    int overtilt = 0;
    int reset = 1;
    int16_t pDataXYZ[3] = {0};
    double offset;
    int wait_for_offset = 0;
    checkled3 = 1;
    while (1) {
        if (checkled2) {
            while (reset == 1 && wait_for_offset < 50) {
                if (wait_for_offset == 1) cout << "measuring reference acceleration vector...\nplease wait for 5s...\n";
                checkled3 = !checkled3;
                ThisThread::sleep_for(100ms);
                wait_for_offset++;
                BSP_ACCELERO_AccGetXYZ(pDataXYZ);
                offset = sqrt((double)pDataXYZ[0]*(double)pDataXYZ[0]+(double)pDataXYZ[1]*(double)pDataXYZ[1]+(double)pDataXYZ[2]*(double)pDataXYZ[2]);
                if (wait_for_offset == 50) cout << "start tilt detect...\n";
            }
            reset = 0;
            wait_for_offset = 0;
            
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            detect_angle = 57.3*acos((double)pDataXYZ[2]/offset);
            uLCD.background_color(BLACK);
            uLCD.cls();
            uLCD.color(GREEN);
            uLCD.text_width(2); //2X size text
            uLCD.text_height(2);
            uLCD.locate(0,0);
            uLCD.printf("%d", (int)detect_angle);
            cout << "detected angle = " << detect_angle << "\n";
            if (detect_angle > angle) {
                overtilt++;
                uLCD.locate(0,1);
                uLCD.color(RED);
                uLCD.printf("overtilt %d times", overtilt);
                publish_message(global_client);
            }
            ThisThread::sleep_for(300ms);
            if (overtilt >= 5) {
                //checkled2 = 0;
                overtilt = 0;
                uLCD.cls();
                reset = 1;
                cout << "stop tilt detection mode" << "\n";
            }
            
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
