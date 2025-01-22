/*********************************************************************

*********************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>

//pin-pin-an + address ****************************

//Stepper pin (DRV8825)
#define dirPin 12
#define stepPin 13

//Encoder pin
#define CLK_En 14
#define DT_En 16
#define SW_En 15

//HX711 loadcell
#define DT_HX711 0
#define SCK_HX711 2
HX711 LoadCell;

// by default Oled Config
// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

//end pin-pin-an + address ****************************

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 48) //checking if wrong oled lcd used, since 64x48 is kinda rarelly used nowdays
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


//var
float force=120;
int linAdvance=-1;
int stepperStatus=0;
int stepsPerMM = 100*16; //since 1/16 microsteps used

//var calc
int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;

int displayRefreshRate=500;//millis
//end var

void dataShow () {
  //distance between line 8dots, distance between char 6dots
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println("<EXTR_DRV>");

//Display Force
  display.setCursor(0,13);
  display.print("F=");
  display.setCursor(18,13);
  
  float force = LoadCell.get_units()/1000;//can be average unit per meass
  if(force<0){
    display.setCursor(12,13);
  }
  display.print(force,0);
  display.setCursor(42,13);
  display.print("kgF");

//Display Travel Distance
  display.setCursor(0,22);
  display.print("S=");
  display.setCursor(18,22);
  if(linAdvance<0){
    display.setCursor(12,22);
  }
  display.print(linAdvance);
  display.setCursor(42,22);
  display.print("mm");

//Display Status Stepper
  display.setCursor(0,31);
    switch (stepperStatus) {
    case 0:
      display.print("IDLE");
      break;
    case 1:
      display.print("ADVANCE");
      break;
    case 2:
      display.print("RETRACT");    
      break;
    default:
      display.print("Err 01"); //display Error
      break;
  }


  display.setCursor(30,40);
  display.print(millis()/1000);
  // display.setCursor(30,40);
  display.print(" s");
 
  display.display();
 // delay(500);
}

void stepper() {
	
  stepperStatus=1; //set status to "ADVANCE"
  
  digitalWrite(dirPin, LOW);
  if(linAdvance<0){
    	digitalWrite(dirPin, HIGH);
      stepperStatus=2; //set status to "RETRACT"
  }
  dataShow();

  	for(int x = 0; x < abs(stepsPerMM*linAdvance); x++)
	{
		digitalWrite(stepPin, HIGH);
		delay(2);
		digitalWrite(stepPin, LOW);
		delay(2);
    if( millis()%displayRefreshRate == 0){
      dataShow();
    }
	}

  stepperStatus=0; //set status to "ADVANCE"
  dataShow();

	delay(1000); // Wait a second
}

void encoder() {
        
	// Read the current state of CLK_En

	currentStateCLK = digitalRead(CLK_En);

	// If last and current state of CLK_En are different, then pulse occurred
	// React to only 1 state change to avoid double count
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

		// If the DT_En   state is different than the CLK_En state then
		// the encoder is rotating CCW so decrement
		if (digitalRead(DT_En) != currentStateCLK) {
			linAdvance --;
			currentDir ="CCW";
		} else {
			// Encoder is rotating CW so increment
			linAdvance ++;
			currentDir ="CW";
		}

	//	Serial.print("Direction: ");
	//	Serial.print(currentDir);
	//	Serial.print(" | Counter: ");
	//	Serial.println(linAdvance);
	}

	// Remember last CLK_En state
	lastStateCLK = currentStateCLK;

	// Read the button state
	int btnState = digitalRead(SW_En);

	//If we detect LOW signal, button is pressed
	if (btnState == LOW) {
		//if 50ms have passed since last LOW pulse, it means that the
		//button has been pressed, released and pressed again
		if (millis() - lastButtonPress > 50) {
	//		Serial.println("Button pressed!");
      stepper(); // execute stepper movement

		}

		// Remember last button press event
		lastButtonPress = millis();
	}

	// Put in a slight delay to help debounce the reading
	//delay(1);
}

void setup()   {
//  Serial.begin(9600);
  //while (!Serial) {} // wait for serial port to connect. Needed for native USB port only
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.clearDisplay();
  display.display();

  // test display
  // display.clearDisplay();
  // display.drawBitmap(30, 16,  logo16_glcd_bmp, 24, 16, 1);
  // display.display();
  // delay(5000);

  //setup HX711
  LoadCell.begin(DT_HX711, SCK_HX711);
  LoadCell.set_scale(-9.76); // calibration factor = (reading)/(known weight)
  LoadCell.tare(); // reset the LoadCell to 0

  //pinmod for DRV8825
  pinMode(stepPin, OUTPUT);
	pinMode(dirPin, OUTPUT);

  //pinmod for Encoder
  pinMode(CLK_En,INPUT);
	pinMode(DT_En,INPUT);
	pinMode(SW_En, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK_En);
  
  delay(2000); //set delay for init
}


void loop() {
  encoder();
  if(millis()%displayRefreshRate==0){
    dataShow();
  }
  delay(1);
}
