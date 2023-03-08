/**********************************************************/
/* 					 Program 1.A.A.c  			          */
/*  Sum shared min heap root values to a shared variable  */
/**********************************************************/

#include <stdio.h>          /* printf()                   */
#include <stdlib.h>         /* exit()                     */
#include <sys/types.h>      /* key_t, sem_t, pid_t        */
#include <sys/shm.h>        /* shmat(), IPC_RMID          */
#include <sys/wait.h>       /* waitpid()                  */
#include <semaphore.h>      /* sem_open(), sem_wait() ... */
#include <errno.h>          /* errno, ECHILD              */
#include <fcntl.h>          /* O_CREAT, O_EXEC            */
#include <time.h>		    /* clock()                    */

#define N 10  /* number of child processes */


void initialize_shared ();
void detach_shared ();
void child_process ();
void swap ();
void shift_down ();


typedef sem_t semaphore;  /* binary semaphore */
semaphore *s;

int *p;  /* shared var for the sum */	
int *root;  /* shared min Heap */ 
int *last;  /* index of the last element in shared min heap to be summed up*/       	

int shmid1, shmid2, shmid3; /* shared memory ids */


int main ()
{
  	clock_t start_t, end_t, total_t;  /* start and end clock ticks vars */
	pid_t w; 
	int status;

	pid_t pid[N];  /* fork pids */			  
    int i;	/* counter var */
						       
    initialize_shared();

    srand(time(0));  /* use current time as seed for random generator */
	
	start_t = clock(); /* start time */
    
	printf("\n\nInitial value of shared variable p: %d\n\n", *p);
	
	for (i = 0; i < N; i++)
    {
    	pid[i] = fork();  /* creation of child processes */
    	
		if (pid[i] < 0)
    	{
       		printf ("\nFork error.\n");
       		return (-1);  
    	}
    	else if (pid[i] == 0)  /* child process */
			 {
				child_process (i+1);			
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
           // printf("\nChild %d terminated abnormally.\n", w); */
    }  
  
   	end_t = clock();  /* end time */
    total_t = end_t - start_t;  /* total clock ticks (CPU cycles) */
   
	printf("\n\nFinal value of shared variable p = %d", *p);
	printf("\n\nTotal cycles taken by CPU: %lg", total_t);	
	printf("\nTotal time taken by CPU: %lg secs\n\n\n", (double) total_t/CLOCKS_PER_SEC);	
		
    detach_shared ();
    
    printf("\n\n");
	exit (0);
} 


/* initialize shared variables in shared memory */
void initialize_shared ()
{
	int i;
	
	shmid1 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);  
    shmid2 = shmget (IPC_PRIVATE, sizeof (int) *10*N, 0644 | IPC_CREAT); /* min heap is implemented as an array of 10*N int values accessible by root pointer */
    shmid3 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);

    /* shared memory error check */
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0) 
	{
        perror ("\nShmget error.\n");
        exit (1);
    }
    							       
    /* attach p, root, last to shared memory */
    p = (int *) shmat (shmid1, NULL, 0);  
	root = (int *) shmat (shmid2, NULL, 0);
	last = (int *) shmat (shmid3, NULL, 0);
    
    
	*p = 0;  /* initialize shared var p to 0 */
		
	/* initialize semaphore s for shared processes */
    s = sem_open ("Sem", O_CREAT | O_EXCL, 0644, 1); 
	
	/* store numbers 1..10*N to heap array - the heap array initially is a min heap data structure */
	for (i = 0; i < 10*N; i++)
	     *(root + i) = i + 1; 
	     
	*last = 10*N;  /* initialize shared var last to 10*N (total number of heap elements to be summed up) */
}


/* shared memory detach and close semaphores */
void detach_shared ()
{
	/* shared memory detach */
    shmdt (p);
    shmctl (shmid1, IPC_RMID, 0); 
    shmdt (root);
    shmctl (shmid2, IPC_RMID, 0); 
    shmdt (last);
    shmctl (shmid3, IPC_RMID, 0); 
   
    /* unlink and close semaphore */
    sem_unlink ("Sem");   
    sem_close (s);  
}


/* sum the value pointed by heap root to shared variable p, move this value to end of heap and reconstruct the heap using function shift_down() */
void child_process (int k)
{
   
    while (1)
    {
	   sem_wait (s);  /* DOWN operation */
	   if ((*last) == 0)  /* all the 10*N values are summed up - child process must terminate */ 
	   {
	      sem_post (s);  /* UP operation */
		  break;
	   } 
	   else
	   {
	      *p = *p + *root;
	      printf ("Process %d adds %d: p = %d, ", k, *root, *p);
	  	  swap (root, root + *last - 1);
	  	  (*last)--;
		  shift_down (root, *last); 
		  sem_post (s);
       } 
       usleep (rand()%100);  /* child process sleeps for a while between two successive summations */
    }
}


/* swap the contents of two int values */
void swap (int *a, int *b)
{
    int temp;

    temp = *a;
    *a = *b;
    *b = temp;
}


/* shift down the heap root element in order to satisfy the heap property */
void shift_down (int *root, int last)
{
 	int j, k;  /* heap array indexes */
 	int v;  /* heap root element */

 	v = *root;
 	k = 1;

    while (k <= last/2)
 	{
        j = 2*k;
        if (j < last)
        {
            if (*(root + j) < *(root + j-1)) 
                j++;
        }
        if (v <= *(root + j-1))  
            break;
        *(root + k-1) = *(root + j-1);  /* shift down */
        k = j;
  	}
  	*(root + k-1) = v;  /* root element is stored at the right place in the min heap */
}

