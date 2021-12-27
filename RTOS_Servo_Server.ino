#include "tasks.h"

// Network credentials
const char* ssid     = "AndroidRed";
const char* password = "1234567890ab";

WiFiServer server(80);              // Web server on port 80 (http)
String header;                      // Variable to store the HTTP request

// Decode HTTP GET value
bool sweepEanble= false;
bool ledState= false;
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

int motor_current_pos =0;
int motor_min_pos=90;
int motor_max_pos=120;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


enum LedStatus { OFF, ON , BLINK }; 
LedStatus led_status = OFF;

enum MotorStatus { STOP, MANUAL, SWEEP, EMERGENCY_STOP};
MotorStatus motor_status = STOP;


void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) // A Task shall never return or exit.
  {
    switch(led_status)
    {
      case BLINK:
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        vTaskDelay(1000);  // one tick delay (15ms) in between reads for stability
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        vTaskDelay(500);  // one tick delay (15ms) in between reads for stability
        break;
      case ON:
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
        break;
      case OFF:
        digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
        vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
        break;
    }
  }    
}

void TaskMotor(void *pvParameters)  // This is a task.
{
  (void) pvParameters;  
  Servo myservo;                      // create servo object to control a servo
  static const int servoPin = 18;     // Servo GPIO pin
  myservo.setPeriodHertz(50);         // Set servo PWM frequency to 50Hz
  myservo.attach(servoPin,600, 2400);// Attach to servo and define minimum and maximum positions
  
  for (;;) // A Task shall never return or exit.
  {
    switch(motor_status)
    {
      case STOP:
        vTaskDelay(10); 
        break;
      case MANUAL:
        myservo.write(motor_current_pos);  
        break;
      case SWEEP:
        for (int pos = motor_min_pos; pos <= motor_max_pos; pos += 1) {
          if(motor_status== SWEEP)
            myservo.write(pos);   
          else
            break;
          delay(15);             
        }
        for (int pos = motor_max_pos; pos >= motor_min_pos; pos -= 1) {
          if(motor_status== SWEEP)
            myservo.write(pos);   
          else
            break;  
          delay(15);            
        } 
        break;
      case EMERGENCY_STOP:
        myservo.detach(); 
        break;
    }
  }
}



// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  0);

  xTaskCreatePinnedToCore(
    TaskMotor
    ,  "Motor Sweep"
    ,  1024  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  1);

 
  
  // Start serial
  Serial.begin(115200);


  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  led_status = ON;
}

void loop()
{
 // Listen for incoming clients
  WiFiClient client = server.available();   
  
  // Client Connected
  if (client) {                             
    // Set timer references
    currentTime = millis();
    previousTime = currentTime;
    
    // Print to serial port
    Serial.println("New Client."); 
    
    // String to hold data from client
    String currentLine = ""; 
    
    // Do while client is cponnected
    while (client.connected() && currentTime - previousTime <= timeoutTime) { 
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
        
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK) and a content-type
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

                        // GET data
            if(header.indexOf("GET /?value=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              
              // String with motor position
              valueString = header.substring(pos1+1, pos2);
              
              // Move servo into position
              sweepEanble = false;
              led_status= ON;
              led_status= ON;
              motor_current_pos = valueString.toInt();
              motor_status= MANUAL;
              
              // Print value to serial monitor
              Serial.print("Val =");
              Serial.println(valueString); 
            }


            if (header.indexOf("GET /on") >= 0) {
              Serial.println("Sweep on");
              sweepEanble = true;
              led_status= BLINK;
              motor_status = SWEEP;
            } else if (header.indexOf("GET /off") >= 0) {
              Serial.println("Sweep off");
              sweepEanble = false;
               led_status= ON;
                motor_status = STOP;
            }else if(header.indexOf("GET /cal_min") >= 0) {
              Serial.println("callibrate minimum");
              sweepEanble = false;
               led_status= ON;
                motor_status = STOP;
                motor_min_pos=valueString.toInt();                
            }else if(header.indexOf("GET /cal_max") >= 0) {
              Serial.println("callibrate maximum");
              sweepEanble = false;
               led_status= ON;
                motor_status = STOP;
                motor_max_pos=valueString.toInt();                
            }else if(header.indexOf("GET /emergency") >= 0) {
              Serial.println("Emergency STOP");
              sweepEanble = false;
               led_status= ON;
                motor_status = EMERGENCY_STOP;              
            }

            // Display the HTML web page
            
            // HTML Header
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            
            // CSS - Modify as desired
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto; }");
            client.println(".slider { -webkit-appearance: none; width: 300px; height: 25px; border-radius: 10px; background: #ffffff; outline: none;  opacity: 0.7;-webkit-transition: .2s;  transition: opacity .2s;}");
            client.println(".slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; border-radius: 50%; background: #ff3410; cursor: pointer; }");

            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #FF3333;}</style>");
           
            // Get JQuery
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
                     
            // Page title
            client.println("</head><body style=\"background-color:#70cfff;\"><h1 style=\"color:#000000;\">Right Arm Control</h1>");
            
            // Position display
            client.println("<h2 style=\"color:#ffffff;\">Position: <span id=\"servoPos\"></span>&#176;</h2>"); 
                     
            // Slider control
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");

            // Button ON/Off control
            if (sweepEanble==false)
              client.println("<p><a href=\"/on\"><button class=\"button\">START</button></a></p>");
            else  
              client.println("<p><a href=\"/off\"><button class=\"button button2\">STOP</button></a></p>");
            
            // Button Callibrate Min
            client.println("<p><a href=\"/cal_min\"><button class=\"button\">Callibrate min</button></a></p>");
            // Button Callibrate Max
            client.println("<p><a href=\"/cal_max\"><button class=\"button\">Callibrate max</button></a></p>");
            // Button Emergency Stop
            client.println("<p><a href=\"/emergency\"><button class=\"button button2\">Emergency STOP</button></a></p>");
            
            
            // Javascript
            client.println("<script>var slider = document.getElementById(\"servoSlider\");");
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");
            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");
            
            // End page
            client.println("</body></html>");     
            

            // The HTTP response ends with another blank line
            client.println();
            
            // Break out of the while loop
            break;
          
          } else { 
            // New lline is received, clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }  
}
