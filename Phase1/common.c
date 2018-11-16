#include "common_impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include<netdb.h>
#include<arpa/inet.h>


//Create the socket

int creer_socket(int *port_num) {
  int fd;
  struct sockaddr_in serv_addr;
  fd=do_socket(AF_INET, SOCK_STREAM,0);
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr=init_serv_addr(serv_addr);

  if(fd == -1) {
    perror("ERROR in do_socket()");
  }
  int nb_connexion=20;
  if(bind(fd, (struct sockaddr*)(&serv_addr), sizeof(struct sockaddr_in)) == -1){
    perror("ERROR with bind()");
  }

  if(listen(fd, nb_connexion) == -1){
    perror("ERROR with bind()");
  }

  socklen_t len;
  len = sizeof(struct sockaddr*);
  if(getsockname(fd, (struct sockaddr*)(&serv_addr), &len) == -1){
    perror("error with getsockname()");
  }

  *port_num = ntohs(serv_addr.sin_port);
  printf("port num cote server=%d\n", *port_num);

  return fd;

  /* fonction de creation et d'attachement */
  /* d'une nouvelle socket */
  /* renvoie le numero de descripteur */
  /* et modifie le parametre port_num */
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */

int do_socket(int domain, int type, int protocol) {
  int sockfd;
  int yes = 1;
  sockfd = socket(domain,type,protocol);

  //check for socket validity
  if (sockfd==-1){
    perror("ERROR: NO CLIENT SOCKET CREATED");
  }
  // set socket option, to prevent "already in use" issue when rebooting the server right on

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
    perror("ERROR setting socket options");
  }
  return sockfd;
}

// Server initialization

struct sockaddr_in init_serv_addr(struct sockaddr_in serv_addr){
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = 0;
  return serv_addr;
}

int compte_lignes(FILE *fichier){
  int caractere;
  int n_ligne=0;
  caractere=0;
  while (caractere!=EOF){
    caractere=fgetc(fichier);
    if(caractere=='\n'){
      n_ligne=n_ligne+1;
    }
  }
  return n_ligne;
}

char** tableau_mot(char **tableau, FILE *fichier, int n_ligne){
  rewind(fichier);
  int i;
  for(i=0;i<n_ligne;i++){
    fscanf(fichier, "%s", tableau[i]);
    printf("tableau[%d]=%s",i,tableau[i]);
  }
  fclose(fichier);
  return tableau;
}


int do_accept(int sockfd){
  int var = accept(sockfd,NULL,NULL);
  if(var<0){
    perror("server: accept");
  }
  return var;
}


int do_write(int sockfd, char *buf){
  char *taille = malloc(sizeof(size_t));
  size_t len = strlen(buf);
  sprintf(taille, "%d", (int) len);
  write(sockfd, taille, sizeof(size_t));
  write(sockfd, buf, len);
  return 0;
}

ssize_t do_read(int sockfd, char *buf){
  ssize_t rl;
  ssize_t size;
  char *taille = malloc(sizeof(size_t));
  rl = read(sockfd, taille, sizeof(size_t));
  if (rl == 0) {
    return 0;
  }
  size = read(sockfd, buf, (size_t) atoi(taille));
  return size;
}

char * hostname_to_ip(char* hostname){
  struct hostent *he;
  struct in_addr **addr_list;
  int i;
  if((he=gethostbyname(hostname))==NULL){
    return NULL;
  }
  addr_list=(struct in_addr **) he->h_addr_list;
  for(i=0; addr_list[i] !=NULL; i++){
    return inet_ntoa(*addr_list[i]);
  }
  return NULL;
}
