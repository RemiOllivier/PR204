#include "common_impl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#define TAILLE_MAX 128
#define TAILLE_BUFFER 2048

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;
sig_atomic_t child_exit_status;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fprintf(stdout,"./bin/dsmexec machine_file ~/Desktop/PR204/Phase1/bin/truc arg1 arg 2...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
  /* on traite les fils qui se terminent */
  /* pour eviter les zombies */
  int status;
  waitpid(-1,&status,WNOHANG);
  child_exit_status=status;
  num_procs_creat--;
  printf("Processus fils zombie traité il reste %d processus en vie\n",num_procs_creat);

}


int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  }
  else {
    pid_t pid;
    int num_procs = 0;
    int i;
    int k;
    FILE *fichier=NULL;
    fichier= fopen(argv[1],"r");
    num_procs = compte_lignes(fichier);
    proc_array=malloc(num_procs*sizeof(dsm_proc_t));
    char **tableau;
    tableau = malloc(num_procs * sizeof(char*));
    for (i = 0; i < num_procs; i++) {
      tableau[i] = malloc(TAILLE_MAX);
    }
    char buffer[TAILLE_BUFFER];


    struct pollfd* fds;

    int* pipe_fd_out; // Tableau de fd de pipe
    int* pipe_fd_err;
    pipe_fd_out = malloc(num_procs*sizeof(int));
    pipe_fd_err = malloc(num_procs*sizeof(int));

    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    /* XXX.sa_handler = sigchld_handler; */
    struct sigaction siga;

    memset(&siga, 0, sizeof(struct sigaction));
    siga.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &siga, NULL);

    /* lecture du fichier de machines */
    /* 1- on recupere le nombre de processus a lancer */
    /* 2- on recupere les noms des machines : le nom de */
    /* la machine est un des elements d'identification */

    tableau=tableau_mot(tableau, fichier, num_procs);

    /* creation de la socket d'ecoute */
    /* + ecoute effective */
    int port_num=0;
    int sockfd = creer_socket(&port_num);

    /* creation des fils */
    for(i = 0; i < num_procs ; i++) {

      /* creation du tube pour rediriger stdout */
      int pipe_stdout[2];
      pipe(pipe_stdout);

      /* creation du tube pour rediriger stderr */
      int pipe_stderr[2];
      pipe(pipe_stderr);

      pid = fork();

      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */
        free(proc_array);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        /* redirection stdout */
        close(STDOUT_FILENO);
        dup(pipe_stdout[1]);
        close(pipe_stdout[1]);

        /* redirection stderr */
        close(STDERR_FILENO);
        dup(pipe_stderr[1]);
        close(pipe_stderr[1]);

        /* Creation du tableau d'arguments pour le ssh */
        int len=100;
        int numero_arg=3;
        char adresse[100];
        gethostname(adresse, len);
        char **arg=malloc((argc+10)*sizeof(char*));
        char *port=malloc(sizeof(char));
        sprintf(port, "%d",port_num);

        arg[0]="ssh";
        arg[1]=tableau[i];
        arg[2]="~/Desktop/PR204/Phase1/bin/dsmwrap";
        arg[3]=adresse;
        arg[4]=port;
        arg[5]=argv[2];

        for (numero_arg=3; numero_arg<argc; numero_arg++){
          arg[3+numero_arg]=argv[numero_arg];
        }

        arg[3+argc]=NULL;

        /* jump to new prog : */
        execvp("ssh",arg);
      }

      else  if(pid > 0) { /* pere */
        memset(proc_array[i].connect_info.machine_name, 0, 100*sizeof(char));
        proc_array[i].connect_info.rank=i;
        strcpy(proc_array[i].connect_info.machine_name,tableau[i]);
        proc_array[i].connect_info.sockfd=0;
        pipe_fd_out[i] = pipe_stdout[0];
        pipe_fd_err[i] = pipe_stderr[0];
        /* fermeture des extremites des tubes non utiles */
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        num_procs_creat++;
      }
    }

    num_procs = num_procs_creat;
    fds = malloc((2*num_procs_creat + 1) * sizeof(*fds));

    for(i = 0; i < 2*num_procs_creat; i++) {
      if(i < num_procs_creat) {
        fds[i].fd = pipe_fd_out[i];
        fds[i].events = POLLIN;
      }
      else {
        fds[i].fd = pipe_fd_err[i-num_procs_creat];
        fds[i].events = POLLIN;
      }
    }

    struct sockaddr_in sinclient;
    socklen_t len;
    len = sizeof(sinclient);
    for(i = 0; i < num_procs ; i++){
      /* on accepte les connexions des processus dsm */
      int connexion = accept(sockfd,(struct sockaddr*)&sinclient,&len);
      if(connexion<0){
        perror("server: accept");
      }


      /*  On recupere le nom de la machine distante */
      /* 1- d'abord la taille de la chaine */
      /* 2- puis la chaine elle-meme */

      char *nom_machine=malloc(100);
      memset(nom_machine, 0, 100*sizeof(char));
      if(do_read(connexion, nom_machine)==NULL){
        perror("server: read");
      }
      int j;

      for(j = 0; j < num_procs ; j++){
        if(strcmp(nom_machine,proc_array[j].connect_info.machine_name)==0 && proc_array[j].connect_info.sockfd==0){ //la condition est pas bonne
          break;
        }
      }

      proc_array[j].connect_info.sockfd=connexion;

      /* On recupere le pid du processus distant  */
      char* buf=malloc(100);
      memset(buf, 0, 100*sizeof(char));
      if(do_read(proc_array[j].connect_info.sockfd,buf)==NULL){
        perror("server: read");
      }

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      if(do_read(proc_array[j].connect_info.sockfd, buf)==NULL){
        perror("server: read");
      }
      proc_array[j].connect_info.port=atoi(buf);

    }

    char* buf=malloc(100);
    memset(buf, 0, 100*sizeof(char));
    for(k= 0; k < num_procs ; k++){
      sprintf(buf,"%d", num_procs);
      /* envoi du nombre de processus aux processus dsm*/
      do_write(proc_array[k].connect_info.sockfd, buf);
      /* envoi des rangs aux processus dsm */
      sprintf(buf,"%d", proc_array[k].connect_info.rank);
      do_write(proc_array[k].connect_info.sockfd, buf);

      /* envoi des infos de connexion aux processus */
      int j;
      for(j=0;j<k; j++){
        sprintf(buf,"%d", proc_array[j].connect_info.port);
        do_write(proc_array[k].connect_info.sockfd, buf);
        do_write(proc_array[k].connect_info.sockfd, proc_array[j].connect_info.machine_name);
      }
    }
    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */
    while(1)
    {
      poll(fds, 2*num_procs+1, -1);
      if(fds[2*num_procs].revents == POLLIN)
      break;

      for(i=0; i<num_procs; i++)
      {
        if(fds[i].revents == POLLIN)
        {
          memset(buffer, 0, sizeof(char)*TAILLE_BUFFER);
          read(fds[i].fd, buffer, sizeof(char)*TAILLE_BUFFER);
          if(fds[i].fd != 0 ){
            printf("[Processus %d : %s: stdout] %s\n", i, proc_array[i].connect_info.machine_name, buffer);
            fflush(stdout);
          }
        }
      }

      for(i=num_procs; i<2*num_procs; i++)
      {
        if(fds[i].revents == POLLIN)
        {
          memset(buffer, 0, sizeof(char)*TAILLE_BUFFER);
          read(fds[i].fd, buffer, sizeof(char)*TAILLE_BUFFER);
          if(fds[i].fd != 0 ){
            printf("[Proc %d : %s : stderr] %s", i-num_procs, proc_array[i-num_procs].connect_info.machine_name,buffer);
            fflush(stdout);
          }
        }
      }

      /*
      je recupere les infos sur les tubes de redirection
      jusqu'à ce qu'ils soient inactifs (ie fermes par les
      processus dsm ecrivains de l'autre cote ...)*/
    }

    /* on attend les processus fils */
    //dans sig child handler
    /* on ferme les descripteurs proprement */
    free(pipe_fd_out);
    free(pipe_fd_err);
    free(fds);
    free(buf);
    /* on ferme la socket d'ecoute */
    close(sockfd);
  }
  exit(EXIT_SUCCESS);
}
