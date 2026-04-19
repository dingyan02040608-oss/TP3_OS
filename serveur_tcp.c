#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "shared.h"
#include "creme.h" 

void envoiContenu(int fd) {
    char buf[1024];
    int n = read(fd, buf, 1);
    if (n <= 0) {
        close(fd);
        return;
    }

    if (buf[0] == 'L') {
        pid_t pid = fork();
        if (pid == 0) { 
            dup2(fd, STDOUT_FILENO); 
            dup2(fd, STDERR_FILENO); 
            close(fd);
            execlp("ls", "ls", "-l", shared_reppub, NULL);
            exit(1);
        }
    } 
    else if (buf[0] == 'F') {
        int i = 0;
        while (read(fd, &buf[i], 1) == 1) {
            if (buf[i] == '\n') break;
            i++;
        }
        buf[i] = '\0'; 
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", shared_reppub, buf);

        pid_t pid = fork();
        if (pid == 0) { 
            dup2(fd, STDOUT_FILENO); 
            dup2(fd, STDERR_FILENO); 
            close(fd);
            execlp("cat", "cat", path, NULL);
            exit(1);
        }
    }
    close(fd); 
}

void * serveur_tcp(void * rep) {
    (void)rep;
    int sid_tcp;
    struct sockaddr_in serv_addr, cli_addr;
    signal(SIGCHLD, SIG_IGN);

    if ((sid_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(sid_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_BEUIP);

    if (bind(sid_tcp, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sid_tcp);
        pthread_exit(NULL);
    }

    listen(sid_tcp, 5);

    while (tcp_running) {
        socklen_t addr_len = sizeof(cli_addr);
        int client_fd = accept(sid_tcp, (struct sockaddr *)&cli_addr, &addr_len);
        
        if (!tcp_running) {
            if (client_fd >= 0) close(client_fd);
            break;
        }

        if (client_fd >= 0) {
            envoiContenu(client_fd);
        }
    }
    close(sid_tcp);
    return NULL;
}