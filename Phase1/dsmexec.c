#include "common_impl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#define TAILLE_MAX 128

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fprintf(stdout,"./bin/dsmexec machine_file ~/Documents/Enseirb/PR204/Phase1/bin/truc coucouco\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
  /* on traite les fils qui se terminent */
  /* pour eviter les zombies */
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
    printf("Nombre de processus:%d\n", num_procs);
    char **tableau;
    tableau = malloc(num_procs * sizeof(char*));
    for (i = 0; i < num_procs; i++) {
      tableau[i] = malloc(TAILLE_MAX);
    }

    tableau=tableau_mot(tableau, fichier, num_procs);


    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    /* XXX.sa_handler = sigchld_handler; */

    /* lecture du fichier de machines */
    /* 1- on recupere le nombre de processus a lancer */
    /* 2- on recupere les noms des machines : le nom de */
    /* la machine est un des elements d'identification */

    /* creation de la socket d'ecoute */
    /* + ecoute effective */
    int port_num=0;
    int sockfd = creer_socket(&port_num);
        /* creation des fils */
    for(i = 0; i < num_procs ; i++) {
      // int pipe_stdout[2];
      // pipe(pipe_stdout);
      // int pipe_stderr[2];
      // pipe(pipe_stderr);
      /* creation du tube pour rediriger stdout */

      /* creation du tube pour rediriger stderr */
printf("hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n");
fflush(stdout);
      pid = fork();
      //printf("pis:%d\n", pid);
      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */
        free(proc_array);
        // close(pipe_stdout[1]);
        // close(pipe_stderr[1]);
        //
        //     close(STDOUT_FILENO);
        //     int d=dup(pipe_stdout[0]);
        //     close(pipe_stdout[0]);
        //     close(STDERR_FILENO);
        //     fprintf(stdout,"%d:%u\n",i, d);
        //     int ds=dup(pipe_stderr[0]);
        //     close(pipe_stderr[0]);
        //     fprintf(stdout,"%d:%u\n", i,ds);


        //get the socket
        /* redirection stdout */

        /* redirection stderr */

        /* Creation du tableau d'arguments pour le ssh */
        int len=100;
        int numero_arg=3;
        char adresse[100];
        gethostname(adresse, len);
        //printf("%s\n", adresse);
        char **arg=malloc((argc+10)*sizeof(char*));
        char *port=malloc(sizeof(char));
        sprintf(port, "%d",port_num);
        //printf("%d\n", port_num);
        arg[0]="ssh";
        arg[1]=tableau[i];
        arg[2]="~/Documents/Enseirb/PR204/Phase1/bin/dsmwrap";
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
        //printf("je suis pere\n");
        fflush(stdout);
        // close(pipe_stdout[0]);
        // close(pipe_stderr[0]);
        /* fermeture des extremites des tubes non utiles */
        num_procs_creat++;
        
      }
    }
    struct sockaddr_in sinclient;
    socklen_t len;
    len = sizeof(sinclient);
    for(i = 0; i < num_procs ; i++){
      //wait(NULL);
      /* on accepte les connexions des processus dsm */
      proc_array[i].connect_info.sockfd = accept(sockfd,(struct sockaddr*)&sinclient,&len);
      printf("cccccccccccccccccccc\n");
      fflush(stdout);
      if(proc_array[i].connect_info.sockfd<0){
        perror("server: accept");
      }

      /*  On recupere le nom de la machine distante */
      /* 1- d'abord la taille de la chaine */
      /* 2- puis la chaine elle-meme */
      proc_array[i].connect_info.rank=i;
      memset(proc_array[i].connect_info.machine_name, 0, 100*sizeof(char));
      if(do_read(proc_array[i].connect_info.sockfd, proc_array[i].connect_info.machine_name)==NULL){
        perror("server: read");
      }
      //printf("machine name=%s\n", proc_array[i].connect_info.machine_name);
      fflush(stdout);

      /* On recupere le pid du processus distant  */
      char* buf=malloc(100);
      memset(buf, 0, 100*sizeof(char));
      if(do_read(proc_array[i].connect_info.sockfd,buf)==NULL){
        perror("server: read");
      }
      proc_array[i].pid=atoi(buf);
    //  printf("pid=%d\n", proc_array[i].pid);
      fflush(stdout);

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      if(do_read(proc_array[i].connect_info.sockfd, buf)==NULL){
        perror("server: read");
      }
      fflush(stdout);
      proc_array[i].connect_info.port=atoi(buf);
      //printf("portttttt=%d\n", proc_array[i].connect_info.port);
      fflush(stdout);

    }
    char* buf=malloc(100);
    sprintf(buf,"%d", num_procs);

for(k= 0; k < num_procs ; k++){
    /* envoi du nombre de processus aux processus dsm*/
    do_write(proc_array[k].connect_info.sockfd, buf);
    /* envoi des rangs aux processus dsm */
    sprintf(buf,"%d", proc_array[k].connect_info.rank);
    do_write(proc_array[k].connect_info.sockfd, buf);

    /* envoi des infos de connexion aux processus */
    int j;
    for(j=0;j<num_procs; j++){
    sprintf(buf,"%d", proc_array[j].connect_info.port);
    do_write(proc_array[k].connect_info.sockfd, buf);
    do_write(proc_array[k].connect_info.sockfd, proc_array[j].connect_info.machine_name);
  }
    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */
  }
    /* while(1)
    {
    je recupere les infos sur les tubes de redirection
    jusqu'à ce qu'ils soient inactifs (ie fermes par les
    processus dsm ecrivains de l'autre cote ...)
  };
  */

  /* on attend les processus fils */

  /* on ferme les descripteurs proprement */

  /* on ferme la socket d'ecoute */
}
exit(EXIT_SUCCESS);
}
