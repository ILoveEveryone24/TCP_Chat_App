#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <queue>
#include <string>
#include <chrono>
#include <thread>
#include <ncurses.h>

#define PORT 4444
#define IP "127.0.0.1"

int main(){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

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

    printw("Connecting");
    refresh();
    int connected = connect(s, (sockaddr*)&server_addr, server_addr_len);

    while(errno == ECONNREFUSED){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        connected = connect(s, (sockaddr*)&server_addr, server_addr_len);
        printw(".");
        refresh();
    }
    clear();
    move(0,0);
    int rows, cols, max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);

    getyx(stdscr, rows, cols);
    
    char buffer[1024] = {0};
    
    strcpy(buffer, "Client has joined the channel!");
                
    send(s, buffer, sizeof(buffer), 0);

    fd_set readfds, writefds;

    std::queue<std::string> inputQueue;
    std::string text;
    int ch;

    move(max_rows-1, cols);
    printw("Client: ");
    while(true){
        ch = getch();
        if(ch != ERR){
            if(ch == '\n' || ch == KEY_ENTER){
                inputQueue.push(text);

                move(rows, cols);
                printw("Client: %s", text.c_str());

                text.clear();
                move(max_rows-1, cols);
                clrtoeol();
                printw("Client: ");
                refresh();
                rows++;
            }
            else if(ch == 127 || ch == KEY_BACKSPACE){
                if(!text.empty()){
                    text.pop_back();
                    move(max_rows-1, cols);
                    clrtoeol();

                    printw("Client: %s", text.c_str());
                    refresh();
                }
            }
            else{
                text += ch;
                move(max_rows-1, cols);
                clrtoeol();

                printw("Client: %s", text.c_str());
                refresh();
            }
        }

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(s, &readfds);

        FD_SET(s, &writefds);

        if(select(s + 1, &readfds, &writefds, NULL, NULL) > 0){
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
                    int tmp_cols = getcurx(stdscr);
                    move(rows, cols);
                    printw("Server: %s", buffer);
                    move(max_rows-1, tmp_cols);
                    refresh();
                    rows++;
                }
            }
        }

    }

    close(s);
    endwin();

    return 0;
}
