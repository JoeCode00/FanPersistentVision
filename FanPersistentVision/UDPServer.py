import socket
import time
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

def set_leds(arr):
    for blade in range(BLADES):
        arr[blade, 0, 0] = blade
    return arr

def create_bytes(arr):
    return bytearray(arr.reshape((BLADES*LEDS_PER_BLADE*BYTES_PER_LED)).tolist())

def start_server(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((ip, port))
    except OSError:
        raise ConnectionError("Plug in Teensy Ethernet to computer.")
    return sock

def transmit(sock, client_ip, client_port, arr):
    sock.sendto(create_bytes(arr), (client_ip, client_port))

def main():
    sock = start_server(ip='0.0.0.0', port=SERVER_PORT)
    arr = np.array(np.zeros((BLADES, LEDS_PER_BLADE, BYTES_PER_LED)), dtype=np.int8)
    arr = set_leds(arr)

    while True:
        transmit(sock, TEENSY_IP, TEENSY_PORT, arr)

constants = read_h_constants(os.getcwd()+'/src/constants.h')
BLADES = constants["BLADES"]
LEDS_PER_BLADE = constants["LEDS_PER_BLADE"]
BYTES_PER_LED = constants["BYTES_PER_LED"]

TEENSY_IP = constants["TEENSY_IP"]
TEENSY_PORT = constants["TEENSY_PORT"]
SERVER_IP = constants["SERVER_IP"]
SERVER_PORT =constants["SERVER_PORT"]

print("UDP server started")
main()