#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <queue>

#define PORT 4444

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
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(s, (sockaddr*)&server_addr, server_addr_len) < 0){
        std::cout << "Failed to bind the socket" << std::endl;
        return -1;
    }

    if(listen(s, 1) < 0){
        std::cout << "Failed to listen" << std::endl;
        return -1;
    }

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_s = accept(s, (sockaddr*)&client_addr, &client_addr_len);
    if(client_s < 0){
        std::cout << "Failed to connect to the client" << std::endl;
        return -1;
    }
    
    char buffer[1024] = {0};
    strcpy(buffer, "Welcome to the server!");
        
    send(client_s, buffer, sizeof(buffer), 0);

    fd_set readfds, writefds;

    std::queue<std::string> inputQueue;

    while(true){

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(0, &readfds);
        FD_SET(client_s, &readfds);
        
        FD_SET(client_s, &writefds);

        if(select(std::max(client_s, 0) + 1, &readfds, &writefds, NULL, NULL) > 0){
            if(FD_ISSET(0, &readfds)){
                std::string input;
                std::getline(std::cin, input);

                inputQueue.push(input);
            }

            if(!inputQueue.empty() && FD_ISSET(client_s, &writefds)){
                std::string inputToSend = inputQueue.front();
                send(client_s, inputToSend.c_str(), inputToSend.length(), 0);
                inputQueue.pop();
            }

            if(FD_ISSET(client_s, &readfds)){
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytes_received = recv(client_s, buffer, sizeof(buffer), MSG_DONTWAIT);
                if(bytes_received < 0){
                    if(errno != EAGAIN && errno != EWOULDBLOCK){
                        std::cerr << "Recv error: " << strerror(errno) << std::endl;
                    }
                    continue;
                }
                else if(bytes_received == 0){
                    std::cout << "Client disconnected" << std::endl;
                    close(client_s);
                    break;
                }
                else{
                    std::cout << "Client: " << buffer << std::endl;
                }
            }
        }

    }
   
    close(s);

    return 0;
}
