import socket
import time
from datetime import datetime
import os
import numpy as np
from src.buildTeensy import build_teensy
from src.tcp import start_server, transmit

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

def set_leds(arr, frame=0):
    for circumf in range(LEDS_CRICUMF):
        arr[circumf, :, :] = np.mod(circumf, 256)
    return arr

def create_bytes(preamble_list, arr):
    # More efficient: pre-allocate and use numpy operations
    total_size = len(preamble_list) + arr.size
    result = np.empty(total_size, dtype=np.uint8)
    result[:len(preamble_list)] = preamble_list
    result[len(preamble_list):] = arr.ravel()
    return result.tobytes()

def int_to_2_uint8(integer):
    return np.mod(np.floor(integer/256), 256), np.mod(integer, 256)

def set_preamble(frame):
    preamble = np.zeros(PREAMBLE_LENGTH)
    preamble[PREAMBLE_MAGIC_NUMBER_INDEX] = PREAMBLE_MAGIC_NUMBER
    preamble[PREAMBLE_FRAME_INDEX0], preamble[PREAMBLE_FRAME_INDEX1] = int_to_2_uint8(frame)

    return preamble.astype(np.uint8).tolist()

def ellapsed_time_s():
    return datetime.now().timestamp - start_time

def main():
    build_teensy()
    frame = 0
    
    arr = np.array(np.zeros((LEDS_CRICUMF, LEDS_PER_BLADE, BYTES_PER_LED)), dtype=np.uint8)
    server_socket = start_server()
    
    try:
        client_socket, client_address = server_socket.accept()
        print(f"Client connected from {client_address}")
        # while True:
        #     preamble_list =  set_preamble(frame)
        #     arr = set_leds(arr, frame)
        #     data_to_send = create_bytes(preamble_list, arr)
        #     transmit(client_socket, data_to_send)
        #     print(f"Sent frame {frame}, size: {len(data_to_send)} bytes")
        #     frame = frame + 1

        preamble_list =  set_preamble(frame)
        arr = set_leds(arr, frame)
        data_to_send = create_bytes(preamble_list, arr)
        while True:
            transmit(client_socket, data_to_send)
            print(f"{datetime.now().isoformat()} Sent frame {frame}, size: {len(data_to_send)} bytes")
            frame = frame + 1
    except KeyboardInterrupt:
        try:
            client_socket.close()
            server_socket.close()
        except:
            ...


constants = read_h_constants(os.getcwd()+'/src/constants.h')
LEDS_CRICUMF = constants["LEDS_CRICUMF"]
LEDS_PER_BLADE = constants["LEDS_PER_BLADE"]
BYTES_PER_LED = constants["BYTES_PER_LED"]
BLADES = constants["BLADES"]

TEENSY_IP = constants["TEENSY_IP"]
TEENSY_PORT = constants["TEENSY_PORT"]
SERVER_IP = constants["SERVER_IP"]
SERVER_PORT =constants["SERVER_PORT"]

PREAMBLE_LENGTH = constants["PREAMBLE_LENGTH"]
PREAMBLE_MAGIC_NUMBER = constants["PREAMBLE_MAGIC_NUMBER"]
PREAMBLE_MAGIC_NUMBER_INDEX = constants["PREAMBLE_MAGIC_NUMBER_INDEX"]
PREAMBLE_FRAME_INDEX0 = constants["PREAMBLE_FRAME_INDEX0"]
PREAMBLE_FRAME_INDEX1 = constants["PREAMBLE_FRAME_INDEX1"]
PREAMBLE_FRAME_BUFFER_IN_INDEX0 = constants["PREAMBLE_FRAME_BUFFER_IN_INDEX0"]
PREAMBLE_FRAME_BUFFER_IN_INDEX1 = constants["PREAMBLE_FRAME_BUFFER_IN_INDEX1"]

start_time = datetime.now().timestamp()

print("UDP server started")
try:
    main()
except KeyboardInterrupt:
    exit(0)