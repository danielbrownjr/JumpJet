# About
This is the firmware for my integrated chamber heater for the Prusa Core One(+). You can find the main project on printables:
https://www.printables.com/model/1696936-jetpack-chamber-heater-for-the-core-one/

# Features
- Nicely integrated into the corner of the printer, so that it fits without major chamber modifications.
- Fully automated with PrusaLink API
- Multiple layers of safety thought into the design

# Hardware characterization
Standalone CZ4060 heater characterization has been completed at 24 V, including startup, 60-minute steady-state, and fan-assisted cooldown testing. See [`docs/hardware/cz4060-characterization.md`](docs/hardware/cz4060-characterization.md) for measured current, power, and thermal data. Full Jump Jet power-path and installed-system validation remain in progress.

# Disclaimer: Read this before you proceed:
I have done my best to ensure the safety of this project. However, I hold no special qualifications with regard to any part of this project. Therefore, you should not consider my advice reliable. If you are not able to judge whether my safety suggestions are adequate, then please consult with someone knowledgeable before you proceed with this. Working with high-powered electronics such as a 200W heater can be dangerous, both for people and for property. By proceeding with this project, you agree to assume all responsibility for any potential damage that may follow. I do my best to further your safety, but in the ends, it is in your hands.

# Instructions
This GitHub contains the instructions for how to handle the firmware for the ESP32 used in the chamber heater project. All other aspects of the project, including the bill of material, is covered on the Printables page which I encourage you to vist.

# Compatibility
The firmware was written for a ESP32-S3 SuperMini Devboard. I believe it will work with any other ESP32-S3 dev board, provided that you adapt the pins accordingly. While the project does rely on the wifi of the ESP32-S3 chip, it probably does not have to be an S3 series chip. I imagine that many other types of ESP32 chips will also handle the code well, but please know that I have only tested with the S3.

# API access
The ESP32 responds to commands sent via wifi. These commands are not necessary to operate the heater, but they are useful to collect telemetry and take manual control for testing. Assuming a Linux terminal, the commands are the following:

Note: Replace ESP32IP with the ip address of your ESP32 on the local network. You can find this e.g. on your router.
- `GET ESP32IP/setTtarget?value=XX`: Sets the target temperature to XX. This only has an effect in manual mode
- `GET ESP32IP/modeManual`: Set the ESP32 into manual mode. This disables automatic choice of target temperature and printer checks, so that you can manually set a target temperature for testing. Keep in mind that in this mode the heater will remain on indefinitely, unless and error state is triggered. Use with caution.
- `GET ESP32IP/modeAuto`: Set the ESP32 into auto mode. This is the default mode where the ESP32 reads printer telemetry to decide on it's target state. The ESP32 resets to this mode whenever it is powered on.
- `GET ESP32IP/status`: Returns telemetry in a JSON format. This contains most interesting parameter in a single data dump. I suggest to use the syntax `GET ESP32IP/status | jq` to format the output or `GET ESP32IP/status | jq '.sensors.Tcase'` to select a specific varible from the JSON.
- `GET ESP32IP/abort`: Emergency command to set the heater into a state where it will no longer try to heat. In this state, the heater is disabled and the fan is set to max, indefinitely. This is also triggered automatically if the sensors suggest that the heater fan might have failed.
- `GET ESP32IP/unabort`: Revokes an aborted state and returns the ESP32 to normal operation.

# Preparing and flashing the code
The code is written in the Arduino language, so the standard way to work with it is to download the Arduino IDE and then use that both to adapt the code and to flash it to your physical ESP32 chip. The step-by-step instructions are as follows:
1) Download and install the Arduino IDE either from your preferred software repository or from https://www.arduino.cc/en/software/
2) You will need to add ESP32 support to Arduino IDE, which you can do by following the instructions here: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
3) Choose the ESP32-S3 Dev board (or whatever fits your board, if you went with something other than an S3)
4) Additionally, I had to set USB CDC On Boot → Enabled in the tools menu before the IDE discovered the ESP32
5) Now download the github repo here to a folder of your choice. Notice that the .ino file most be placed in a folder of the same name, just like here.
6) Open the .ino file in Arduino IDE
7) Preparing the code: The takes both your wifi password, wifi SSID, Printer IP, and the PrusaLink API key as hardcoded constants in the code. Therefore, before you flash the code, please find the labels "WIFI SSID HERE", "WIFI PASSWORD HERE", "PRUSALINK API KEY OF YOUR PRINTER HERE", and "PRINTER LOCAL IP HERE", and replace each with the corresponding item.
  - If you have not already done this, I suggest you log in to your wifi router and set a static IP for your printer so that it doesn't suddenly change. You can also find the printer IP on the router.
  - You can find the PrusaLink API key either on the printer or on PrusaConnect. In either case, I think it first has to be enabled as on option in the printer settings on the printer.
  - Note that the ESP32 might have issues with 5 GHz wifi. Choose a 2.4 GHz network if available. ( I don't know if this is a real problem)
8) Prepare for ESP32 serial communication by opening the serial monitor (looking glass symbol, or the shortcut crtl+shift+m). Choose baudrate 115200 to match the board / code. 
8) Flashing the firmware: First time you flash your ESP32, you may have to hold down the boot key on the dev board. Therefore, while continuing to hold down the boot key on the board, connect it to your computer via USB. While still holding on the boot key, click on the upload button (forward arrow) in the Arduino IDE interface. For me, the shortcut was Crtl + U. Wait for the process to complete, and then release the key.
9) Probably, you will have to reboot the ESP32 by e.g., disconnecting the USB and then plugging it again. But then, you should now be receiving telemetry from the board in the serial monitor.
10) Return to your router and set a static IP also for the ESP32. This will be useful later to communicate via the REST API for manual control and telemetry.

If you encountered no showstopping problems, then your ESP32 should now be ready for use in the heater unit!
