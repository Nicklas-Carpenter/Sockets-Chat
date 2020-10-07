// chat.h - Definitions for sockets_chat
// Copyright (C) 2020  Nicklas Carpenter

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef CHAT_H
#define CHAT_H

#define DEFAULT_PROTOCOL 0 // The default protocol for a given socket type
#define NO_FLAGS 0 // Specify no flags in send and receive operations
#define CONNECTION_NOT_ESTABLISHED -1 // Connection to server not established
#define PORT 1024 // The port the chatserver is hosted on
#define SERVICE "1024" // The service name for the chat server
#define MAX_MSG_SIZE 140 // The size of the largest messages that can sent
#define MAX_UNAME_SIZE 12 // The maximum size a username is allowed to be
#define EXIT_CMD "~quit\n" // The command that initiates disconnect and quits
#define HOST 0
#define CLIENT 1
#define PORT_MIN 1024
#define PORT_MAX 65535
#define IPV4_REGEX "((([01]?[0-9]?[0-9])|(2([0-4][0-9]|5[0-5])))\.){3}(([01]?[0-9]?[0-9])|(2([0-4][0-9]|5[0-5])))"

#endif