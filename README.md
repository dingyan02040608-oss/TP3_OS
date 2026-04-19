Markdown
# Polytech OS User - TP3 : BICEPS (Bel Interpréteur de Commandes)

**Auteurs :** Yan DING et YiHan HU

## Description du Projet
Ce projet est la réalisation de la partie 3 du TP d'OS User. Il s'agit d'une évolution de notre programme `biceps` (Bel Interpréteur de Commandes des Elèves de Polytech Sorbonne), intégrant le protocole BEUIP (BEUI over IP). 

Conformément aux consignes, nous avons réalisé :
- **Étape 1 :** Implémentation du multi-threading pour faire cohabiter l'interpréteur de commandes et le serveur UDP dans le même processus, résolvant ainsi les failles de sécurité (partage de la table des contacts via mutex).
- **Étape 2 :** Remplacement du tableau fixe par une **liste chaînée** dynamique pour gérer les utilisateurs présents sur le réseau. (La partie 2.1 a été sautée comme autorisé).
- **Étape 3 :** Mise en place d'un thread serveur TCP permettant le partage et le téléchargement de fichiers entre utilisateurs via les commandes `beuip ls` et `beuip get`.

## Structure du Code

Afin de garantir la lisibilité et la maintenabilité (< 20 lignes par fonction en moyenne), le code est découpé de manière modulaire :

* **`bicep.c`** : Point d'entrée du programme. Gère la boucle principale de l'interpréteur, le prompt personnalisé, l'historique (`readline`) et la capture des signaux (`SIGINT`).
* **`gescom.c` / `gescom.h`** : Cœur de la logique des commandes. Contient l'analyseur lexical, la gestion de la liste chaînée (protégée par `pthread_mutex_t`), la gestion des threads (lancement/arrêt) et l'exécution des commandes internes (`beuip start`, `stop`, `list`, `message`, `ls`, `get`).
* **`serveur_udp.c`** : Code exécuté par le thread serveur UDP. Écoute sur le port 9998, répond aux requêtes de présence (code '1' et '2'), met à jour la liste chaînée et affiche les messages reçus (code '9').
* **`serveur_tcp.c`** : Code exécuté par le thread serveur TCP (Bonus). Écoute sur le port 9998 en TCP pour traiter les requêtes de listage (`L`) et de téléchargement de fichiers (`F`) via des processus fils (`fork` + `exec`).
* **`creme.c` / `creme.h`** : Librairie utilitaire encapsulant l'envoi des datagrammes UDP formatés pour le protocole BEUIP.
* **`shared.h`** : Fichier d'en-tête global contenant les définitions des structures (liste chaînée `struct elt`), les variables globales partagées (`mutex`, flags de threads) et l'adresse de broadcast (`BCAST_ADDR "192.168.88.255"`).
* **`Makefile`** : Gère la compilation stricte et les tests de fuites mémoire.

## Compilation et Exécution

Le projet compile strictement avec les options `-Wall -Wextra -Werror` (aucun warning toléré).

### 1. Compilation standard
Pour générer l'exécutable principal `biceps` :
```bash
make
./biceps
2. Tests de fuites mémoire (Valgrind)
Pour compiler une version spécifique au debug (-g -O0) et lancer les tests Valgrind :

Bash
make memory-leak
valgrind --leak-check=full --track-origins=yes --errors-for-leak-kinds=all --error-exitcode=1 ./biceps-memory-leaks
Note : Le programme a été testé rigoureusement et assure un nettoyage complet (libération de la liste chaînée, des buffers et de l'historique readline) lors de la commande exit ou de la combinaison CTRL+D.

3. Nettoyage
Pour supprimer tous les exécutables et fichiers objets :

Bash
make clean
Exemples d'utilisation
Une fois dans le shell biceps :

Bash
# Lancer les serveurs UDP et TCP avec votre pseudo
beuip start Yan

# Afficher la liste des utilisateurs présents (lecture de la liste chaînée)
beuip list
# Résultat attendu : 192.168.88.161 : YiHan

# Envoyer un message privé à un utilisateur
beuip message YiHan Salut, ça va ?

# Envoyer un message à tout le monde
beuip message all Bonjour le réseau !

# Lister les fichiers du répertoire "reppub" d'un utilisateur distant
beuip ls YiHan

# [Bonus] Télécharger un fichier distant dans votre dossier local "reppub"
beuip get YiHan document.pdf

# Arrêter proprement les serveurs
beuip stop
