Chat Server Secure
=
Chat-Server
Simple chatroom in C unsing SSL to tunnel the connection between client and server. The repository is a fork of the [original Chat Server](https://github.com/yorickdewid/Chat-Server "Chat Server") repository. Due to the many requests for a secure version of he chat server I've decided to fork the project into a new repository. This project demonstrates the basic use of sockets and SSL. There is currently no client available but any telnet client will do. However I have planned a SSL client as well for the future. For now, just connect to the server on the specified port and address. By default port 5000 is used. The project was intended to run on Linux and Unix based systems. However with minor changes you'd be able to run it on Windows as well.

## Build
Run GNU make in the repository
`make`

Then start
`./chat_server`

## Features
* Accept multiple client (max 100)
* Name and rename users
* Send private messages
* SSL encrypted connections

## Chat commands

| Command       | Parameter             |                                     |
| ------------- | --------------------- | ----------------------------------- |
| \QUIT         |                       | Leave the chatroom                  |
| \PING         |                       | Test connection, responds with PONG |
| \ME           |                       | Sending message in 3rd person       |
| \NAME         | [nickname]            | Change nickname                     |
| \PRIVATE      | [reference] [message] | Send private message                |
| \ACTIVE       |                       | Show active clients                 |
| \HELP         |                       | Show this help                      |
