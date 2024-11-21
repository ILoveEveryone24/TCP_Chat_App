#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <queue>
#include <ncurses.h>

#define PORT 4444

int main(){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        endwin();
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(s, (sockaddr*)&server_addr, server_addr_len) < 0){
        endwin();
        std::cerr << "Failed to bind the socket" << std::endl;
        return -1;
    }

    if(listen(s, 1) < 0){
        endwin();
        std::cerr << "Failed to listen" << std::endl;
        return -1;
    }

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_s = accept(s, (sockaddr*)&client_addr, &client_addr_len);
    if(client_s < 0){
        endwin();
        std::cerr << "Failed to connect to the client" << std::endl;
        return -1;
    }
    move(0,0);
    int rows, cols, max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);

    getyx(stdscr, rows, cols);
    
    char buffer[1024] = {0};
    strcpy(buffer, "Welcome to the server!");
        
    send(client_s, buffer, sizeof(buffer), 0);

    fd_set readfds, writefds;

    std::queue<std::string> inputQueue;
    std::string text;
    int ch;

    move(max_rows-1, cols);
    printw("Server: ");
    while(true){
        ch = getch();
        if(ch != ERR){
            if(ch == '\n' || ch == KEY_ENTER){
                if(text == "/exit"){
                    printw("Exiting...");
                    refresh();
                    break;
                }
                if(!text.empty()){
                    inputQueue.push(text);

                    move(rows, cols);
                    printw("Server: %s", text.c_str());

                    text.clear();
                    move(max_rows-1, cols);
                    clrtoeol();
                    printw("Server: ");
                    refresh();
                    rows++;
                }
            }
            else if(ch == 127 || ch == KEY_BACKSPACE){
                if(!text.empty()){
                    text.pop_back();
                    move(max_rows-1, cols);
                    clrtoeol();

                    printw("Server: %s", text.c_str());
                    refresh();
                }
            }
            else{
                text += ch;
                move(max_rows-1, cols);
                clrtoeol();

                printw("Server: %s", text.c_str());
                refresh();
            }
        }

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(client_s, &readfds);
        
        FD_SET(client_s, &writefds);

        if(select(client_s + 1, &readfds, &writefds, NULL, NULL) > 0){
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
                        printw("Recv error: %s", strerror(errno));
                    }
                    continue;
                }
                else if(bytes_received == 0){
                    printw("Client disconnected");
                    refresh();
                    close(client_s);
                    break;
                }
                else{
                    int tmp_cols = getcurx(stdscr);
                    move(rows, cols);
                    printw("Client: %s", buffer);
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
