


 # Electronics for a Custom Force Massage Gun
> For those who have frequent muscle tension, a massage gun can relieve such tension. While it is common for massage guns to have different modes of vibration (constant, pulse, etc.), none that the author knows of allow for any custom function to be forced. The purpose of this project is to make the electronics of a massage gun that can accept any drawn function from the user and output it as vibration. This project uses an Arduino Uno R3 to control an eccentric rotating mass (ERM) motor and LCD touchscreen. The user can draw a custom forcing function, change the maximum amplitude, change the frequency, and start/stop the forcing with the LCD touchscreen. The two logical outputs of the Arduino Uno go to an H-bridge circuit to control the direction and intensity (via pulse width modulation) of the motor. This README document outlines the electronics and software of the project and contains links for the hardware components.
## Video of Full Function
The video link shows the full function of the project. 
The Youtube link is here: https://www.youtube.com/watch?v=qZPgrXRUjcU
## Electronics
### LCD Touchscreen
> There are two sections to the electronics: the LCD screen and the motor circuit. First, the LCD screen is placed on top of a shield expansion board which is on the Arduino Uno R3 itself. This expansion board is needed since the LCD touchscreen uses almost all of the pins and 4 other pins (5V, ground, pin 10, pin 11) need to be accessed for the motor circuit. It should be noted that pin 10 and 11 typically control the SD card slot for the LCD touchscreen, but since we aren't utilizing it, we reuse them for motor control. A picture is below of the LCD screen from a top down view.

![LCD Touchscreen Circuit](https://cdn.discordapp.com/attachments/1332078492183040156/1332084001015337062/20250123_135637.jpg?ex=6793f766&is=6792a5e6&hm=be95d851b3424a66a8274ef85ca8252ac952d5def538fb82b5b3d8fb6558d5cc&)

> The Amazon links for the specific parts are below:
 1. Arduino Uno R3: https://tinyurl.com/3auh9cnw
 2. Expansion Board: https://tinyurl.com/2pckve6t
 3. LCD Touchscreen: https://tinyurl.com/3fpetmz3
 4. 9V Battery to Power Connector: https://tinyurl.com/3tvfwbav

### Motor Circuit
> The motor circuit uses an L293D quad half-H driver chip to create an H-bridge circuit for the motor. Besides the chip itself, it also uses 4 Schottky diodes for dissipating the kickback voltage of the motor. From the Arduino, it takes a 5V input, ground, as well as logical input from pin 10 and 11 from the Arduino Uno. Last, it takes a 9V input for the motor voltage via a 9V battery. 

> The chip description on the Texas Instruments website shows the circuit diagram in question.  The link to the description is here: https://www.ti.com/product/L293D#description
The page also contains a link to the datasheet.
> 
![L293D Description Example Image](https://www.ti.com/ds_dgm/images/fbd_slrs008d.gif)


> The image above shows various circuits that could be made with the chip. We implement the circuit on the left hand side which can control the ERM motor forward and backward. In the image, V_CC2 is our 9V source and V_CC1 is our 5V source. Pin 10 and 11 from the Arduino Uno control pin 2 and 7 on the chip. It doesn't matter if the which Arduino pin connects to a given L293D pin since our motor used is bidirectional. This bidirectionality is used to have motor backstopping. This backstopping allows a "crisp" drop in voltage to occur without any wind down of motor when it goes from a much higher to a much lower voltage. 
> The picture below shows the circuit implemented on the right half of the chip. The left positive rail is 9 volts and the right positive rail is 5 volts. The ground is shared between them.  The yellow wires are the power wires from the chip to the motor wires, the orange are the logical outputs of pin 10/11, and the red/green wires are for positive voltage/ground respectively. 

![Breadboard Motor Circuit Image](https://cdn.discordapp.com/attachments/1332078492183040156/1332082963562758304/20250123_141447.jpg?ex=6793f66f&is=6792a4ef&hm=bc6fa14f2029ee736a37472434fdd386743b36e47774dbc22505a8875df98f89&)

The Amazon links for the parts featured are below:

 1. L293D Chip: https://tinyurl.com/f6f5cyc7
 2. Schottky Diodes: https://tinyurl.com/56nj4rhk
 3. 9V to Pin connector: https://tinyurl.com/4vcyejb7
 4. Small Breadboard: https://tinyurl.com/5aubn9hd

## Software
### Overview of User Interface
> From the video and the image below, the user interface has two sliders for amplitude and frequency, a delete button, a save button, a start/stop button, and a function drawing area. The user can increase/decrease the amplitude and frequency, as well as draw/re-draw the custom forcing function when the motor is off. **It should be noted that none of the changes are kept until the "save" button is pressed.** Note: if the x axis doesn't have all points drawn, it will extend the previous point (i.e. keeping the forcing held flat). If there are no points drawn when "save" is hit, it draws a uniform function at the 50% line. 

![Motor Off Image of UI](https://cdn.discordapp.com/attachments/1332078492183040156/1332081995232182323/20250123_140231.jpg?ex=6793f588&is=6792a408&hm=bccea8dd4c892b58e71e2cb4ac8fa2f838db15983a7894c12daf109917b75cd2&)

> After that, hitting the start button will output the forcing function. When the motor is on, no data can be changed. The save/delete buttons change their color to gray to denote their inability to be pressed. Likewise, the drawing area and sliders don't move when pressed.

![Motor On Image of UI](https://cdn.discordapp.com/attachments/1332078492183040156/1332082530832355420/20250123_140950.jpg?ex=6793f608&is=6792a488&hm=c7d89ae9714f9e1d9f0113d84b89056a61e92c203e06a7505b7417e9728f52b3&)

> When the motor is off and the delete button is pressed, it will reset the entire screen to default settings. The entire drawn area is wiped and the slider are set back the the middle. If one tries to press start with nothing "saved," then nothing will output.

### Arduino Code Overview
> The Arduino sketch has a header file and main ".ino" file. The latter contains the function definitions and program execution. For the real time functionality, a task queue is used to repeated call it's functions in the "loop()" function. It only contains two functions call "updateScreen()" and "updateTouch()." As their names would imply, the "updateTouch()" checks the input of the touchscreen for a button/draw press to then call other functions and update variables. The "updateScreen()" function specifically updates the screen and pin 10/11 output via variables changed by "updateTouch()." For example, if a user presses the start button, the "updateTouch()" will recognize that and change the boolean "motorOn" variable to "true." Then, "updateScreen()" will recognize the "motorOn" as "true," update the screen, and start forcing the motor via the given distribution. The rest of the functions that aren't in the "ForceDistribution" class are helper functions for "updateScreen()" and "updateTouch()."

> The *drawn distribution*, and it's frequency/amplitude, are kept separate from the *forcing distribution.* Since the motor uses backstopping to stop the motor, the forcing distribution isn't the same as the drawn distribution. Only when the saved button is pressed is it transferred over to the "ForceDistribution" object. The "ForceDistribution" class exists to keep the variables/functions of the LCD touchscreen separate from the variables/functions of the force distribution. This separation exists for organization's sake and keeping the drawn distribution before the save separate from the one after the save.  
> Within the "ForceDistribution" class, the forceArray and timeArray, which are the arrays for the force distribution and time (in milliseconds) per element, are dynamically allocated since we don't know the number of backstopping events that need to occur beforehand. This changes the length of the array based upon the user's input.
> All functions, whether as "ForceDistribution" member functions or as stand alone functions, have a description in the header file stating what they do.

## Credit
> For this project, some code used is from the open source Adafruit Library called "Elegoo_TFTLCD". The comment below is required of them for any redistribution. Link to original Github repository is here: https://github.com/Erutan409/Elegoo_TFTLCD

This is a library for the Adafruit 2.8" TFT display.

This library works with the Adafruit 2.8" TFT Breakout w/SD card
  
----> [http://www.adafruit.com/products/335](http://www.adafruit.com/products/335)

as well as Adafruit TFT Touch Shield
  
----> [http://www.adafruit.com/products/376](http://www.adafruit.com/products/376)
 



Check out the links above for our tutorials and wiring diagrams.

These displays use 8-bit parallel to communicate, 12 or 13 pins are required
to interface (RST is optional).

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!



Written by Limor Fried/Ladyada for Adafruit Industries.
MIT license, all text above must be included in any redistribution
