#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <netinet/in.h>

#define MAX_USERS 255
#define LPSEUDO 23
#define BCAST_ADDR "192.168.88.255"

struct elt {
    char nom[LPSEUDO + 1];
    char adip[16];
    struct elt *next;
};

extern struct elt *liste_contacts;
extern char my_pseudo[256];
extern int udp_running;
extern pthread_t udp_thread;
extern pthread_mutex_t table_mutex;
extern int tcp_running;
extern pthread_t tcp_thread;
extern char shared_reppub[256];

void * serveur_udp(void * p);
void * serveur_tcp(void * rep);
void commande(char octet1, char * message, char * pseudo);
void ajouteElt(char * pseudo, char * adip);
void supprimeElt(char * adip);
void listeElts(void);
char* get_pseudo_by_ip_safe(char * adip);
char* get_ip_by_pseudo_safe(char * pseudo);
void demandeListe(char * pseudo);
void demandeFichier(char * pseudo, char * nomfic);

#endif