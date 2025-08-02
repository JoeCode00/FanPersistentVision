import socket
import time
from datetime import datetime
import os
import numpy as np

def read_h_constants(filepath):
    constants = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('#define'):
                parts = line.split()
                if len(parts) >= 3:
                    name = parts[1]
                    value = parts[2].strip('"')
                    constants[name] = value
                    try:
                        constants[name] = float(value)
                        try:
                            constants[name] = int(value)
                        except ValueError:
                            continue
                    except ValueError:
                        continue
    return constants

def set_leds(arr, frame_mod=0):
    for blade in range(BLADES):
        arr[blade, :, :] = blade
    return arr

def create_bytes(preamble_list, arr):
    return bytearray(preamble_list+arr.reshape((BLADES*LEDS_PER_BLADE*BYTES_PER_LED)).tolist())

def start_server(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((ip, port))
    except OSError:
        raise ConnectionError("Plug in Teensy Ethernet to computer.")
    return sock

def transmit(sock, client_ip, client_port, preamble_list, arr):
    sock.sendto(create_bytes(preamble_list, arr), (client_ip, client_port))

def build_teensy():
    os.system("cd ~/Documents/GitHub/FanPersistentVision/FanPersistentVision; pio run -s -t upload > output.log 2>&1 & pio run -s -t upload > output.log 2>&1")
    print("Teensy attempted build")

def set_preamble(frame_mod):
    preamble = np.zeros(PREAMBLE_LENGTH)
    preamble[PREAMBLE_FRAME_INDEX] = frame_mod 
    return preamble.astype(np.uint8).tolist()

def main():
    build_teensy()
    frame_mod = 0
    sock = start_server(ip='0.0.0.0', port=SERVER_PORT)
    arr = np.array(np.zeros((BLADES, LEDS_PER_BLADE, BYTES_PER_LED)), dtype=np.int16)
    

    while True:
        frame_mod = np.mod(frame_mod+1, 255)
        preamble_list =  set_preamble(frame_mod)
        arr = set_leds(arr, frame_mod)
        transmit(sock, TEENSY_IP, TEENSY_PORT, preamble_list, arr)
        time.sleep(0.001)

constants = read_h_constants(os.getcwd()+'/src/constants.h')
BLADES = constants["BLADES"]
LEDS_PER_BLADE = constants["LEDS_PER_BLADE"]
BYTES_PER_LED = constants["BYTES_PER_LED"]

TEENSY_IP = constants["TEENSY_IP"]
TEENSY_PORT = constants["TEENSY_PORT"]
SERVER_IP = constants["SERVER_IP"]
SERVER_PORT =constants["SERVER_PORT"]

PREAMBLE_LENGTH = constants["PREAMBLE_LENGTH"]
PREAMBLE_FRAME_INDEX = constants["PREAMBLE_FRAME_INDEX"]

print("UDP server started")
try:
    main()
except KeyboardInterrupt:
    exit(0)