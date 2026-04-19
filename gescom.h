#ifndef GESCOM_H
#define GESCOM_H

extern char** Mots;
extern int NMots;

int analyseCom(char *s);
void majComInt(void);
void listeComInt(void);
int execComInt(int N, char **p);
int execComExt(char **p);

#endif