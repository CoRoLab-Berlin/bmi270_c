# BMI270 - C Implementation (I2C) - Version: 0.5

Bare bones BMI270 - C implementation (I2C). This was a project for my practical phase at my University and my first time working hands on with IMUs.

## Usage
Make sure these lines are connected: GND, 3V3, SDA, SCL

If you are using the SparkFun SPX-17353 BMI270 Breakout Board, you can easily connect them using a 1mm 4-pin JST connector cable. (example: Qwiic cables)

Make sure that the device is available at `0x68` or `0x69` i2c address by running this command:

`i2cdetect -y 1`

The BMI270 requires a config load. This initialization step is necessary to be able to use all its functions.

`bmi270_init(&sensor)`

A full power cycle is necessary if you want to load the config again.

Check out the [main.c](https://github.com/CoRoLab-Berlin/bmi270_python/tree/main/examples) for more usage information.

## Tested with:
- Ubuntu 22.04.2 LTS
- Raspbian 10 - Buster (32 Bit)

## Functionality

- BMI270 integration
- load config file into BMI270
- write/read registers
- a few other functions (check bmi270.h)

## Python Version

The BMI270 Python Implementation is also [available as a package on pypi.org](https://pypi.org/project/bmi270).

You can install this package using this command:

`pip3 install bmi270`

For a Raspberry Pi Setup add/change this line in /boot/config.txt to your desired baudrate:

`dtparam=i2c_baudrate=400000`

Reboot your Raspberry Pi after applying the change.

Check the [README](https://github.com/CoRoLab-Berlin/bmi270_python) for more information.