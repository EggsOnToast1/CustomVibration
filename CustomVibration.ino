/*
Author: Isaac Benson
Github: EggsOnToast1
Last Updated: 1/23/2025
Purpose: Arduino code for muscle vibrator with drawable arbitrary forcing function
Credit: Some code from Elegoo_TFTLCD examples (https://github.com/Erutan409/Elegoo_TFTLCD) taken for touchscreen controls and that code is under the MIT license. All other code is original.
CustomVibtration.ino: function definitions and program execution (setup() and loop())
*/
#include "CustomVibration.h" //include header file

bool setVoltage(float input = 0.0){
  //This function sets the voltage from -MAXVOLT to MAXVOLT using PWM
  //It return true if the input is defined and false otherwise
  
  unsigned long currentTime = millis();   //Measure for currentTime when run
  int usedPWMPin = -1;   // This is the pin that will be turned on/off

  if(input == 0.0){
    //if zero voltage, set everything to low, reset the timer, setting the current voltage, and then return true
    digitalWrite(negPWMPin,LOW);
    digitalWrite(posPWMPin,LOW);
    nowHigh = false;
    beginTime = currentTime;
    currentVoltage = input;

    return true;
  } else if((input > 0.0) && (input <= MAXVOLT)) {
    //if input is positive and within bounds, we use positive chip pin
    usedPWMPin = posPWMPin;

    //now make sure negative pin is low
    digitalWrite(negPWMPin,LOW);

    //assign new voltage
    currentVoltage = input;
  } else if((input < 0.0) && (input >= -MAXVOLT)) {
    //if input is negative, we use negative chip pin
    usedPWMPin = negPWMPin;

    //now make sure positive pin is low
    digitalWrite(posPWMPin,LOW);

    //assign new voltage
    currentVoltage = input;
  } else {
    //this is a case where the input is an error (ie not setting properly).
    //We send an error message to Serial and return false
    Serial.println("ERROR: Invalid input for setVoltage Function. Voltage is: ");
    Serial.print(input);
    return false;
  }

  //this section controls the PWM

  if(nowHigh && (currentTime - beginTime >= ONTIME)){
    //If it is on and needs to be turned off (after set ONTIME length), we write low to the used pin and switch boolean
    digitalWrite(usedPWMPin,LOW);
    beginTime = currentTime;
    nowHigh = false;
    
  }
   if((!nowHigh) && (currentTime - beginTime >= (ONTIME*MAXVOLT/abs(currentVoltage) - ONTIME))){
    // The time forumla above is derived from average voltage formula: V_avg = ONTIME/(ONTIME+OFFTIME)*MAXVOLT and solving for OFFTIME.
    // The currentVoltage is the average voltage in the formula
    
    //If it is off and needs to be turned on, we write high to the used pin and switch boolean
    digitalWrite(usedPWMPin,HIGH);
    beginTime = currentTime;
    nowHigh = true;
  }

  return true;
}

bool inRect(int inputY, int inputX, int lowY, int lowX, int highY, int highX){
  //This returns bool value of (inputX,inoputY) are in the rectangle
  return ((lowX <= inputX) && ((highX+lowX) >= inputX) && (lowY <= inputY) && ((highY+lowY) >= inputY));
}

void screenSetup(){
  //Here, we reset the screen and wipe it
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);

  tft.fillRect(0,screenHeight/3,screenWidth,screenHeight*2.0/3,WHITE); //top-right screen for drawing
  tft.drawRect(0,screenHeight/3,screenWidth,screenHeight*2.0/3,BLACK); //border around white box above

  tft.fillRect(0,0,screenWidth/8,screenHeight/3,BLUE); // START/STOP button
  tft.drawRect(0,0,screenWidth/8,screenHeight/3,BLACK);

  tft.fillRect(screenWidth/8,0,screenWidth/8,screenHeight/6,RED); //DELETE Button
  tft.drawRect(screenWidth/8,0,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,GREEN); //SAVE Button
  tft.drawRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(screenWidth/4,0,screenWidth*3.0/4,screenHeight/3,LIGHTGRAY); //side bar for speed/amplitude
  tft.drawRect(screenWidth/4,0,screenWidth*3.0/4,screenHeight/3,BLACK); 

  tft.fillRect(screenWidth/4+20,screenHeight/12, screenWidth-(screenWidth/4+20)-20-20, 5, DARKGRAY); //Left bar (for freq)
  tft.fillRect(screenWidth/4+20,screenHeight/3-screenHeight/12-5, screenWidth-(screenWidth/4+20)-20-20, 5, DARKGRAY); //Right bar (for amplitude)

  tft.fillRect(screenWidth/4+20+(screenWidth-(screenWidth/4+20)-20-20)/2,screenHeight/12-10, 7, 25, BLUE); //left slider
  tft.fillRect(screenWidth/4+20+(screenWidth-(screenWidth/4+20)-20-20)/2,screenHeight/3-screenHeight/12-5-10, 7, 25, BLUE); //right slider

  //Text above the two sliders
  tft.setTextColor(BLACK);
  tft.setRotation(1);
  tft.setTextSize(2);

  tft.setCursor(5,15);
  tft.print("FREQ");

  tft.setCursor(65,15);
  tft.print("AMP");

  //Text for bottom left set of buttons

  tft.setCursor(screenHeight/12-2,screenWidth-22);
  tft.print("START");

  tft.setCursor(10,screenWidth*5/6-12);
  tft.print("DEL");

  tft.setCursor(4+screenHeight/6,screenWidth*5/6-12);
  tft.print("SAVE");

  
  //draw "Put function here" in white draw box
  tft.setTextSize(3);
  tft.setCursor(screenHeight*1.25/3,screenWidth*1.0/2-10);
  tft.setTextColor(LIGHTGRAY);
  tft.print("DRAW HERE");
  


  //reset rotation
  tft.setRotation(0);

  //Now for set/reset of drawing data
  //set the drawnDistribution array to all -1.0 to signify no input yet
  for(int i = 0; i < drawnDistributionLength; i++){
    drawnDistribution[i] = -1;
  }

}

void updateScreen(){
  //Here we toggle the motorOn/motorOff screen
  if(motorOn){
    if(motorChangeFirstTime){
      drawMotorOn();
      motorChangeFirstTime = false;

    }

    forceDis.startForcing(); //start forcing of motor
  } else { 
    //Otherwise, we allow all functions of the motor off screen
    if(motorChangeFirstTime){
      drawMotorOff();
      motorChangeFirstTime = false;
    }
  }

  //Here, we redraw the left bar screen if it was changed
  if(leftBarChange){
    leftBarChange = false; 

    //Now, we redraw the left bar area to the new position
    tft.fillRect(screenWidth/4+1,1,screenWidth*3.0/4-33,screenHeight/6-2,LIGHTGRAY); //left side bar for speed/amplitude
    tft.fillRect(screenWidth/4+20,screenHeight/12, screenWidth-(screenWidth/4+20)-20-20, 5, DARKGRAY); //Left bar (for freq)
    tft.fillRect(screenWidth/4+20+(screenWidth-(screenWidth/4+20)-20-20)*leftBarPosition,screenHeight/12-10, 7, 25, BLUE); //left slider
  }

  //Same as above but for right bar
  if(rightBarChange){
    rightBarChange = false;

    //Now, we redraw the right bar area to the new position
    tft.fillRect(screenWidth/4+1,screenHeight/6+1,screenWidth*3.0/4-33,screenHeight/6-2,LIGHTGRAY); //right side bar for speed/amplitude
    tft.fillRect(screenWidth/4+20,screenHeight/3-screenHeight/12-5, screenWidth-(screenWidth/4+20)-20-20, 5, DARKGRAY); //Right bar (for amplitude)
    tft.fillRect(screenWidth/4+20+(screenWidth-(screenWidth/4+20)-20-20)*rightBarPosition,screenHeight/3-screenHeight/12-5-10, 7, 25, BLUE); //right slider

  }

  //Here, we check any drawing
  if((funcDrawX != -1) && (funcDrawY != -1)){
    
    //If it was the user's first draw, we wipe the screen to get rid of the "DRAW HERE" message
    if(firstDraw){
      firstDraw = false;
      tft.fillRect(0,screenHeight/3,screenWidth,screenHeight*2/3,WHITE); //box for drawing space
      tft.drawRect(0,screenHeight/3,screenWidth,screenHeight*2.0/3,BLACK); //border around white box above
    }
    
    //Now, we check if the touched point is within bound and draw it if it is
    if((funcDrawY+3) <= drawnRightBound){
      tft.fillRect(1,funcDrawY,screenWidth-2,3,WHITE);
      tft.fillRect(funcDrawX,funcDrawY,3,3,BLUE);
    } else if((funcDrawY+2) <= drawnRightBound){
      tft.fillRect(1,funcDrawY,screenWidth-2,2,WHITE);
      tft.fillRect(funcDrawX,funcDrawY,3,2,BLUE);
    } else if((funcDrawY+1) <= drawnRightBound){
      tft.fillRect(1,funcDrawY,screenWidth-2,1,WHITE);
      tft.fillRect(funcDrawX,funcDrawY,3,1,BLUE);
    }
    

    //update at, 1 and 2 right points to drawnDistribution array if they are in bounds
    drawnDistribution[funcDrawY-drawnLeftBound] = (int) (100*((float) funcDrawX) / screenWidth);

    if((funcDrawY-drawnLeftBound+1) < drawnDistributionLength){
      drawnDistribution[funcDrawY-drawnLeftBound+1] = (int) (100*((float) funcDrawX) / screenWidth);
    }
    
    if((funcDrawY-drawnLeftBound + 2) < drawnDistributionLength){
      drawnDistribution[funcDrawY-drawnLeftBound+2] = (int) (100*((float) funcDrawX) / screenWidth);
    }

    //reset variables to their default state
    funcDrawX = -1;
    funcDrawY = -1;
  }
}

void updateTouch(){
  //Here, we get the touched point from the TSDataPin 
  digitalWrite(TSDataPin, HIGH);
  TSPoint point = ts.getPoint();
  digitalWrite(TSDataPin, LOW);

  //reset pins to output for real time output
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  //If the point is within the pressure thresholds, we utilize the touched point
  if((point.z > MINPRESSURE) && (point.z < MAXPRESSURE)){

    //adjust input of points to their proper position
    point.x = tft.width() - 240.0/(245+7)*(map(point.x, TS_MINX, TS_MAXX, screenWidth, 0)+7);
    point.y =  320.0/(318-7)*(map(point.y, TS_MINY, TS_MAXY, screenHeight, 0)-7);
  
    if(inRect(point.x,point.y,0,0,screenWidth/8,screenHeight/3) && buttonProperPush(millis())){     //first, we check if Start/Stop button is pushed

      //Here, we toggle the motor's bool value and state that if was change for the first time
      motorOn = !motorOn;
      motorChangeFirstTime = true;

      //If we jsut changed it to off, we set voltage to zero
      if(!motorOn){
        setVoltage(0.0);
      }
    } else if(inRect(point.x,point.y,screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6) && buttonProperPush(millis()) && !motorOn){ //Now, we check for the save button press
      //Here, we know the button was pushed in the right spot and time and the motor is off

      //fill in holes in drawn distribution
      fillInDrawnDistribution();
        
      //Now, we update the forceDistirbution with the drawn distribution
      forceDis.setDistribution(drawnDistribution, drawnDistributionLength);
    } else if(inRect(point.x,point.y,screenWidth/8,0,screenWidth/8,screenHeight/6) && buttonProperPush(millis()) && !motorOn){  //Check for Delete button press
      //If the delete button is pressed, we reset the entire screen, reset default values, and reset the forceDistribution object

      //reset forceDis object
      forceDis.reset();

      //reset all screen variables 
      firstDraw = true;
      motorOn = false;
      motorChangeFirstTime = false; 
      pushBeginTime = millis();
      leftBarPosition = 0.5; 
      rightBarPosition = 0.5;
      leftBarChange = false;
      rightBarChange = false;
      funcDrawX = -1;
      funcDrawY = -1;

      //reset all motor variables
      beginTime = millis();
      nowHigh = false;
      currentVoltage = 0.0;

      //now reset screen
      screenSetup();
    } else if(inRect(point.x,point.y,screenWidth/4+20,0,screenWidth-(screenWidth/4+20)-20-20,screenHeight/6) && sliderProperPush(millis()) && !motorOn){ // Left Bar Push for Frequency
      //Here, we translate the x position point to a 0.0 to 1.0 float to denote the relative position
      leftBarPosition = ((float) point.x - (screenWidth/4+20)) / ((float) screenWidth-(screenWidth/4+20)-20-20 );
      
      //we now update the time in the forceDistribution function and state the the left bar has been changed
      forceDis.setTimePerInterval(MINTIME + (MAXTIME-MINTIME)*(1-leftBarPosition));

      leftBarChange = true; 
    } else if(inRect(point.x,point.y,screenWidth/4+20,screenHeight/6,screenWidth-(screenWidth/4+20)-20-20,screenHeight/6) && sliderProperPush(millis()) && !motorOn){  //Right bar Push for Amplitude
      //Here, we translate the x position point to a 0.0 to 1.0 float to denote the relative position
      rightBarPosition = ((float) point.x - (screenWidth/4+20)) / ((float) screenWidth-(screenWidth/4+20)-20-20 );

      //we now update the time in the forceDistribution function and state the the left bar has been changed
      forceDis.setMaxVoltage(MINVOLT + (MAXVOLT-MINVOLT)*rightBarPosition);
      rightBarChange = true;
    } else if(inRect(point.x,point.y,0,screenHeight/3,screenWidth,screenHeight*2/3) && !motorOn){ //Right side draw function

      //we get the points from the screen and save them for updateScreen()
      funcDrawX = point.x;
      funcDrawY = point.y;
    }
  }
}

bool buttonProperPush(unsigned long inputTime){
  //This checks if the time of touch is long enough to count as a button push
  if((inputTime-pushBeginTime) > BUTTONTIME){
    pushBeginTime = millis();
    return true;
  }

  return false;
}

bool sliderProperPush(unsigned long inputTime){
  //This checks if the time of touch of slider is long enough to count as a slider push

  if( (inputTime-pushBeginTime) > SLIDERTIME){
    pushBeginTime = millis();
    return true;
  }

  return false;
}

void drawMotorOn(){
  tft.fillRect(screenWidth/8,0,screenWidth/8,screenHeight/6,LIGHTGRAY); //DELETE Button with gray background
  tft.drawRect(screenWidth/8,0,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,LIGHTGRAY); //SAVE Button with gray background
  tft.drawRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(0,0,screenWidth/8,screenHeight/3,BLUE); // redraw START/STOP button
  tft.drawRect(0,0,screenWidth/8,screenHeight/3,BLACK);

  tft.setTextColor(BLACK); //set for writing text
  tft.setRotation(1);
  tft.setTextSize(2);

  tft.setCursor(10,screenWidth*5/6-12); //write for delete button 
  tft.print("DEL");

  tft.setCursor(4+screenHeight/6,screenWidth*5/6-12); //write for save button 
  tft.print("SAVE");

  tft.setCursor(screenHeight/12+2,screenWidth-22); //write stop for start/stop button
  tft.print("STOP");

  tft.setRotation(0); //reset rotation
}

void drawMotorOff(){
  tft.fillRect(screenWidth/8,0,screenWidth/8,screenHeight/6,RED); //DELETE Button
  tft.drawRect(screenWidth/8,0,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,GREEN); //SAVE Button
  tft.drawRect(screenWidth/8,screenHeight/6,screenWidth/8,screenHeight/6,BLACK);

  tft.fillRect(0,0,screenWidth/8,screenHeight/3,BLUE); // START/STOP button
  tft.drawRect(0,0,screenWidth/8,screenHeight/3,BLACK);

  tft.setTextColor(BLACK); //set for writing text
  tft.setRotation(1);
  tft.setTextSize(2);
  
  tft.setCursor(screenHeight/12-2,screenWidth-22); //write start  for start/stop button
  tft.print("START");

  tft.setCursor(10,screenWidth*5/6-12); //write delete button 
  tft.print("DEL");

  tft.setCursor(4+screenHeight/6,screenWidth*5/6-12); //write save for save button
  tft.print("SAVE");

  tft.setRotation(0); //reset rotation
}

void fillInDrawnDistribution(){
  bool noInput = true; //bool for whether there was any drawing input or not
  int firstInputPosition = -1; //place of array of first found drawn position
  int currentPosition = -1; //If filled in somewhat, it represents the current position of the array

  //here, we check for any input whatsoever
  for(int i = 0; i < drawnDistributionLength; i++){
    if(drawnDistribution[i] != -1){
      noInput = false;
      firstInputPosition = i;
    }
  }

  //if no input, we set it all points to the 50% mark and then end
  if(noInput){
    for(int i = 0; i < drawnDistributionLength; i++){
      drawnDistribution[i] = 50;
    }
    //draw 50% mark and make firstDraw on draw screen false
    tft.fillRect(2,drawnLeftBound,screenWidth-2-2,drawnRightBound-drawnLeftBound,WHITE);
    tft.fillRect(screenWidth/2,drawnLeftBound,3,drawnRightBound-drawnLeftBound,BLUE);
    firstDraw = false;

    return;
  }

  //Otherwise, we fill in the gaps 
  for(int i = 1; i < drawnDistributionLength; i++){
    currentPosition = firstInputPosition + i; //we get all of the positions after the first input

    //we make this circular so that we reach entire array
    if(currentPosition >= drawnDistributionLength){
      currentPosition = currentPosition - drawnDistributionLength;
    }

    //if a point isn't drawn yet, we use the last drawn in point 
    if(drawnDistribution[currentPosition] == -1){

      //copy from last known point and maintain circularity
      if(currentPosition == 0){
        drawnDistribution[currentPosition] = drawnDistribution[drawnDistributionLength-1];
      } else {
        drawnDistribution[currentPosition] = drawnDistribution[currentPosition-1];
      }

      //Now, we update the image with new drawing at that location
      if((drawnLeftBound+currentPosition) <= (drawnRightBound-1)){
        tft.drawRect(((float) drawnDistribution[currentPosition])/100*(screenWidth),drawnLeftBound+currentPosition,3,1,BLUE);
      }
    }
  }
}

//Start of ForceDistribution Functions
ForceDistribution::ForceDistribution(int* setDistributionInput = nullptr, int arraySize = 0, float maxVoltInput = MINVOLT + (MAXVOLT-MINVOLT)/2, int setTimeInput = MINTIME + (MAXTIME-MINTIME)/2){
  //here, we set the max voltage and time, and if there is any input for the distribution, we set that too
  setMaxVoltage(maxVoltInput);
  setTimePerInterval(setTimeInput);

  if((setDistributionInput != nullptr) && (arraySize > 0)){
    setDistribution(setDistributionInput, arraySize);
  }

}

ForceDistribution::~ForceDistribution(){
  reset(); //just call for reset function
}

bool ForceDistribution::setMaxVoltage(float inputVoltage){
  //return false if not in range
  if((inputVoltage > MAXVOLT) || (inputVoltage < 0.0)){
    return false;
  }
  //set new voltage and return true
  maxVoltage = inputVoltage;
  return true;
}

bool ForceDistribution::setTimePerInterval(int newTime){
  //reject if not in range
  if((newTime < 0) || (newTime > MAXTIME)){
    return false;
  }
  // update new time and return true
  timePerInterval = newTime;
  return true;
}

void ForceDistribution::reset(){
  //delete the two arrays if not nullptr  
  if(forceArray != nullptr){
    delete[] forceArray;
    forceArray = nullptr;
  }

  if(timeArray != nullptr){
    delete[] timeArray;
    timeArray = nullptr;
  }

  //reset all variables to the default
  arrSize = 0;
  maxVoltage = MINVOLT + (MAXVOLT-MINVOLT)/2;
  timePerInterval = MINTIME + (MAXTIME-MINTIME)/2;
  startTime = 0;
  startRun = true;
  currentForce = 0;
}

bool ForceDistribution::setDistribution(int* inputDistribution, int arraySize){
  int backStoppingCount = 0;   //we count the number of backstoppings needed
  int nextElement = 0;  //next element for backstopping loop
  int nextElementInput = 0; //Next element for inputArray
  int inputIncrement = 0; //increment for the inputDistribution array (which can be different size than the force/time arrays)
  
  //check to make sure the input array is proper in magnitude and check for needed backStopping
  for(int i=0; i < arraySize; i++){
    if((inputDistribution[i] < 0) || (inputDistribution[i] > 100)){
      return false;
    }
    //next element (in a wrap around array)
    nextElement = (i+1) % arraySize;

    // If a difference of 3V or more, we will have a backstop inbetween
    if((((float) inputDistribution[nextElement] - inputDistribution[i])/100) < BACKSTOPVOLTAGE){
      backStoppingCount++;
    }
  }

  //if previous arrays are non-zero, we delete them
  if(forceArray != nullptr){
    delete[] forceArray;
    forceArray = nullptr;
  }
  if(timeArray != nullptr){
    delete[] timeArray;
    timeArray = nullptr;
  }

  //force/time array size is the inputDistribution size plus the backstopping
  arrSize = arraySize + backStoppingCount;

  //declare time interval array and distribution array of arrSize
  forceArray = new int[arrSize];
  timeArray = new int[arrSize];

  // first, fill array with default distribution values and check for possible backstops
  for(int i=0; i<arrSize; i++){
    nextElementInput = (inputIncrement+1) % arraySize;

    //for inputDistribution-defined forcing, we get forcing from inputDistribution and set to default time interval
    forceArray[i] = inputDistribution[inputIncrement];
    timeArray[i] = timePerInterval;
    
    //if next voltage jump is greater than 60% of the maximum voltage, then make forcing the negative maximum and give custom time interval
    if((((float) inputDistribution[nextElementInput] - inputDistribution[inputIncrement])/100) < BACKSTOPVOLTAGE){
      i++;

      forceArray[i] = -100;
      //This time interval is set to the defined backstopping time
      timeArray[i] = BACKSTOPPINGTIME;
    }
    
    //update inputDistribution incrementer
    inputIncrement++;
  }

  return true;
}

bool ForceDistribution::restartForcing(){
  //reset startRun variable
  startRun = true;
  return true;
}

bool ForceDistribution::startForcing(){
  //This outputs the force distribution  in the forceArray to the ERM motor.
  // This works by holding each element with time variable (in ms)
  //This function also includes calculated backstopping for big changes in voltage.
  //This function returns true if it sucessfully outputs the distrubution and false otherwise

  //If either array isn't set up, we can't force the motor
  if((forceArray == nullptr) || (timeArray == nullptr)){
    return false;
  }

  //This establishes the current time
  unsigned long currentTime = 0;

  //if we restart, we set start time to current time
  if(startRun){
    startTime = millis();
    currentTime = startTime;
    startRun = false;
    // we also set currentForce to zero to make it start from the first element
    currentForce = 0;
  } else {
    //get current time
    currentTime = millis();
  }

  //If the time since the previous switch is greater or equal to the interval length, we go to next interval
  if((currentTime - startTime) >= timeArray[currentForce]){
    currentForce = (currentForce+1)%arrSize;
    startTime = millis();
  }

  //Now output this to the setVoltage
  setVoltage(maxVoltage*( (float) forceArray[currentForce])/100);
}

void ForceDistribution::printList(){
  //This prints the force and time array with labels above
  // It checks that they aren't nullptr, then prints the list horizontally
  
  Serial.println("forceArray below:");
  
  if(forceArray != nullptr){
    for(int i=0; i<arrSize; i++){
    Serial.print(forceArray[i]);
    Serial.print(" ");
  }
    Serial.println(" ");
  }
  
  Serial.println("timeArray below:");
  if(timeArray != nullptr){
    for(int i=0; i<arrSize; i++){
    Serial.print(timeArray[i]);
    Serial.print(" ");
  }

  Serial.println(" ");
  }
}

//Execution of program
void setup() {
  Serial.begin(9600); //setup serial bus for printing

  //Pin setup for output pins
  pinMode(posPWMPin,OUTPUT);
  pinMode(negPWMPin,OUTPUT);

  //Set to low by default
  digitalWrite(negPWMPin,LOW);
  digitalWrite(posPWMPin,LOW);  

  screenSetup(); //Initial screen setup here

  //give task queue it's functions
  taskQueue[0] = updateScreen;
  taskQueue[1] = updateTouch;
}

void loop() {
  //loop through task queue for real time function
  for(int i=0; i<taskQueueLength; i++){
    (*taskQueue[i])();
  }
}
