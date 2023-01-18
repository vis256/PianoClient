#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <poll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>
#include <string.h>

#define HEARTBEAT_INTERVAL 10

// Define the maximum number of users that can be in a room
#define MAX_USERS_PER_ROOM 16

// Define user roles
enum class UserRole {
    USER,
    MUSICIAN
};

// Define user class
class User {
public:
    User(int socket, UserRole role) : socket_(socket), role_(role) {}

    bool operator==(const User& other) { return socket_ == other.socket_; }
    
    bool operator!=(const User& other) { return !(*this == other); }
    
    // Get user's socket descriptor
    int getSocket() { return socket_; }

    // Get user's role
    UserRole getRole() { return role_; }

private:
    int socket_;
    UserRole role_;
};

// Define room class
class Room {
public:
    std::vector<User> users_;
    
    int getUserCount() { return users_.size(); }
    
    std::vector<User>& getUsers() { return users_; }
    
    Room(int id) : id_(id) {}
    
    Room() {}
    // Get room's ID
    int getId() { return id_; }

    // Add a user to the room
    void addUser(User& user) {
        users_.push_back(user);
    }

    // Remove a user from the room
    void removeUser(User& user) {
        users_.erase(std::remove(users_.begin(), users_.end(), user), users_.end());
    }

    // Broadcast a message to all users in the room
    void broadcast(const char* message, int messageSize) {
        for (auto& user : users_) {
            int socket = user.getSocket();
            write(socket, message, messageSize);
        }
    }

    // Check if the room is empty
    bool isEmpty() {
        return users_.empty();
    }

private:
    int id_;
};

// Define the server class
class Server {
public:
    Server() : nextRoomId_(0) {}

    // Start the server
    void start() {
        int port = 10000;
        std::cout<<"[D] starting server..."<<std::endl;
        // Create a socket
        int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

        std::cout << "[D] socket descrptr=" << socketDescriptor << std::endl;
        // Bind the socket to an address and port
        sockaddr_in address{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
        int br = bind(socketDescriptor, (sockaddr*) &address, sizeof(address));
        
        while (br == -1) {
            std::cout << "[D] " << port << " is taken, trying " << ++port << std::endl;
            address.sin_port = htons((short)port);
            br = bind(socketDescriptor, (sockaddr*) &address, sizeof(address));
        }
        
        std::cout << "[D] bind=" << br << std::endl;
        
        // Listen for incoming connections
        int lr = listen(socketDescriptor, SOMAXCONN);

        std::cout << "[D] listen=" << lr << std::endl;
        // Set up the poll structure
        pollfd pollFd;
        pollFd.fd = socketDescriptor;
        pollFd.events = POLLIN;
        
        /*
            while(true) {
                int clientSock = accept(servSock, nullptr, nullptr);
                printf("Accepted a new connection\n");
                while(true){
                    uint16_t msgSize;
                    if(recv(clientSock, &msgSize, sizeof(uint16_t), MSG_WAITALL) != sizeof(uint16_t))
                        break;
                    msgSize = ntohs(msgSize);
                    char data[msgSize+1]{};
                    if(recv(clientSock, data, msgSize, MSG_WAITALL) != msgSize)
                        break;
                    printf(" Received %2d bytes: |%s|\n", msgSize, data);
                }
                close(clientSock);
                printf("Connection closed\n");
            }
        */
        std::cout << "[!] listening on port " << port << std::endl;

        while (true) {
            // Wait for activity on the socket
            int pr = poll(&pollFd, 1, -1);
            std::cout << "[D] poll=" << pr << std::endl;
            
            // Check for incoming connections
            if (true) {
                sockaddr_in clientAddress;
                socklen_t clientAddressSize = sizeof(clientAddress);
                int clientSocket = accept(socketDescriptor, (sockaddr*) &clientAddress, &clientAddressSize);
                
                std::cout << "[D] clientSckt=" << clientSocket << std::endl;
                // Start a new thread to handle the connection
                std::thread([this, clientSocket]() {
                    // Process the request
                    while (1) {
                        handleRequest(clientSocket);
                    }
    
                    close(clientSocket);
                }).detach();
        }

        // Send heartbeat to all users to check if they are alive
        sendHeartbeat();

        // Remove empty rooms
        removeEmptyRooms();
    }
}

private:
std::map<int, Room> rooms_;
int nextRoomId_;

// Send the list of active rooms to a client
void sendRoomList(int clientSocket) {
    char message[1000] = "ROOMS|";
    // Build the list of room IDs
    for (auto const& it : rooms_) {
        char roomData[120];
        int uc = it.second.users_.size();
        sprintf(roomData, "%d %d/%d|", it.first, uc, MAX_USERS_PER_ROOM);
        strcat(message, roomData);        
    };

    // Send the list to the client
    // std::cout << "sent: " << message << "size: " << strlen(message) << std::endl;
    send(clientSocket, message, strlen(message), 0);
}

// Handle a request to create or join a room
void handleRequest(int clientSocket) {
    // Wait for the client to send a request to create or join a room
    char requestBuffer[1024];
    int requestSize = recv(clientSocket, requestBuffer, sizeof(requestBuffer), 0);
    
    std::string request = requestBuffer;
    
    std::cout << "[D] request=" << request << std::endl;

    if (request.substr(0, 6) == "create") {
        // Create a new room
        User user(clientSocket, UserRole::MUSICIAN);
        Room room(nextRoomId_++);
        room.addUser(user);
        rooms_[room.getId()] = room;

        // Send the room ID to the client
        send(clientSocket, ("JOIN|"+std::to_string(room.getId())).c_str(), std::to_string(room.getId()).size() + 14, 0);
    } else if (request.substr(0, 4) == "join") {
        // Get the room ID
        int roomId = std::stoi(request.substr(5));

        // Check if the room exists
        if (rooms_.count(roomId) == 0) {
            send(clientSocket, "invalid room", 12, 0);
            return;
        }

        // Check if the room is full
        Room& room = rooms_[roomId];
        if (room.getUserCount() >= MAX_USERS_PER_ROOM) {
            send(clientSocket, "room full", 9, 0);
            return;
        }

        // Add the user to the room
        User user(clientSocket, UserRole::USER);
        room.addUser(user);

        // Send the room ID to the client
        send(clientSocket, std::to_string(roomId).c_str(), std::to_string(roomId).size(), 0);
    } else if (request.substr(0, 4) == "list") {
        sendRoomList(clientSocket);
    } else {
        // Invalid request
        send(clientSocket, "invalid request|", 17, 0);
        return;
    }
}

// Send a heartbeat to all users to check if they are alive
void sendHeartbeat() {
    // for (auto& room : rooms_) {
    //     Room& current_room = room.second;
    //     for (auto& user : current_room.getUsers()) {
    //         int socket = user.getSocket();
    //         // send the message
    //         int sent = send(socket, "heartbeat", 9, 0);
    //         if (sent <= 0)
    //         {
    //             std::cout << "User is disconnected" << std::endl;
    //             close(socket);
    //             current_room.removeUser(user);
    //         }
    //     }

    //     sleep(HEARTBEAT_INTERVAL * 1000);
    //     }
    }
    // Remove empty rooms from the server
void removeEmptyRooms() {
    for (auto it = rooms_.begin(); it != rooms_.end();) {
        if (it->second.isEmpty()) {
            rooms_.erase(it++);
        } else {
            ++it;
        }
    }
}
};



int main(int argc, char * argv[]) {
    Server server;
    server.start();
    return 0;
}
