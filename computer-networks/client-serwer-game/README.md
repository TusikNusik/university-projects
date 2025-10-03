The repository contains a project in C++ that I wrote as part of a graded assignment for the Computer Networks course. Below is a shortened description of the task.

Running the make command should produce two executables: approx-server and approx-client (the server and the client, respectively). The assignment simulates a game.

The goal of each player (represented by the client program) is to approximate as accurately as possible a polynomial of degree N received from the server, in integer points from 0 to K.
Initially, the values of the approximating function f are zero. Approximation is performed by adding a given value at a given point specified by the client. For example, for K=3 and the command to add 0.5 at point 2.

Communication between the client and the server is text-based over a TCP connection, using either IPv4 or IPv6. Each message ends with a carriage return character (ASCII code 13) and a newline character (ASCII code 10), denoted as \r\n. Fields in messages are separated by a single space. There are no other whitespace characters in the messages.

Server and Client Run Parameters
Server

The server is started with the command approx-server with the following parameters:

-p port – the server port number, an integer between 0 and 65535 in base 10. Optional, default is 0.

-k value – the value of constant K, an integer in the range 1–10000 in base 10. Optional, default is 100.

-n value – the value of constant N, an integer in the range 1–8 in base 10. Optional, default is 4.

-m value – the value of constant M, an integer in the range 1–12341234 in base 10. Optional, default is 131.

-f file – the name of the file containing server messages with the polynomial coefficients (see the description of the COEFF message). Mandatory.

The server listens on the given TCP port for client connections. If the port number is zero, the server chooses an arbitrary port. It must be possible to establish connections with the server using both IPv6 and IPv4.
If the computer running the server does not support IPv6 communication, it must still be possible to connect via IPv4.

Client

The client is started with the command approx-client with the following parameters:

-u player_id – the player identifier consisting of digits and uppercase/lowercase English letters. Mandatory.

-s server – the server address or hostname. Mandatory.

-p port – the server port number, an integer between 1 and 65535 in base 10. Mandatory.

-4 – force communication with the server via IPv4. Optional.

-6 – force communication with the server via IPv6. Optional.

-a – selects the type of strategy (see the section “Client Strategy”). Optional.

If both -4 and -6 are provided, or if neither is given, the client should choose the protocol version based on the first IP address assigned to the server.
