#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "gescom.h"
#include "creme.h"
#include "shared.h"

char** Mots = NULL;
int NMots = 0;
#define NBMAXC 15 

typedef struct {
    char* nom;
    int (*fonction)(int, char**);
} CommandeInterne;

static CommandeInterne Tabcom[NBMAXC];
static int Nbcom = 0;

struct elt *liste_contacts = NULL;
char my_pseudo[256] = "";
int udp_running = 0;
int tcp_running = 0;
pthread_t udp_thread;
pthread_t tcp_thread;
char shared_reppub[256] = "reppub";
pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

void ajouteElt(char * pseudo, char * adip) {
    pthread_mutex_lock(&table_mutex);
    struct elt **ptr = &liste_contacts;
    while (*ptr) {
        if (strcmp((*ptr)->adip, adip) == 0) {
            struct elt *to_delete = *ptr;
            *ptr = (*ptr)->next;
            free(to_delete);
            break; 
        } else {
            ptr = &(*ptr)->next;
        }
    }
    
    struct elt *nouv = malloc(sizeof(struct elt));
    if (!nouv) { pthread_mutex_unlock(&table_mutex); return; }
    strncpy(nouv->nom, pseudo, LPSEUDO);
    nouv->nom[LPSEUDO] = '\0';
    strncpy(nouv->adip, adip, 15);
    nouv->adip[15] = '\0';
    nouv->next = NULL;
    
    ptr = &liste_contacts;
    while (*ptr && strcmp((*ptr)->nom, pseudo) < 0) {
        ptr = &(*ptr)->next;
    }
    nouv->next = *ptr;
    *ptr = nouv;
    pthread_mutex_unlock(&table_mutex);
}

void supprimeElt(char * adip) {
    pthread_mutex_lock(&table_mutex);
    struct elt **ptr = &liste_contacts;
    while (*ptr) {
        if (strcmp((*ptr)->adip, adip) == 0) {
            struct elt *to_delete = *ptr;
            *ptr = (*ptr)->next;
            free(to_delete);
            break;
        }
        ptr = &(*ptr)->next;
    }
    pthread_mutex_unlock(&table_mutex);
}

void listeElts(void) {
    pthread_mutex_lock(&table_mutex);
    struct elt *curr = liste_contacts;
    while(curr) {
        printf("%s : %s\n", curr->adip, curr->nom);
        curr = curr->next;
    }
    pthread_mutex_unlock(&table_mutex);
}

char* get_pseudo_by_ip_safe(char * adip) {
    static char pseudo_copy[LPSEUDO + 1];
    strcpy(pseudo_copy, "Inconnu");
    pthread_mutex_lock(&table_mutex);
    struct elt *curr = liste_contacts;
    while(curr) {
        if (strcmp(curr->adip, adip) == 0) {
            strcpy(pseudo_copy, curr->nom);
            break;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&table_mutex);
    return pseudo_copy;
}

char* get_ip_by_pseudo_safe(char * pseudo) {
    static char ip_copy[16];
    strcpy(ip_copy, "");
    pthread_mutex_lock(&table_mutex);
    struct elt *curr = liste_contacts;
    while(curr) {
        if (strcmp(curr->nom, pseudo) == 0) {
            strcpy(ip_copy, curr->adip);
            break;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&table_mutex);
    return ip_copy;
}

void demandeListe(char * pseudo) {
    char * ip = get_ip_by_pseudo_safe(pseudo);
    if (strlen(ip) == 0) return;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_BEUIP);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd); return;
    }

    write(sockfd, "L", 1); 
    char buf[1024];
    int n;
    while ((n = read(sockfd, buf, sizeof(buf)-1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    close(sockfd);
}

void demandeFichier(char * pseudo, char * nomfic) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", shared_reppub, nomfic);

    if (access(path, F_OK) == 0) return;

    char * ip = get_ip_by_pseudo_safe(pseudo);
    if (strlen(ip) == 0) return;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_BEUIP);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd); return;
    }

    char req[512];
    snprintf(req, sizeof(req), "F%s\n", nomfic);
    write(sockfd, req, strlen(req));

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        close(sockfd); return;
    }

    char buf[1024];
    int n;
    while ((n = read(sockfd, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }
    
    close(fd);
    close(sockfd);
}

static int Sortie(int N, char* p[]) {
    (void)N; (void)p;
    if (udp_running) {
        udp_running = 0;
        tcp_running = 0;
        send_beuip_cmd('0', BCAST_ADDR, "", NULL); 
        send_beuip_cmd('0', "127.0.0.1", "", NULL);
        
        int tmp_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT_BEUIP);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(tmp_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        close(tmp_sock);

        pthread_join(udp_thread, NULL);
        pthread_join(tcp_thread, NULL);
    }

    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
        Mots = NULL;
    }

    struct elt *curr = liste_contacts;
    while(curr) {
        struct elt *suiv = curr->next;
        free(curr);
        curr = suiv;
    }
    liste_contacts = NULL;
    clear_history();
    exit(0);
}

static int CD(int N, char* p[]) {
    if (N < 2) return -1;
    if (chdir(p[1]) != 0) return -1;
    return 0;
}

static int PWD(int N, char* p[]) {
    (void)N; (void)p;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) { printf("%s\n", cwd); return 0; }
    return -1;
}

static int VERS(int N, char* p[]) {
    (void)N; (void)p;
    return 0;
}

void commande(char octet1, char * message, char * pseudo) {
    if (!udp_running) return;

    if (octet1 == '3') { 
        listeElts();
    } 
    else if (octet1 == '4' && pseudo && message) { 
        pthread_mutex_lock(&table_mutex);
        struct elt *curr = liste_contacts;
        while(curr) {
            if (strcmp(curr->nom, pseudo) == 0) {
                send_beuip_cmd('9', curr->adip, message, NULL);
                break;
            }
            curr = curr->next;
        }
        pthread_mutex_unlock(&table_mutex);
    } 
    else if (octet1 == '5' && message) { 
        pthread_mutex_lock(&table_mutex);
        struct elt *curr = liste_contacts;
        while(curr) {
            if (strcmp(curr->nom, my_pseudo) != 0) { 
                send_beuip_cmd('9', curr->adip, message, NULL);
            }
            curr = curr->next;
        }
        pthread_mutex_unlock(&table_mutex);
    }
}

void add_self_with_real_ip(char* pseudo) {
    struct ifaddrs *ifaddr, *ifa;
    char local_ip[16] = "127.0.0.1"; 

    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                char *ip = inet_ntoa(sa->sin_addr);
                if (strncmp(ip, "127.", 4) != 0) {
                    strncpy(local_ip, ip, 15);
                    local_ip[15] = '\0';
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    ajouteElt(pseudo, local_ip);
}

static int BEUIP_CMD(int N, char* p[]) {
    if (N < 2) return -1;

    if (strcmp(p[1], "start") == 0 && N == 3) {
        if (udp_running) return -1;
        mkdir(shared_reppub, 0777); 
        strncpy(my_pseudo, p[2], 255);
        udp_running = 1;
        tcp_running = 1;
        
        add_self_with_real_ip(my_pseudo);
        
        pthread_create(&udp_thread, NULL, serveur_udp, my_pseudo);
        pthread_create(&tcp_thread, NULL, serveur_tcp, shared_reppub);
    } 
    else if (strcmp(p[1], "stop") == 0) {
        if (!udp_running) return -1;
        udp_running = 0; 
        tcp_running = 0;
        send_beuip_cmd('0', BCAST_ADDR, "", NULL); 
        send_beuip_cmd('0', "127.0.0.1", "", NULL);
        
        int tmp_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT_BEUIP);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(tmp_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        close(tmp_sock);

        pthread_join(udp_thread, NULL);
        pthread_join(tcp_thread, NULL);
    }
    else if (strcmp(p[1], "list") == 0) {
        commande('3', NULL, NULL);
    }
    else if (strcmp(p[1], "message") == 0 && N >= 4) {
        char msg_buf[1024] = "";
        for(int i = 3; i < N; i++) {
            strcat(msg_buf, p[i]);
            if (i < N - 1) strcat(msg_buf, " ");
        }
        if (strcmp(p[2], "all") == 0) {
            commande('5', msg_buf, NULL);
        } else {
            commande('4', msg_buf, p[2]);
        }
    }
    else if (strcmp(p[1], "ls") == 0 && N == 3) {
        demandeListe(p[2]);
    }
    else if (strcmp(p[1], "get") == 0 && N == 4) {
        demandeFichier(p[2], p[3]);
    }
    return 0;
}

int analyseCom(char *s) {
    int j = 0; char* token; int max = 100;
    if (Mots != NULL) {
        for (int i = 0; i < NMots; i++) free(Mots[i]);
        free(Mots);
    }
    Mots = (char**)malloc(max * sizeof(char*));
    if (Mots == NULL) return 0;
    while ((token = strsep(&s, " \t\n")) != NULL) {
        if (strlen(token) > 0) { Mots[j] = strdup(token); j++; }
    }
    Mots[j] = NULL; NMots = j; return NMots;
}

static void ajouteCom(char* nom, int(*fonction)(int, char**)) {
    if (Nbcom >= NBMAXC) return;
    Tabcom[Nbcom].nom = nom; Tabcom[Nbcom].fonction = fonction; Nbcom++;
}

void majComInt(void) {
    ajouteCom("exit", Sortie);
    ajouteCom("cd", CD);
    ajouteCom("pwd", PWD);
    ajouteCom("vers", VERS);
    ajouteCom("beuip", BEUIP_CMD); 
}

void listeComInt(void) {
    for (int i = 0; i < Nbcom; i++) printf("- %s\n", Tabcom[i].nom);
}

int execComInt(int N, char **p) {
    if (N == 0 || p[0] == NULL) return 0;
    for (int i = 0; i < Nbcom; i++) {
        if (strcmp(p[0], Tabcom[i].nom) == 0) {
            Tabcom[i].fonction(N, p); return 1;
        }
    }
    return 0;
}

int execComExt(char **p) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    else if (pid == 0) {
        execvp(p[0], p); exit(EXIT_FAILURE);
    } else {
        int status; waitpid(pid, &status, 0);
    }
    return 0;
}