/**********************************************************/
/* 					 Program 1.B2.c  			          */
/* Readers / writers problem using threads and globally   */
/* accessible (and thus shared) memory                    */
/**********************************************************/

#include <stdio.h>          /* printf()                   */
#include <stdlib.h>         /* exit()                     */
#include <sys/types.h>      /* key_t, sem_t, pid_t        */
#include <semaphore.h>      /* sem_open(), sem_wait() ... */
#include <errno.h>          /* errno, ECHILD              */
#include <fcntl.h>          /* O_CREAT, O_EXEC            */
#include <time.h>		    /* time()                     */


#define N 10  /* total number of readers / writers */


void initialize_shared ();
void close_semaphores ();
void *reader ();
void *writer ();


typedef sem_t semaphore;  /* binary semaphore */
semaphore *cSem, *dataSem;

int rc, wc, priority;  /* globally accessible (shared) vars */	
int data;  /* shared data value accessed by readers and updated by writers */


int main ()
{
	pthread_t tid[N];  /* thread pids */			  
    int i;	/* counter var */
    int param[N];  /* array of parameters passed to each reader() / writer() thread */ 
	int err, status;  /* thread join error and status */  
	
	initialize_shared ();
	
	/* Use current time as seed for random generator */
	srand(time(0)); 
	
	printf("\n\n");
		
	/* creation of readers / writers threads */
	for (i = 0; i < N; i++)
	{
		param[i] = i;
		usleep(rand()%100);
		if (i%2 == 0)
			pthread_create (&tid[i], NULL, &reader, &param[i]);   	 
		else 
			pthread_create (&tid[i], NULL, &writer, &param[i]);  			
	}  
    	
        
   /* wait for all threads to terminate */ 
   for (i = 0; i < N; i++) 
   {    
	   err = pthread_join (tid[i], (void **) &status);
	   if (err != 0)
	 	{  
			perror ("\nThread join error.\n"); 
			status = -1;	
        	return status;
     	} 
   
   }
   
   close_semaphores();
   
   printf("\n\n");
   exit (0);
} 


/* Initialize shared variables in shared memory */
void initialize_shared ()
{   	
	/* initialize semaphores for shared threads */
    cSem = sem_open ("cSem", O_CREAT | O_EXCL, 0644, 1); 
    dataSem = sem_open ("dataSem", O_CREAT | O_EXCL, 0644, 1); 
    
    /* initialize global vars */
    rc = 0; wc = 0; priority = 1; data = 0; 
}


/* shared memory detach and close semaphores */
void close_semaphores ()
{
   /* unlink and close semaphores */
   sem_unlink ("cSem");   
   sem_close (cSem);  
   sem_unlink ("dataSem");
   sem_close (dataSem); 
}


/* reader thread */
void *reader (void *k)
{
   
   int i;
   
   for (i = 0; i < 5; i++)
   {
	  sem_wait (cSem);  /* DOWN operation */
	  rc++;
	  sem_post (cSem);  /* UP operation */
	  sem_wait (dataSem);
	  if (priority == 0)
	  {
	      printf("Reader %d reads %d at try %d\n", *((int *) k), data, i);   
		  sem_wait (cSem);
		  rc--;
		  if (wc != 0)
		      priority = 1;  /* enable a waiting writer to proceed */
		  sem_post (cSem);
      }
	  else
	  {
	      sem_wait (cSem);
	      if (wc == 0)
		      priority = 0;  /* enable a waiting reader to proceed */
		  (rc)--;
		  sem_post (cSem); 
      } 
      sem_post (dataSem);
      
	  usleep(rand()%100);
   }
   
   pthread_exit (0);
}


/* writer thread */
void *writer (void *k)
{
   
   int i;
   
   for (i = 0; i < 5; i++)
   {
	  sem_wait (cSem);  /* DOWN operation */
	  wc++;
	  sem_post (cSem);  /* UP operation */
	  sem_wait (dataSem);
	  if (priority == 1)
	  {
	      data++;
		  printf("Writer %d writes %d at try %d\n", *((int *) k), data, i);   
		  sem_wait(cSem);
		  wc--;
		  if (rc != 0)
		      priority = 0;  /* enable a waiting reader to proceed */
		  sem_post (cSem);
      }
	  else
	  {
	      sem_wait (cSem);
	      if (rc == 0)
		      priority = 1;  /* enable a waiting writer to proceed */
		  (wc)--;
		  sem_post (cSem); 
      } 
      sem_post (dataSem);
      
	  usleep(rand()%100);
   }
   
   pthread_exit (0);
}


