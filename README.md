# About

The Katana project is a Wi-Fi penetration software project that uses the evil-twin attack method for phishing Wi-Fi passwords. The project requires a single ESP32 and a laptop/computer to facilitate the evil-twin attack. In theory any devices that can read and write serial commands to the ESP32 device can facilitate evil-twin attack.

![Katana](/doc/images/Katana.png)

This is an old project of mine that I would like to share, feel free to use every asset in this project. **I would not be able to actively contribute to this project because I will be focusing on my new niche hobby (Data Science and Artificial Intelligence)**. If you want to contribute, I suggest to fork this repository and I will put the link of your repository here. For questions on how the system works, you can contact me at telegram: @hexx2

## Disclaimer

Usage of all tools, procedure, information and instruction on this project for attacking targets without prior mutual consent is illegal. It is the end user’s responsibility to obey all applicable local, state, and federal laws. I assume no liability and are not responsible for any misuse or damage caused by this project or software.

## Installation and setup

1. Install Python and pyserial
   - In order for this to work, you need to have python installed and install the library pyserial by running the command `pip install pyserial==3.5`
2. Install CP210x USB to UART Bridge VCP Drivers
   - Install the driver for ESP32 which can be downloaded here: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
3. Connect ESP32 to your laptop/computer then in your computer search for "Device Manager" and click on "Ports (COM & LPT)". You should be able to see something like "Silicon Labs CP210x USB to UART Bridge (COM4)". Remember what is inside the parentheses which is "COM4"

![CP210x driver](/doc/images/CP210x-driver.png)

4. Install and flash the program into the ESP32
   - Install esptool using pip by running the command `pip install esptool`
   - **In your terminal**, run the command `python -m esptool -p COM4 -b 460800 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 bootloader.bin 0x10000 Awps.bin 0x8000 partition-table.bin`
   - Don't forget to replace `COM4` in `python -m esptool -p COM4` with what is indicated in the device manager
5. Change directory into `serial_com` and run the command `python main.py COM4 info`. You should be able to see the message "ESP32 system started...", otherwise restart ESP32 by pressing the "EN" button. You can also run the script as `python main.py COM4 verbose` which outputs more information in the console.

## How to use

It is important to follow this step-by-step procedure:

1. Scan for devices
   - Command: `scan`
   - Scan for nearby access points
2. Show nearby access points
   - Command: `show`
   - Show the list of nearby nearby access points after scanning
3. Select target
   - Command: `select [INDEX]` where INDEX is the ID of the acess point from the list
   - Selects target access point from the list
4. Pre-exploitation (Stage 1): Intercept key exchanges
   - Command:
     - `preexploit pmkid`
     - `preexploit mic`
   - Intercept key exchanges between clients from target access point, this enables monitor mode in the ESP32
5. Pre-exploitation (Stage 1): Show client
   - Command: `showclient`
   - Show the list of vulnerable clients connected to the target access point
6. Pre-exploitation (Stage 1): Select client target
   - Command: `selclient [INDEX]` where INDEX is the ID of the client from the list
   - Selects the target client from the target access point
7. Exploitation (Stage 2): Clone target access point
   - Command: `exploit ap`
   - Creates a clone version of the target access point
8. Exploitation (Stage 2): Disconnect client
   - Command: `exploit deauth`
   - Disconnects the target client from their access point so it connects to our access point instead
   - Once the client has connected to our access point, we wait for the password to be entered

The system is not guranteed to be perfect and bugs is inevitable. Incase an unknown state or event has happened, consider doing a system restart on ESP32 by running the command `restart` or you can just press the "EN" button on the ESP32 board. Restarting the system would not take long and doing a system restart for every attack clears memory state and refreshes the internal state of the system.
