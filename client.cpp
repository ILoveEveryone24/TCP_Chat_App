#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <string>

#define PORT 4444
#define IP "127.0.0.1"

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        std::cout << "Failed to create socket" << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &server_addr.sin_addr);

    connect(s, (sockaddr*)&server_addr, server_addr_len);

    char buffer[1024] = {0};
    
    strcpy(buffer, "Client has joined the channel!");
                
    send(s, buffer, sizeof(buffer), 0);

    fd_set readfds, writefds;

    std::queue<std::string> inputQueue;

    while(true){

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(0, &readfds);
        FD_SET(s, &readfds);

        FD_SET(s, &writefds);

        if(select(std::max(s, 0) + 1, &readfds, &writefds, NULL, NULL) > 0){
            if(FD_ISSET(0, &readfds)){
                std::string input;
                std::getline(std::cin, input);

                inputQueue.push(input);
            }

            if(!inputQueue.empty() && FD_ISSET(s, &writefds)){
                std::string inputToSend = inputQueue.front();
                send(s, inputToSend.c_str(), inputToSend.length(), 0);
                inputQueue.pop();
            }

            if(FD_ISSET(s, &readfds)){
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_received = recv(s, buffer, sizeof(buffer), MSG_DONTWAIT);
                if(bytes_received < 0){
                    if(errno != EAGAIN && errno != EWOULDBLOCK){
                        std::cerr << "Recv error: " << strerror(errno) << std::endl;
                    }
                    continue;
                }
                else if(bytes_received == 0){
                    std::cout << "Connection closed." << std::endl;
                    break;
                }
                else{
                    std::cout << "Server: " << buffer << std::endl;
                }
            }
        }

    }

    close(s);

    return 0;
}
