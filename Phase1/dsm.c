#include "dsm.h"
#include "common_impl.h"

#include <poll.h>

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */

dsm_proc_t * proc_array;
/* indique l'adresse de debut de la page de numero numpage */
static char *num2address( int numpage )
{
  char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));

  if( pointer >= (char *)TOP_ADDR ){
    fprintf(stderr,"[%i] Invalid address !\n", DSM_NODE_ID);
    return NULL;
  }
  else return pointer;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info( int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
  if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {
    if (state != NO_CHANGE )
    table_page[numpage].status = state;
    if (owner >= 0 )
    table_page[numpage].owner = owner;
    return;
  }
  else {
    fprintf(stderr,"[%i] Invalid page number !\n", DSM_NODE_ID);
    return;
  }
}

static dsm_page_owner_t get_owner( int numpage)
{
  return table_page[numpage].owner;
}

static dsm_page_state_t get_status( int numpage)
{
  return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page( int numpage )
{
  char *page_addr = num2address( numpage );
  mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return ;
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
  char *page_addr = num2address( numpage );
  mprotect(page_addr, PAGE_SIZE, prot);
  return;
}

static void dsm_free_page( int numpage )
{
  char *page_addr = num2address( numpage );
  munmap(page_addr, PAGE_SIZE);
  return;
}

static void *dsm_comm_daemon( void *arg)
{
  struct pollfd *fds = malloc(sizeof(struct pollfd)*DSM_NODE_NUM);
  char* buf=malloc(100);
  memset(buf, 0, 100*sizeof(char));
  int j=0;
  for(int j=0; j<DSM_NODE_NUM; j++){
    fds[j].fd=proc_array[j].connect_info.sockfd;
  }
  while(1){
    printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
    sleep(2);
    poll(fds, DSM_NODE_NUM ,0);
    for(j=1; j<DSM_NODE_NUM; j++){
      if(fds[j].revents==POLLIN){
        do_read(fds[j].fd, buf);
        printf("%s", buf);
        fflush(stdout);
      }
    }

  }
  return NULL;
}


/* retrouver à partir d'une adresse un numéro de page*/

int address2num (char* addr)
{
  return (((long int)(addr-BASE_ADDR))/(PAGE_SIZE));

}


static int dsm_send(int dest,void *buf,size_t size)
{
  return 0;
  /* a completer */
}

static int dsm_recv(int from,void *buf,size_t size)
{
  return 0;
  /* a completer */
}

static void dsm_handler( void )
{
  /* A modifier */
  printf("[%i] FAULTY  ACCESS !!! \n",DSM_NODE_ID);
  abort();
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
  /* A completer */
  /* adresse qui a provoque une erreur */
  void  *addr = info->si_addr;
  /* Si ceci ne fonctionne pas, utiliser a la place :*/
  /*
  #ifdef __x86_64__
  void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
  #elif __i386__
  void *addr = (void *)(context->uc_mcontext.cr2);
  #else
  void  addr = info->si_addr;
  #endif
  */
  /*
  pour plus tard (question ++):
  dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
  */
  /* adresse de la page dont fait partie l'adresse qui a provoque la faute */
  void  *page_addr  = (void *)(((unsigned long) addr) & ~(PAGE_SIZE-1));

  if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR))
  {
    dsm_handler();
  }
  else
  {
    /* SIGSEGV normal : ne rien faire*/
  }
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int sockfd, int socket_ecoute)
{
  struct sigaction act;
  int index;
  struct sockaddr_in sin;

  /* reception du nombre de processus dsm envoye */
  /* par le lanceur de programmes (DSM_NODE_NUM)*/
  char* buf=malloc(100);
  memset(buf, 0, 100*sizeof(char));
  if(do_read(sockfd, buf)==NULL){
    perror("server: read");
  }
  DSM_NODE_NUM=atoi(buf);

  /* reception de mon numero de processus dsm envoye */
  /* par le lanceur de programmes (DSM_NODE_ID)*/
  if(do_read(sockfd, buf)==NULL){
    perror("server: read");
  }
  DSM_NODE_ID=atoi(buf);
  /* reception des informations de connexion des autres */
  /* processus envoyees par le lanceur : */
  /* nom de machine, numero de port, etc. */
  int j;
  proc_array=malloc(DSM_NODE_NUM*sizeof(dsm_proc_t));
  for(j=0; j<DSM_NODE_ID; j++){
    proc_array[j].connect_info.rank=j;
    if(do_read(sockfd, buf)==NULL){
      perror("server: read");
    }
    proc_array[j].connect_info.port=atoi(buf);
    memset(proc_array[j].connect_info.machine_name, 0, 100*sizeof(char));
    if(do_read(sockfd,  proc_array[j].connect_info.machine_name)==NULL){
      perror("server: read");
    }
  }

  for(j=0; j<DSM_NODE_NUM;j++){
    proc_array[j].connect_info.sockfd = malloc(sizeof(int));
    proc_array[j].connect_info.sockfd = -1;
  }

  /* initialisation des connexions */
  /* avec les autres processus : connect/accept */
  for(j=0; j<DSM_NODE_ID;j++){
    int c=-1;
    sin.sin_port=htons(proc_array[j].connect_info.port);
    sin.sin_family=AF_INET;
    inet_aton(hostname_to_ip(proc_array[j].connect_info.machine_name), &sin.sin_addr);
    proc_array[j].connect_info.sockfd = socket(AF_INET, SOCK_STREAM,0);
    while(c<0){
      c=connect(proc_array[j].connect_info.sockfd,(struct sockaddr*)&sin,sizeof(sin));
    }
    sprintf(buf,"%d", DSM_NODE_ID);
    do_write(proc_array[j].connect_info.sockfd, buf);
  }

  int connexion=0;
  for(j=DSM_NODE_ID+1; j<DSM_NODE_NUM;j++){
    connexion=0;
    struct sockaddr_in sinclient;
    socklen_t len;
    len = sizeof(sinclient);

    while(connexion <=0){
      connexion =  accept(socket_ecoute,(struct sockaddr*)&sinclient,&len);
    }
    char *rank=malloc(10*sizeof(char));
    if(do_read(connexion,  rank)==NULL){
      perror("server: read");
    }
    int k=atoi(rank);
    proc_array[k].connect_info.sockfd=connexion;
  }
  close(socket_ecoute);

  bzero(buf,100);
  j=0;
  for (j=0; j<DSM_NODE_NUM; j++) {
    if (j != DSM_NODE_ID){
      sprintf(buf,"Salut %d je suis %d", j, DSM_NODE_ID);
      do_write(proc_array[j].connect_info.sockfd, buf);
    }
  }

  bzero(buf,100);
  for (j=0; j< DSM_NODE_NUM; j++) {
    if (j!= DSM_NODE_ID) {
      if(do_read(proc_array[j].connect_info.sockfd, buf)==NULL){
        perror("server: read");
      }
      fprintf(stdout, "%s\n", buf);
      fflush(stdout);
    }
  }

  pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
  return NULL;
}

void dsm_finalize( void )
{
  /* fermer proprement les connexions avec les autres processus */

  /* terminer correctement le thread de communication */
  /* pour le moment, on peut faire : */
  pthread_cancel(comm_daemon);

  return;
}
