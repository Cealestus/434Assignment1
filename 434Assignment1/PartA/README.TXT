As only one of the three types of servers are completed, only one makefile command is required to run the server.

To compile the server, enter make on the terminal. This will compile the server. This will create an output "server" file.
To run the server, type ./server and hit enter. The server will then be waiting for a connection.

In a separated terminal, type netcat <IP address of the server terminal> 30005.
This will connect to the server. Running any of the commands should then be possible.

To exit the server, type exit.