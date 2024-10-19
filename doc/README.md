# About

The Katana project is a Wi-Fi penetration software project that uses the evil-twin attack method for phishing Wi-Fi passwords. The project requires a single ESP32 and a laptop/computer to facilitate the evil-twin attack. In theory any devices that can read and write serial commands to the ESP32 device can facilitate evil-twin attack. This includes any devices with operating systems like Android, IOS, Windows, Linux. It can also work with Arduino assumming that it has the necessary algorithm and serial codes for converting user input into serial commands that can be executed by ESP32 as well as reading serial events from ESP32.

## Disclaimer

Usage of all tools, procedure, information and instruction on this project for attacking targets without prior mutual consent is illegal. It is the end user’s responsibility to obey all applicable local, state, and federal laws. I assume no liability and are not responsible for any misuse or damage caused by this project or software.

## Use Case Diagram

![Katana](/doc/images/Use-case.png)

The use case diagram illustrates what a user can do to the system. This includes things like scanning for target access points, selecting target access points, launching phishing site and disconnecting connected clients from target access points.

## Workflow

![Katana](/doc/images/Workflow.png)

The workflow illustrates the recommended flow of the attack using flowchart. It is a high level overview of how an evil-twin attack should be conducted. Although not every attack is guranteed to follow the workflow because an attack varies depending on the situation such as the strength of the Wi-Fi signal, the behaviour of the client target and human error.

The success of the attack depends on a lot of factor:

1. Is the target access point vulnerable to MIC and PMKID key monitor?
   - This includes capturing the 2nd or the 1st authentication message from 4-way handshake. Without the authentication message, a password cannot be determined if it is correct or wrong in stage 2.
2. Are there any active clients connected in the target access point? Is anyone of them vulnerable to deauthentication attack?
   - Connected clients to target access point is also important because afterall the goal is to trick a connected client to give the Wi-Fi password using a phishing website. Active clients are clients that are actively using the Wi-Fi such as browsing the internet, watching TikToks, and other ways.
   - If we can’t deauthenticate/disconnect a client from their access point then it would be pointless to conduct the attack. The client has to be disconnected from their access point so that it joins to our clone access point instead (The access point we control).
3. Now that the client is disconnected from its access point. Did the client attempted to connect to our access point? Are we able to launch the captive portal in its device?
   - The moment a target client is disconnect from its access point and connects to our access point, we have successfully tricked the client into connecting to our access point. If the client doesn’t connect, it could be that it is not an active client or is smart enough to know that it is an attack.
   - Now that the client is connected, a captive portal will be shown on the client’s device. This is possible because we can control the DNS server in our access point. But different device responds differently to our DNS server. So this is also another factor that contributes to success of an attack.
   1. Now that the captive portal has launched in the client’s device, we wait for the password to be entered and see if we can trick the client into giving in the password.

## Workflow Procedure

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

The system is not guranteed to be perfect and bugs is inevitable. Incase an unknown state or event has happened, consider doing a system restart on ESP32 by running the command restart or you can just press the "EN" button on the ESP32 board. Restarting the system would not take long and doing a system restart for every attack clears memory state and refreshes the internal state of the system.

## Input Format for ESP32

The input for the ESP32 is a sequence of hexadecimal string in the following order:
| **NAME** | **ALLOCATION** | **DESCRIPTION** |
| --- | --- | --- |
| 1. Mode | 2 bytes | The mode of the operation (e.g. Deauth Mode, Access Point Start Mode, Scan Mode ) |
| 2. Key Type | 2 bytes | Tells ESP32 to capture a PMKID key type (Defaults to 00 for MIC) |
| 3. Channel | 2 bytes | Channel of the target access point (1 - 14) |
| 4. AP Name Len | 2 bytes | The length of the name of target access point |
| 5. BSSID | 12 bytes | Mac Address of target access point |
| 6. AP Name | 64 bytes (Max) | The name of target access point |

- **The sequence is commonly use for starting an access point, key sniffing or deauthentication**
- For example to initiate deauthentication, the input should be in the following order:
  - _Mode→Key Type→Channel→AP Name Len→BSSID_
- For scanning, the input should be in the following order:
  - _Mode_

### Input Usage and Example

Different possible input to ESP32
| **MODE** | **CODE** | **EXAMPLE** | **DESCRIPTION** |
| --- | --- | --- | --- |
| Null | 00 | 00 | No operation |
| Deauth Start | 01 | 01-00-07-00-331122ABCDEF | Start deauthentication of target access point |
| Deauth Stop | 02 | 02 | Stop deauthentication of target access point |
| AP Start | 03 | 03-00-07-09-331122ABCDEF-544553545F57494649 | Create a clone of target access point to initiate phishing attack |
| AP Stop | 04 | 04 | Stop the access point |
| Sniff Start | 05 | 05-01-06-11-331122ABCDEF-544553545F57494649 | Start monitor mode to intercept key data (PMKID/MIC) from target access point |
| Sniff Stop | 06 | 06 | Stop monitor mode |
| Scan | 07 | 07 | Scan for target access point |
| Restart | 0B | 0B | Restarts the ESP32 system |
| Client Target | 0C | 0C-00-00-00-00-331122ABCDEF | Set the MAC address of client as target for deauthentication |

#### Raw Input Example

05000611331122ABCDEF544553545F57494649
<br>
A raw input is a sequence of combination of instruction code and data

#### Breakdown of Example Input

- **Deauthentication:** _01-00-07-00-331122ABCDEF_
  - _01_ - Deauthentication mode (ESP32 will broadcast a deauthentication frame to clients of target access point)
  - _00_ - Tells ESP32 to capture a PMKID key type (Defaults to _00_ for MIC)
  - _07_ - Channel of target access point
  - _00_ - The length of the name of target access point (_09_ is a hexadecimal string when converted into integer will be equal to _9_)
  - _331122ABCDEF_- Mac address of target access point
- **Access Point Control:** _03-00-07-09-331122ABCDEF-544553545F57494649_
  - _03_ - Access point mode (ESP32 will create it’s own access point)
  - _00_ - Tells ESP32 to capture a PMKID key type (Defaults to _00_ for MIC)
  - _07_ - Channel of target access point
  - _09_ - The length of the name of target access point (09 is a hexadecimal string when converted into integer will be equal to 9)
  - _331122ABCDEF -_ Mac address of target access point
  - _544553545F57494649 -_ The name of the target access point encoded in hexadecimal string
- **Monitor Mode:** _05-01-06-11-331122ABCDEF-544553545F57494649_
  - _05_ - Monitor mode (ESP32 will listen for key data being transmitted between clients and target access point)
  - _01_ - Tells ESP32 to capture a PMKID key type (Defaults to _00_ for MIC)
  - _06 -_ Channel of target access point
  - _11 -_ The length of the name of target access point
  - _331122ABCDEF-_ Mac address of target access point
  - _544553545F57494649-_ The name of the target access point encoded in hexadecimal string
- **Scan Mode:** _07_
  - _07_ - Scan mode tells ESP32 to scan or look for nearby access points. The scanner outputs the access point’s channel, signal strength, mac addres, length of the name and name

## Output Format of ESP32

Different possible expected event output from ESP32

| **CONTEXT** | **COMPONENT**           | **EVENT**                                                                   | **OUTPUT**                                                                                                                                                                                                                         |
| ----------- | ----------------------- | --------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ESP_STARTED | main.c                  |                                                                             |                                                                                                                                                                                                                                    |
| CMD_PARSER  | cmd_parser.c            | 1.CURRENT_CMD 2.PARSED_CMD 3.INITIALIZED                                    | 1.Current command received by ESP32 2.Mode, channel and AP name length 3.\*NULL\*                                                                                                                                                  |
| WIFI_CTL    | wifi_ctl.c              | 1.FOUND_APS 2.SCAN 3.ON_DEVICE_CONNECT 4.ON_DEVICE_DISCONNECT 5.INITIALIZED | 1.The number of nearby access points 2.Mac address, channel and signal strength of a nearby access point 3.Mac address of a device connected to ESP32’s AP 4.Mac address of a device that disconnected from ESP32’s AP 5. \*NULL\* |
| EXPLOITS    | exploits.c              | 1.DEAUTH_STARTING 2.DEAUTH_TARGET 3.DNS_HIJACK_STARTED                      | 1.\*NULL\* 2.Mac address of target access point 3.\*NULL\*                                                                                                                                                                         |
| PMKID       | frame_eapol_validator.c | 1.WRONG_KEY_TYPE 2.WRONG OUI 3.WRONG_KDE                                    | 1.Key type value 2.OUI value 3.KDE value                                                                                                                                                                                           |
| PMKID       | frame_output.c          | 1.MSG_1                                                                     | 1.PMKID of target access point                                                                                                                                                                                                     |
| MIC         | frame_output.c          | 1.MSG_1 2.MSG_2                                                             | 1.Mac address of target access point, mac address of station and anonce 2.Key data including snonce, MIC and etc.                                                                                                                  |
| WEB         | web_server.c            | 1.POST_PSK 2.POST_INVALID_PSK 3.GET_ROOT 4.SERVER_STOPPED 5.SERVER_STARTED  | 1.Password entered by the client 2.The password entered by the client is null or not a string 3.Client request web client 4.The web server component stopped 5.The web server component started                                    |
