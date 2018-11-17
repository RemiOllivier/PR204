#include "common_impl.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include<arpa/inet.h>

int main(int argc, char **argv)
{
  /* processus intermediaire pour "nettoyer" */
  /* la liste des arguments qu'on va passer */
  /* a la commande a executer vraiment */

  /* creation d'une socket pour se connecter au */
  /* au lanceur et envoyer/recevoir les infos */
  /* necessaires pour la phase dsm_init */
  struct sockaddr_in sin;
  sin.sin_family=AF_INET;
  sin.sin_port=htons(atoi(argv[2]));
  //memcpy(hostname_to_ip(argv[1]), &sin->sin_addr, sizeof(sin->sin_addr));
  inet_aton(hostname_to_ip(argv[1]), &sin.sin_addr);
  printf("adresse:%s\n", hostname_to_ip(argv[1]));
  printf("port:%d\n", htons(atoi(argv[2])));
  fflush(stdout);

  //get the socket

  int yes=1;
  int port;
  char *port_envoi=malloc(10*sizeof(char));
  char *adresse=malloc(100);
  memset(adresse, 0, 100);
  gethostname(adresse, 100);
  printf("adresse:%s\n", adresse);
  fflush(stdout);

  int sockfd= socket(AF_INET, SOCK_STREAM,0);
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
    perror("ERROR setting socket options");
  }
  if (connect(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0) {
    perror("client: connect");
  }

  /* Envoi du nom de machine au lanceur */
  do_write(sockfd, adresse);

  /* Envoi du pid au lanceur */
  pid_t pid=getpid();
  char *pid_envoi=malloc(10*sizeof(char));;
  sprintf(pid_envoi,"%d", pid);
  do_write(sockfd, pid_envoi);

  /* Creation de la socket d'ecoute pour les */
  /* connexions avec les autres processus dsm */
  int socket_ecoute=creer_socket(&port);

  /* Envoi du numero de port au lanceur */
  /* pour qu'il le propage à tous les autres */
  /* processus dsm */
  sprintf(port_envoi,"%d", port);
  do_write(sockfd, port_envoi);

  /* on execute la bonne commande */
  char **arg=malloc((argc-3)*sizeof(char*));
  arg[0]=argv[3];
  int numero_arg;
  for (numero_arg=1; numero_arg<argc; numero_arg++){
    arg[numero_arg]=argv[numero_arg+3];
  }
  arg[argc]=NULL;


  /* jump to new prog : */
  sleep(1);
  if(execvp(argv[3],arg)==-1){
    perror("dsmwrap exec: ");
  }
  exit (0);
}
