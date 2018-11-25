# orid
A physical screen orientation detection solution for linux.

![orid in action](https://raw.githubusercontent.com/mrpossoms/orid/master/.demo.gif)

## How does it work?

Sorid is two parts software, and one part hardware. orid uses a small, very simple device attached to the back of your monitor to determine the physical orientation, and software (orid) running on your computer to listen to it. If the device says the orientation has changed, orid will invoke xrandr, and change the orientation.

## How do I use it?

To get started, you'll need the following prerequsites.
* A PC running linux with the X windowing system
* gcc
* Arduino IDE
* GNU Make
* [Adafruit GEMMA M0](https://www.adafruit.com/product/3501?gclid=EAIaIQobChMIlO-QtLPu3gIVHLbACh3wqQCeEAYYASABEgKh7fD_BwE)
* [mpu 6050 based IMU](http://a.co/d/7XmvxoI)

### Assembling the sensor

Building the sensor is totally up to you. Essentially, you just need and Arduino compatible board, and an IMU based on the MPU-6050 chip. However, I'll offer a run through of how I did it if you're so inclined to follow my design.

![GEMMA and 6050](https://raw.githubusercontent.com/mrpossoms/orid/master/.parts.jpg)

First and formost, get your hands on an Adafruit GEMMA M0 and a HiLetGo GY-521, links to both are listed above.

![Stand off example](https://raw.githubusercontent.com/mrpossoms/orid/master/.stand-off.jpg)

Next solder some clipped leads onto the SDA and SCL contacts of the IMU. These will help to make connection with the contacts on the GEMMA which are a bit more spread out.

![Assembled](https://raw.githubusercontent.com/mrpossoms/orid/master/.assembly.jpg)

Solder the SCL pin to the D2 contact of the GEMMA, and the SDA pin to the D0 contact. Then solder a wire connecting the GND of the IMU to the GND of the GEMMA. Finally, solder the VCC of the IMU to the 3Vo of the GEMMA. Next, connect the GEMMA to your computer. Follow [this](https://learn.adafruit.com/adafruit-gemma-m0/arduino-ide-setup) tutorial to get your Arduino IDE setup to program the GEMMA. Then load the sketch from this repo (in orientation-sensor) and flash it to the GEMMA.

### Compile and install orid

Assuming you have gcc and GNU Make installed, this part will be easy. Open a terminal navigate to this repository, then run.

```bash
$ sudo make install
```

That will compile and install orid at `/usr/bin/`.

### Configuring orid

The first time your run orid it will create a directory .orid in your $HOME. Within that directory there will be a number of sub-directories named 0-N where N is the number of monitors you have connected. Within each of these directories are a number of configuration files, each of which contains a single value. The only one you may need to change is named `serial-device` which contains a path to the serial file for a particular connected GEMMA. orid guesses the serial file names, so these will likely need to be changed by hand. Asside from `serial-device` there are 6 files within the sub-directory `rotation`. Each one of these states how the screen should be rotated when each axis and its particular sign are pointing down. By default they are all 'normal' however you will want to change the appropriate ones to 'left' and 'right' where appropriate. Look at the orientation of the axes on the IMU to tell which ones need to be modified based on the sensors orientation on your monitor.

### Add it as a startup program

On an Ubuntu system you can easily configure orid to run at your login. Simply press the 'windows' key, then type 'startup' select the 'Startup Applications' option. When the Startup Applications window is presented click 'Add' then fill in the following.

[Startup config](https://raw.githubusercontent.com/mrpossoms/orid/master/.startup.png)

Finally press 'Add', then you're finished.
