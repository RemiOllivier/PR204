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
  } else {
     pid_t pid;
     int num_procs = 0;
     int i;
     FILE *fichier=NULL;
     fichier= fopen("machine_file","r");
     num_procs = compte_lignes(fichier);
     proc_array=malloc(num_procs*sizeof(dsm_proc_t));
     printf("%d\n", num_procs);
     char **tableau;
     tableau = malloc(num_procs * TAILLE_MAX);
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
     int prop=1;
     int port_num=0;
     int nb_connexions=20;
     int sockfd = creer_socket(&port_num);
printf("port:%d\n", port_num);
     /* creation des fils */
     for(i = 0; i < num_procs ; i++) {
       // int pipe_stdout[2];
       // pipe(pipe_stdout);
       // int pipe_stderr[2];
       // pipe(pipe_stderr);
	/* creation du tube pour rediriger stdout */

	/* creation du tube pour rediriger stderr */

	pid = fork();
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

	   /* jump to new prog : */
	   /* execvp("ssh",newargv); */
     int len=100;
     char adresse[100];
     gethostname(adresse, len);
     printf("%s\n", adresse);
     char **arg=malloc(5*sizeof(char*));
     char *port=malloc(sizeof(char));
     sprintf(port, "%d", ntohs(port_num));
printf("%d\n", port_num);
     arg[0]="ssh";
     arg[1]=tableau[i];
     arg[2]="~/Documents/Enseirb/PR204/Phase1/bin/dsmwrap";
     arg[3]=adresse;
     arg[4]=port;
     arg[5]=NULL;

    execvp("ssh",arg);

	} else  if(pid > 0) { /* pere */
    // close(pipe_stdout[0]);
    // close(pipe_stderr[0]);
	   /* fermeture des extremites des tubes non utiles */
	   num_procs_creat++;
	}
     }

     for(i = 0; i < num_procs ; i++){
wait(NULL);
	/* on accepte les connexions des processus dsm */
  struct sockaddr_in sinclient={0};


  socklen_t len;
  len = sizeof(sinclient);
  proc_array[i].connect_info.sockfd = accept(sockfd,(struct sockaddr*)&sinclient,&len);
  if(proc_array[i].connect_info.sockfd<0){
    perror("server: accept");
  }
	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */
  // int len_socket =100;
  // if(read(proc_array[i].connect_info.sockfd, proc_array[i].connect_info.machine_name,len_socket)<0){
  //   perror("server: read");
  // }

	/* On recupere le pid du processus distant  */
  // if(read(proc_array[i].connect_info.sockfd, proc_array[i].pid,len_socket)<0){
  //   perror("server: read");
  // }
	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */
  // if(read(proc_array[i].connect_info.sockfd, proc_array[i].connect_info.port,len_socket)<0){
  //   perror("server: read");
  // }
     }

     /* envoi du nombre de processus aux processus dsm*/

     /* envoi des rangs aux processus dsm */

     /* envoi des infos de connexion aux processus */

     /* gestion des E/S : on recupere les caracteres */
     /* sur les tubes de redirection de stdout/stderr */
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
