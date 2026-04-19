#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "gescom.h"
 
void gerer_sinal(int sig) {
    (void)sig; 
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}
 
char* generer_prompt() {
    char* user = getenv("USER");
    if (user == NULL) user = "inconnu";
    char machine[256];
    if (gethostname(machine, sizeof(machine)) != 0) strcpy(machine, "machine");
    char end_char = (geteuid() == 0) ? '#' : '$';
    size_t len = strlen(user) + strlen(machine) + 5; 
    char* prompt = (char *)malloc(len);
    if (prompt) snprintf(prompt, len, "%s@%s%c ", user, machine, end_char);
    return prompt;
}
 
int main() {
    char* ligne;
    char* prompt;
    signal(SIGINT, gerer_sinal);
    majComInt();
    char history[1024];
    char* home = getenv("HOME");
    if (home) {
        snprintf(history, sizeof(history), "%s/.biceps_history", home);
        read_history(history);
    }
 
    while(1) {
        prompt = generer_prompt();
        ligne = readline(prompt);
 
        if (ligne == NULL) {
            free(prompt);
            char* args[] = {"exit", NULL};
            execComInt(1, args);
            break;
        }
 
        if (strlen(ligne) > 0) {
            add_history(ligne);
            if (home) write_history(history);
 
            char* cmd_line = ligne;
            char* sous_cmd;
            while((sous_cmd = strsep(&cmd_line, ";")) != NULL) {
                int nombre_mots = analyseCom(sous_cmd);
                if(nombre_mots > 0) {
                    if (execComInt(nombre_mots, Mots) == 0) {                    
                        execComExt(Mots);
                    }
                }
            }
        }
        free(ligne);
        free(prompt);
    }
    return 0;
}