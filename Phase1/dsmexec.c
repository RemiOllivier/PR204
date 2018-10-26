#include "common_impl.h"

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

int compte_lignes(FILE *fichier){
  rewind(fichier);
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

char tableau_mot(FILE *fichier, int n_ligne){
  rewind(fichier);
  int i;
  int taille_max=50;
  char chaine[taille_max];
  char tableau[n_ligne][taille_max];
  for(i=0;i<n_ligne;i++){
    fgets(chaine, taille_max, fichier);
    strcpy(tableau[i], chaine);
    printf("%s",tableau[i]);
  }
  fclose(fichier);
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
     fichier= fopen("machine file","r");
     numprocs = compte_lignes(fichier);
     rewind(fichier);
     char tableau[num_procs][taille_max];
     for(i=0;i<num_procs;i++){
       fgets(chaine, taille_max, fichier);
       strcpy(tableau[i], chaine);
     }
     fclose(fichier);

     /* Mise en place d'un traitant pour recuperer les fils zombies*/
     /* XXX.sa_handler = sigchld_handler; */

     /* lecture du fichier de machines */
     /* 1- on recupere le nombre de processus a lancer */
     /* 2- on recupere les noms des machines : le nom de */
     /* la machine est un des elements d'identification */

     /* creation de la socket d'ecoute */
     /* + ecoute effective */

     /* creation des fils */
     for(i = 0; i < num_procs ; i++) {

	/* creation du tube pour rediriger stdout */

	/* creation du tube pour rediriger stderr */

	pid = fork();
	if(pid == -1) ERROR_EXIT("fork");

	if (pid == 0) { /* fils */

	   /* redirection stdout */

	   /* redirection stderr */

	   /* Creation du tableau d'arguments pour le ssh */

	   /* jump to new prog : */
	   /* execvp("ssh",newargv); */

	} else  if(pid > 0) { /* pere */
	   /* fermeture des extremites des tubes non utiles */
	   num_procs_creat++;
	}
     }


     for(i = 0; i < num_procs ; i++){

	/* on accepte les connexions des processus dsm */

	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */

	/* On recupere le pid du processus distant  */

	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */
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
