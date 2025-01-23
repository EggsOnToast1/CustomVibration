/*
Author: Isaac Benson
Github: IsaacBensonUMBC
Last Updated: 1/23/2025
Purpose: Arduino code for muscle vibrator with drawable arbitrary forcing function
Credit: Some code from Elegoo_TFTLCD examples (https://github.com/Erutan409/Elegoo_TFTLCD) taken for touchscreen controls and that code is under the MIT license. All other code is original.
CustomVibtration.h: header for CustomVibtration.ino
*/

// Libraries for the touchscreen ability
// These need to be downloaded and loaded in the arduino IDE before this program can run
#include <Elegoo_GFX.h>    
#include <Elegoo_TFTLCD.h> 
#include <TouchScreen.h>

 //constants for writing to screen
#define screenWidth 240
#define screenHeight 320
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

//constants for touchscreen
#define YP A3  
#define XM A2  
#define YM 9   
#define XP 8   
#define TSDataPin 13

 //Touchscreen min/max's
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920


#define MINPRESSURE 10 // upper/lower bounds of pressure for Touchscreen
#define MAXPRESSURE 1000

#define	BLACK   0x0000 // Assign human-readable names to some color values:
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define LIGHTGRAY 0xf7de
#define DARKGRAY 0xb5b6
#define LIGHTRED 0xfc4e
#define LIGHTGREEN 0x77f0

//constants for setting Voltage
const float MINVOLT = 5.0; //Note: this is the minimum "maximum voltage" of the function. There can be 0V (and negative voltage for backstopping).
const float MAXVOLT = 9.0; // This is the absolute maximim voltage (in volts) for the circuit output (independent of the distribution)
const int MINTIME = 10;  //This and below are the maximum/minimum one element in the forceArray can take (in ms)
const int MAXTIME = 100;
const int ONTIME = 35; // This is the interval for HIGH for Pulse Width Modulation (PWM) in milliseconds (ms). This was empirically determined.
const int posPWMPin = 10; // This and below are the pins for forward and backward movement of the ERM motor.
const int negPWMPin = 11;
const float BACKSTOPVOLTAGE = -0.6; //minimum % of total voltage jump from one pulse to the next for backstopping to be needed (ex. 5V to 1V is drop greater than 60% OF 5V)
const int BACKSTOPPINGTIME = 7; //number of ms of backstop

//variables for setting Voltage
unsigned long beginTime = millis(); //beginning time for PWM
bool nowHigh = false; // boolean for PWM of circuit
float currentVoltage = 0.0; // current voltage outputted to the circuit



//constants for Touchscreen
const unsigned long BUTTONTIME = 200; //time (in ms) that need to be held for a button push to count
const unsigned long SLIDERTIME = 25; //time (in ms) that need to be held for a slider push to count
const int drawnLeftBound = screenHeight/3+1; //this and below are the left and right boundaries of the drawing area
const int drawnRightBound = screenHeight-2;
const int drawnDistributionLength = drawnRightBound-drawnLeftBound+1; //length of drawnDistribution array

//varaibles for Touchscreen
bool firstDraw = true; //bool for whether the draw of touchscreen function draw is first
bool motorOn = false; //bool for whether the motor is currently running or not
bool motorChangeFirstTime = false; //if motor is on or off, is it the first cycle after it's change?
unsigned long pushBeginTime = millis(); //timer for when a button is pushed or not
float leftBarPosition = 0.5; //position from 0 to 1 of the left bar (frequency)
float rightBarPosition = 0.5; //Same as above but for right bar
bool leftBarChange = false; //bool for whether or not the left bar has been moved
bool rightBarChange = false;//bool for whether or not the right bar has been moved
int funcDrawX = -1; //This and below are the x and y coordinates for drawing the original function
int funcDrawY = -1;
int drawnDistribution[drawnDistributionLength] = {0}; //This is integer array 0-100 from 0 being lowest power to 100 being maximum power that is drawn by the user

const int taskQueueLength = 2; //constant for taskQueue length

// Functions for setting Voltage
bool setVoltage(float input = 0.0); //function to set voltage of circuit via Pulse Width Modulation (PWM). This need to run constantly to work.

//Functions for the screen
bool inRect(int inputX, int inputY, int lowX, int highX, int lowY, int highY); //function to return whether a touched point on TS is in the area
void screenSetup(); //set up initial screen
void updateScreen(); //real time update of screen from changes in updateTouch. This runs constantly.
void updateTouch(); //From inputted point, we update bool values that will change the screen. This runs constantly.
bool buttonProperPush(unsigned long inputTime); //This checks if the time of touch is long enough to count as a button push, then returns true/false
bool sliderProperPush(unsigned long inputTime);//This checks if the time of touch is long enough to count as a slider push, then returns true/false
void drawMotorOn(); // updates delete/save button to indicate they can't be saved/deleted and start/stop button says "stop"
void drawMotorOff(); // updates delete/save button to indicate they can be saved/deleted and start/stop button says "start"
void fillInDrawnDistribution(); //If there are gaps in the drawnDistribution, it fills them in at the preivously drawn point

void (*taskQueue[taskQueueLength])(); //declare task queue for calling functions in loop()

//This is a class for a force distribution that is inputted by the user. Note: this is seperate from the drawn distribution since the forceArray has the backstopping included
// For each element in the forceArray, it outputs that as the voltage and hold it for the corresponding element in the timeArray
// Note: In all of the boolean member functions, a return of true means that the function was successful (and vis versa).
class ForceDistribution {
public:
  ForceDistribution(int* setDistributionInput = nullptr, int arraySize = 0, float maxVoltInput = MINVOLT + (MAXVOLT-MINVOLT)/2, int setTimeInput = MINTIME + (MAXTIME-MINTIME)/2);   //constructor class
  ~ForceDistribution();   //deconstrutor just calls reset() function
  bool setMaxVoltage(float inputVoltage);   //setter for changing max voltage
  bool setTimePerInterval(int newTime); //setter for changing timePerInterval
  void reset();   //This sets object member variables to the default and delete dynamic memory
  bool setDistribution(int* inputDistribution, int arraySize); // this sets the forceArray of the class given the array input from 0.0 to 1.0 and calculates/includes the backstopping
  bool restartForcing(); // If the forcing is stopped mid-way, this function can reset it and start from the beginning
  bool startForcing(); //This outputs the force distribution  in the forceArray to the ERM motor by holding each element with time variable (in ms). This needs to run in real time.
  void printList(); //For manual testing, this prints the two arrays (force and time) of the class (line by line)

private:
  int* forceArray = nullptr;   // These are the pointers to the force array and the time interval array.
  int* timeArray = nullptr;
  int arrSize = 0;   // this is the array size of the force and time array
  float maxVoltage = MINVOLT + (MAXVOLT-MINVOLT)/2;   //this represents the maximum voltage outputted. It also scales the rest of the distribution
  int timePerInterval = MINTIME + (MAXTIME-MINTIME)/2;   //This is the default time per interval
  unsigned long startTime = 0;   //This variable is for the startForcing() function for measuring time intervals
  bool startRun = true;   // This boolean decides whether the forcing starts from the beginning or not
  int currentForce = 0;   // incrementer for current element of forcing from forceArray
};

//object declarations for CustomVibration.ino file
ForceDistribution forceDis = ForceDistribution(); //create our only forceDistribution object

//Setup of touchscreen
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
