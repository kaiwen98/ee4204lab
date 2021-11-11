gcc tcp_clients.c -o tcp_clients
gcc tcp_sers.c -o tcp_sers
./tcp_clients 127.0.0.1 > output.txt
