#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <queue>
#include <ncurses.h>
#include <fcntl.h>

#define PORT 4444

void print_with_color(int pair, const std::string &text){
    if(has_colors()){
        attron(COLOR_PAIR(pair));
        printw("%s", text.c_str());
        attroff(COLOR_PAIR(pair));
    }
    else{
        printw("%s", text.c_str());
    }
}

int main(){
    initscr();
    if(has_colors()){
        start_color();

        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
    }

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

    int flags = fcntl(s, F_GETFL, 0);
    if(flags == -1){
        endwin();
        std::cerr << "Failed to get socket flags" << std::endl;
        close(s);
        return -1;
    }

    if(fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1){
        endwin();
        std::cerr << "Failed to set socket to non-blocking mode" << std::endl;
        close(s);
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
        close(s);
        return -1;
    }

    if(listen(s, 1) < 0){
        endwin();
        std::cerr << "Failed to listen" << std::endl;
        close(s);
        return -1;
    }

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_s = -1;
    move(0,0);
    int rows, cols, max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);

    getyx(stdscr, rows, cols);
    
    char buffer[1024] = {0};

    fd_set readfds, writefds;

    std::queue<std::string> inputQueue;
    std::string text;
    int ch;
    move(rows, cols);
    printw("Waiting for a client");
    rows++;
    move(max_rows-1, cols);
    print_with_color(2, "Server: ");
    while(true){
        ch = getch();
        if(ch != ERR){
            if(ch == '\n' || ch == KEY_ENTER){
                if(text == "/exit"){
                    text.clear();

                    move(rows, cols);
                    printw("Exiting...");
                    move(max_rows-1, cols);
                    refresh();
                    std::string exit_m = "Server disconnected...";
                    if(client_s > -1){
                        send(client_s, exit_m.c_str(), exit_m.length(), 0);
                        shutdown(client_s, SHUT_WR);
                    }
                    shutdown(s, SHUT_RDWR);
                    break;
                }
                else if(text == "/clear"){
                    text.clear();

                    clear();
                    rows = 0;
                    cols = 0;
                    move(max_rows-1, cols);

                    print_with_color(2, "Server: ");
                }
                else if(client_s > -1 && !text.empty()){
                    inputQueue.push(text);

                    move(rows, cols);
                    print_with_color(2, "Server: " + text);

                    text.clear();
                    move(max_rows-1, cols);
                    clrtoeol();
                    print_with_color(2, "Server: ");
                    refresh();
                    rows++;
                }
                else{
                    text.clear();
                    move(max_rows-1, cols);
                    clrtoeol();
                    print_with_color(2, "Server: ");
                    refresh();
                }
            }
            else if(ch == 127 || ch == KEY_BACKSPACE){
                if(!text.empty()){
                    text.pop_back();
                    move(max_rows-1, cols);
                    clrtoeol();

                    print_with_color(2, "Server: " + text);
                    refresh();
                }
            }
            else{
                text += ch;
                move(max_rows-1, cols);
                clrtoeol();

                print_with_color(2, "Server: " + text);
                refresh();
            }
        }

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        if(client_s < 0){
            client_s = accept(s, (sockaddr*)&client_addr, &client_addr_len);
            if(client_s < 0){
                if(errno != EAGAIN && errno != EWOULDBLOCK){
                    int tmp_cols = getcurx(stdscr);
                    move(rows, cols);
                    printw("Failed to accept a socket");
                    move(max_rows-1, tmp_cols);
                    refresh();
                    rows++;
                }
            }
            else{
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "Welcome to the server!");
                send(client_s, buffer, sizeof(buffer), 0);

                int tmp_cols = getcurx(stdscr);
                move(rows, cols);
                printw("New client connected");
                move(max_rows-1, tmp_cols);
                refresh();
                rows++;
            }
        }

        if(client_s > -1){
           FD_SET(client_s, &readfds); 
           FD_SET(client_s, &writefds); 
        }

        if(client_s > -1){
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
                        int tmp_cols = getcurx(stdscr);
                        move(rows, cols);
                        printw("Client disconnected");
                        move(max_rows-1, tmp_cols);
                        refresh();
                        rows++;

                        close(client_s);
                        client_s = -1;

                        continue;

                    }
                    else{
                        int tmp_cols = getcurx(stdscr);
                        move(rows, cols);
                        print_with_color(1, "Client: " +  std::string(buffer));
                        move(max_rows-1, tmp_cols);
                        refresh();
                        rows++;
                    }
                }
            }
        }
    }
   
    close(client_s);
    close(s);
    endwin();

    return 0;
}
