import socket
import time


# Define the server's IP address (or '0.0.0.0' for all available interfaces)
HOST = '0.0.0.0' 
# Choose a port number above 1024 to avoid reserved ports
PORT = 12345

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # TCP optimization for high-throughput data
    server_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)  # Disable Nagle
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024*1024)  # 1MB send buffer
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024*1024)  # 1MB receive buffer
    
    # Additional optimizations for Linux
    try:
        server_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_WINDOW_CLAMP, 1024*1024)
        server_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_USER_TIMEOUT, 5000)  # 5 second timeout
    except AttributeError:
        pass  # Not available on all systems
    
    server_socket.bind((HOST, PORT))
    server_socket.listen(1)  # Only expect one connection
    print("Server started with optimized TCP settings")
    return server_socket

def transmit(client_socket, data_to_send):
    try:
        # Configure client socket for optimal throughput
        client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1024*1024)
        client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024*1024)
        
        # Send data in chunks for better TCP window utilization
        chunk_size = 65536  # 64KB chunks
        total_sent = 0
        data_len = len(data_to_send)
        
        while total_sent < data_len:
            end_pos = min(total_sent + chunk_size, data_len)
            chunk = data_to_send[total_sent:end_pos]
            client_socket.sendall(chunk)
            total_sent = end_pos
            
        # Remove the delay - let TCP handle flow control
        # time.sleep(0.01) - This was limiting your throughput!
        
    except Exception as e:
        print(f"Transmission error: {e}")
        raise