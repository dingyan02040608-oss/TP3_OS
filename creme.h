#ifndef CREME_H
#define CREME_H

#define PORT_BEUIP 9998
#define MAGIC "BEUIP"

void send_beuip_cmd(char code, const char* target_ip, const char* p1, const char* p2);

#endif