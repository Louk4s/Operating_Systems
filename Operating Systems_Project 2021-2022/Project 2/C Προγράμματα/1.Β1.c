/**********************************************************/
/* 					 Program 1.B1.c  			          */
/* Readers / writers problem using fork() child processes */
/* and shared memory                                      */
/**********************************************************/

#include <stdio.h>          /* printf()                   */
#include <stdlib.h>         /* exit()                     */
#include <sys/types.h>      /* key_t, sem_t, pid_t        */
#include <sys/shm.h>        /* shmat(), IPC_RMID          */
#include <sys/wait.h>       /* waitpid()                  */
#include <semaphore.h>      /* sem_open(), sem_wait() ... */
#include <errno.h>          /* errno, ECHILD              */
#include <fcntl.h>          /* O_CREAT, O_EXEC            */
#include <time.h>		    /* time()                     */


#define N 10  /* total number of readers / writers */


void initialize_shared ();
void detach_shared ();
void reader ();
void writer ();


typedef sem_t semaphore;  /* binary semaphore */
semaphore *cSem, *dataSem;

int *rc, *wc, *priority;  /* shared vars */	
int *data;  /* shared data value accessed by readers and updated by writers */

int shmid1, shmid2, shmid3, shmid4;  /* shared memory ids */


int main ()
{
	pid_t w; 
	int status;

	pid_t pid[N];  /* fork pids */			  
    int i;	/* counter var */
    
	initialize_shared ();
	
	/* Use current time as seed for random generator */
	srand(time(0)); 
	
	printf("\n\n");
		
	/* creation of readers / writers processes */
	for (i = 0; i < N; i++)
    {
    	pid[i] = fork ();  /* creation of child processes */
    	
		if (pid[i] < 0)
    	{
       		printf ("\nFork error.\n");
       		return (-1);  
    	}
    	else if (pid[i] == 0)  /* child process */
			 {
				usleep(rand()%100);
				if (i%2 == 0)
				    reader (i);
				else 
				    writer (i);			
				exit (0);
		     }		
	}
     
     
   /* parent process */
   for (i = 0; i < N; i++) 
   {
      	w = waitpid(pid[i], &status, WUNTRACED | WCONTINUED);   /* wait all child processes to exit */
      	if (w == -1) 
	  	{
          	perror ("\nWaitpid error.\n");
          	status = -1;	
          	return status;
      	}
      	// if (WIFEXITED(status))  /* check if all child processes end normally */
           // printf("\nChild %d terminated with exit status %d.\n", w, WEXITSTATUS(status));
        // else
           // printf("\nChild %d terminated abnormally.\n", w);
   }  
	
   detach_shared();

   printf("\n\n");
   exit (0);
} 


/* Initialize shared variables in shared memory */
void initialize_shared ()
{
	shmid1 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);  
    shmid2 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);
    shmid3 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);
    shmid4 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);

    /* shared memory error check */
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0 || shmid4 < 0) 
	{
        perror ("\nShmget error.\n");
        exit (1);
    }
    							       
    /* attach rc, wc, priority to shared memory */
    rc = (int *) shmat (shmid1, NULL, 0);  
	wc = (int *) shmat (shmid2, NULL, 0);
	priority = (int *) shmat (shmid3, NULL, 0);
	
	/* attach data to shared memory */
	data = (int *) shmat (shmid4, NULL, 0);
    	
	/* initialize semaphores for shared processes */
    cSem = sem_open ("cSem", O_CREAT | O_EXCL, 0644, 1); 
    dataSem = sem_open ("dataSem", O_CREAT | O_EXCL, 0644, 1); 
    
    /* initialize shared vars */
    *rc = 0; *wc = 0; *priority = 1; *data = 0; 
}


/* shared memory detach and close semaphores */
void detach_shared ()
{
	/* shared memory detach */
   shmdt (rc);
   shmctl (shmid1, IPC_RMID, 0); 
   shmdt (wc);
   shmctl (shmid2, IPC_RMID, 0); 
   shmdt (priority);
   shmctl (shmid3, IPC_RMID, 0); 
   shmdt (data);
   shmctl (shmid4, IPC_RMID, 0); 
   
   /* unlink and close semaphores */
   sem_unlink ("cSem");   
   sem_close (cSem);  
   sem_unlink ("dataSem");
   sem_close (dataSem); 
}


/* reader process */
void reader (int k)
{
   
   int i;
   
   for (i = 0; i < 5; i++)
   {
	  sem_wait (cSem);  /* DOWN operation */
	  (*rc)++;
	  sem_post (cSem);  /* UP operation */
	  sem_wait (dataSem);
	  if (*priority == 0)
	  {
	      printf("Reader %d reads %d at try %d\n", k, *data, i);   
		  sem_wait (cSem);
		  (*rc)--;
		  if (*wc != 0)
		      *priority = 1;  /* enable a waiting writer to proceed */
		  sem_post (cSem);
      }
	  else
	  {
	      sem_wait (cSem);
	      if (*wc == 0)
		      *priority = 0;  /* enable a waiting reader to proceed */
		  (*rc)--;
		  sem_post (cSem); 
      } 
      sem_post (dataSem);
      
	  usleep(rand()%100);
   }
}


/* writer process */
void writer (int k)
{
   
   int i;
   
   for (i = 0; i < 5; i++)
   {
	  sem_wait (cSem);  /* DOWN operation */
	  (*wc)++;
	  sem_post (cSem);  /* UP operation */
	  sem_wait (dataSem);
	  if (*priority == 1)
	  {
	      (*data)++;
		  printf("Writer %d writes %d at try %d\n", k, *data, i);   
		  sem_wait(cSem);
		  (*wc)--;
		  if (*rc != 0)
		      *priority = 0;  /* enable a waiting reader to proceed */
		  sem_post (cSem);
      }
	  else
	  {
	      sem_wait (cSem);
	      if (*rc == 0)
		      *priority = 1;  /* enable a waiting writer to proceed */
		  (*wc)--;
		  sem_post (cSem); 
      } 
      sem_post (dataSem);
      
	  usleep(rand()%100);
   }
}


