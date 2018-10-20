#include <i2c_t3.h>

void setup() {
  // put your setup code here, to run once:
  Wire2.begin();        // join i2c bus (address optional for master)

  Serial.begin(115200);  // start serial for output

}
static byte msg[5];

void loop() {
  // put your main code here, to run repeatedly:
  Wire2.requestFrom(8, 5);    // request 5 bytes from slave device #8 
  int i = 0;
  while (Wire2.available() && (i<5) ) { // slave may send less than requested
    msg[i++] = Wire2.read(); // receive a byte    
  }
  
  Serial.println(msg[0], BIN);
  Serial.println(msg[1], BIN);
  Serial.println(msg[2], BIN);
  Serial.println(msg[3], BIN);
  Serial.println(msg[4], BIN);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  
 // delay(500);
}
