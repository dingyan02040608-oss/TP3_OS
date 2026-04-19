#include "creme.h"
#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_beuip_cmd(char code, const char* target_ip, const char* p1, const char* p2) {
    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest;
    char buffer[1024];
    memset(buffer, 0, 1024);

    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT_BEUIP);
    dest.sin_addr.s_addr = inet_addr(target_ip);

    if (strcmp(target_ip, BCAST_ADDR) == 0) {
        int b = 1;
        setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b));
    }

    int n1 = sprintf(buffer, "%c%s%s", code, MAGIC, p1);
    int total_len = n1;

    if (code == '4' && p2) {
        strcpy(buffer + n1 + 1, p2);
        total_len = n1 + 1 + strlen(p2);
    }

    sendto(sid, buffer, total_len, 0, (struct sockaddr *)&dest, sizeof(dest));
    close(sid);
}