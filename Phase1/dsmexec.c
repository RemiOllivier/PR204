#include "common_impl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#define TAILLE_MAX 128
#define TAILLE_BUFFER 1024

/* variables globales */
int pipe_exit[2];

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
  wait(NULL);
	num_procs_creat--;
  write(pipe_exit[1], 0, 1);
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
    char buffer[TAILLE_BUFFER]; 
    int r;
    struct sigaction siga;

  
		// Pour le poll
		struct pollfd* fds;

		int* pipe_fd_out; // Tableau de fd de pipe
		int* pipe_fd_err; // Tableau de fd de pipe
    pipe(pipe_exit);
    /* Mise en place d'un traitant pour recuperer les fils zombies*/
    /* XXX.sa_handler = sigchld_handler; */
    memset(&siga, 0, sizeof(struct sigaction));
		siga.sa_handler = sigchld_handler;
		siga.sa_flags = SA_RESTART;
		sigaction(17, &siga, NULL);

    /* lecture du fichier de machines */
    /* 1- on recupere le nombre de processus a lancer */
    /* 2- on recupere les noms des machines : le nom de */
    /* la machine est un des elements d'identification */
      
    tableau=tableau_mot(tableau, fichier, num_procs);


		pipe_fd_out = malloc(num_procs*sizeof(int));
		pipe_fd_err = malloc(num_procs*sizeof(int));

    /* creation de la socket d'ecoute */
    /* + ecoute effective */
    int port_num=0;
    int sockfd = creer_socket(&port_num);
    printf("port=%d\n", port_num);
    /* creation des fils */
    for(i = 0; i < num_procs ; i++) {
      int pipe_stdout[2];
      pipe(pipe_stdout);
      /* creation du tube pour rediriger stdout */

      int pipe_stderr[2];
      pipe(pipe_stderr);
     /* creation du tube pour rediriger stderr */

      pid = fork();
      //printf("pis:%d\n", pid);
      if(pid == -1) ERROR_EXIT("fork");

      if (pid == 0) { /* fils */
        free(proc_array);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        
            close(STDOUT_FILENO);
            int d=dup(pipe_stdout[0]);
            close(pipe_stdout[0]);
            close(STDERR_FILENO);
            fprintf(stdout,"%d:%u\n",i, d);
             /* redirection stdout */

            int ds=dup(pipe_stderr[0]);
            close(pipe_stderr[0]);
            fprintf(stdout,"%d:%u\n", i,ds);
            /* redirection stderr */

        /* Creation du tableau d'arguments pour le ssh */
        int len=100;
        int numero_arg=3;
        char adresse[100];
        gethostname(adresse, len);
        printf("%s\n", adresse);
        char **arg=malloc((argc+10)*sizeof(char*));
        char *port=malloc(sizeof(char));
        sprintf(port, "%d",port_num);
        printf("%d\n", port_num);
        arg[0]="ssh";
        arg[1]=tableau[i];
        arg[2]="~/Bureau/PR204/Phase1/bin/dsmwrap";
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
        printf("je suis pere\n");
        fflush(stdout);
        pipe_fd_out[i] = pipe_stdout[1];
				pipe_fd_err[i] = pipe_stderr[1];
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        /* fermeture des extremites des tubes non utiles */
        num_procs_creat++;
        //break;
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
		fds[i].fd = pipe_exit[0];
		fds[i].events = POLLIN;
  

    for(i = 0; i < num_procs ; i++){

      
      //wait(NULL);
      /* on accepte les connexions des processus dsm */
      struct sockaddr_in sinclient;
      socklen_t len;
      len = sizeof(sinclient);
      proc_array[i].connect_info.sockfd = accept(sockfd,(struct sockaddr*)&sinclient,&len);
      if(proc_array[i].connect_info.sockfd<0){
        perror("server: accept");
      }

      /*  On recupere le nom de la machine distante */
      /* 1- d'abord la taille de la chaine */
      /* 2- puis la chaine elle-meme */
      memset(proc_array[i].connect_info.machine_name, 0, 100*sizeof(char));
      if(do_read(proc_array[i].connect_info.sockfd, proc_array[i].connect_info.machine_name)==NULL){
        perror("server: read");
      }
      printf("machine name=%s\n", proc_array[i].connect_info.machine_name);
      fflush(stdout);

      /* On recupere le pid du processus distant  */
      char* buf=malloc(100);
      memset(buf, 0, 100*sizeof(char));
      if(do_read(proc_array[i].connect_info.sockfd,buf)==NULL){
        perror("server: read");
      }
      proc_array[i].pid=atoi(buf);
      printf("pid=%d\n", proc_array[i].pid);
      fflush(stdout);

      /* On recupere le numero de port de la socket */
      /* d'ecoute des processus distants */
      if(do_read(proc_array[i].connect_info.sockfd, buf)==NULL){
        perror("server: read");
      }
      fflush(stdout);
      proc_array[i].connect_info.port=atoi(buf);
      printf("port=%d\n", proc_array[i].connect_info.port);
      fflush(stdout);

    }

    /* envoi du nombre de processus aux processus dsm*/

    /* envoi des rangs aux processus dsm */


    /* envoi des infos de connexion aux processus */

    /* gestion des E/S : on recupere les caracteres */
    /* sur les tubes de redirection de stdout/stderr */
     while(1)
    {
      poll(fds, 2*num_procs_creat+1, -1);
			if(fds[2*num_procs_creat].revents == POLLIN)
				break;

			for(i=0; i<num_procs_creat; i++)
			{
			    if(fds[i].revents == POLLIN)
				{
			    	memset(buffer, 0, sizeof(char)*1024);
					r = read(pipe_fd_out[i], buffer, sizeof(char)*TAILLE_BUFFER);
					if(r == 0) {
						close(fds[i].fd);
						pipe_fd_out[i] = 0;
						fds[i].fd = 0;
					}
					// else if(fds[i].fd != 0 && fds[i].fd != pipe_exit[0]){
					// 	printf("[Proc %d : %s : stdout] %s", i, arg[i], buffer);
					// 	fflush(stdout);
					// }
				}
			}

			for(i=num_procs_creat; i<2*num_procs_creat; i++)
			{
				if(fds[i].revents == POLLIN)
				{
					memset(buffer, 0, sizeof(char)*1024);
					r = read(pipe_fd_err[i-num_procs_creat], buffer, sizeof(char)*TAILLE_BUFFER);
					if(r == 0) {
						close(fds[i].fd);
						pipe_fd_err[i] = 0;
						fds[i].fd = 0;
					}
					// else if(fds[i].fd != 0 && fds[i].fd != pipe_exit[0]){
					// 	printf("[Proc %d : %s : stderr] %s", i-num_procs_creat, arg[i-num_procs_creat], buffer);
					// 	fflush(stdout);
					// }
				}
			}
	
      /*
    je recupere les infos sur les tubes de redirection
    jusqu'à ce qu'ils soient inactifs (ie fermes par les
    processus dsm ecrivains de l'autre cote ...)*/
  }
  

  /* on attend les processus fils */

  /* on ferme les descripteurs proprement */
  free(pipe_fd_out);
  free(pipe_fd_err);
  free(fds);

  /* on ferme la socket d'ecoute */
  close(sockfd);
}
exit(EXIT_SUCCESS);
}
