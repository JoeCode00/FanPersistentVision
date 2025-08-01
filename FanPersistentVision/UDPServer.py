import socket
import time
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('0.0.0.0', 12345)
sock.bind(server_address)

# arr = []
# for i in range(256*19):
#     arr += [i % 256] * 3  # Repeat each value three times

arr = [0, 0, 0]+[1, 0, 0]+[2, 0, 0]+[3, 0, 0]+[4, 0, 0]
while True:
    sock.sendto(bytearray(arr), ('10.0.0.3', 8888))