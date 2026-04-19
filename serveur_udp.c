#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "shared.h"
#include "creme.h"

void * serveur_udp(void * p) {
    char* pseudo = (char*)p;
    int sid;
    
    if ((sid = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        pthread_exit(NULL);
    }

    struct sockaddr_in serv_addr, cli_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_BEUIP);

    if (bind(sid, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sid);
        pthread_exit(NULL);
    }

    int bEnable = 1;
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &bEnable, sizeof(bEnable));
    send_beuip_cmd('1', BCAST_ADDR, pseudo, NULL);

    while (udp_running) {
        char buf[1024];
        socklen_t addr_len = sizeof(cli_addr);
        int n = recvfrom(sid, buf, 1023, 0, (struct sockaddr *)&cli_addr, &addr_len);
        if (n < 0) continue;
        buf[n] = '\0';

        if (strncmp(buf + 1, MAGIC, 5) != 0) continue; 

        char code = buf[0];
        char* payload = buf + 6;
        char adip[16];
        strcpy(adip, inet_ntoa(cli_addr.sin_addr));

        if (code != '0' && code != '1' && code != '2' && code != '9') {
            continue;
        }

        if (code == '1') { 
            ajouteElt(payload, adip);
            char ack[512];
            sprintf(ack, "2%s%s", MAGIC, pseudo);
            sendto(sid, ack, strlen(ack), 0, (struct sockaddr *)&cli_addr, addr_len);
        }
        else if (code == '2') { 
            ajouteElt(payload, adip);
        }
        else if (code == '9') { 
            printf("\nMessage de %s : %s\n", get_pseudo_by_ip_safe(adip), payload);
        }
        else if (code == '0') { 
            if (!udp_running) break; 
            supprimeElt(adip);
        }
    }
    close(sid);
    return NULL;
}