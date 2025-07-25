import socket
import time
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('0.0.0.0', 12345)
sock.bind(server_address)

# arr = []
# for i in range(256*19):
#     arr += [i % 256] * 3  # Repeat each value three times

arr = [0, 128, 255]*4500 #128 is a dummy byte. 3 bytes to a color, 2000 colors per UDP 'frame'.
while True:
    sock.sendto(bytearray(arr), ('10.0.0.3', 8888))