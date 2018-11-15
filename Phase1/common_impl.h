#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* autres includes (eventuellement) */

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   char* machine_name;
   int sockfd;
   int port;
   /* a completer */
};
typedef struct dsm_proc_conn dsm_proc_conn_t;

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {
  char pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int *port_num);
int do_socket(int domain, int type, int protocol);
struct sockaddr_in init_serv_addr();
int compte_lignes(FILE *fichier);
char** tableau_mot(char **tableau, FILE *fichier, int n_ligne);
int do_accept(int sockfd);
void do_write(int sockfd, char *message, int len);
void do_read(int sockfd, char *buf, int len);
struct in_addr * hostname_to_ip(char* hostname);
