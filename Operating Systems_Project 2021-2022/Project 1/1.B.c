/*********************************************************/
/* 						Program 1.B.c  			         */
/* Max value of shared variable X - Peterson's algorithm */
/*********************************************************/

#include <stdio.h>       	/* printf()              	 */
#include <stdlib.h>      	/* exit()                	 */
#include <sys/types.h>   	/* key_t, sem_t, pid_t 		 */
#include <sys/shm.h>     	/* shmat(), IPC_RMID      	 */
#include <sys/wait.h>   	/* waitpid()              	 */
#include <errno.h>       	/* errno, ECHILD         	 */
#include <fcntl.h>       	/* O_CREAT, O_EXEC        	 */


void child_process_i ();
void enter_region ();
void leave_region ();


int *pX;  /* shared var */	
int *pturn;  /* Peterson's algorithm shared vars */
int *pinterested;         	


int main ()
{
  	pid_t w; 
	int status;

	pid_t pid[10];	/* fork pids */			  
    int i;  /* counter var */
	
							       
    /* Initialize shared variables in shared memory */

	int shmid1, shmid2, shmid3;  /* shared memory ids */
	
	shmid1 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);  
    shmid2 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);
    shmid3 = shmget (IPC_PRIVATE, sizeof (int) * 2, 0644 | IPC_CREAT);

    /* shared memory error check */
    if (shmid1 < 0 || shmid2 < 0 || shmid3 < 0) 
	{
        perror ("\nShmget error.\n");
        exit (1);
    }
    							       
    /* attach pX, pturn, pinterested to shared memory */
    pX = (int *) shmat (shmid1, NULL, 0);  
	pturn = (int *) shmat (shmid2, NULL, 0);
	pinterested = (int *) shmat (shmid3, NULL, 0); 
    
	/* initialize shared variables */
	*pX = 0; 
	for (i = 0; i < 2; i++)
	    *(pinterested + i) = 0; 
    
	printf("\n\nInitial value of shared variable X: %d\n\n", *pX);


    for (i = 0; i < 2; i++)
    {
    	pid[i] = fork ();  /* creation of child processes */
    	
		if (pid[i] < 0)
    	{
       		printf ("\nFork error.\n");
       		return (-1);  
    	}
    	else if (pid[i] == 0)  /* child process */
			 {
				 child_process_i (i);
				 exit (0);
			 }		
	}
     
     
   	/* parent process */
   	for (i = 0; i < 2; i++) 
   	{
     	/* wait both child processes to exit */
		w = waitpid(pid[i], &status, WUNTRACED | WCONTINUED);   
      	if (w == -1) 
	  	{
          		perror ("\nWaitpid error.\n");
          		status = -1;	
          		return status;
      	}
      	// if (WIFEXITED(status))  /* check if the two child processes end normally */
            // printf("\n\nChild %d terminated with exit status %d.\n", w, WEXITSTATUS(status));
        // else
            // printf("\n\nChild %d terminated abnormally.\n", w); 
   	}  
  
  
   	printf ("\n\nFinal value of shared variable X = %d\n\n\n", *pX);
		
		
   	/* shared memory detach */
   	shmdt (pX);
   	shmctl (shmid1, IPC_RMID, 0); 
   	shmdt (pturn);
   	shmctl (shmid2, IPC_RMID, 0); 
   	shmdt (pinterested);
   	shmctl (shmid3, IPC_RMID, 0); 
   
   	exit (0);
} 


/* increment by 1 shared variable X for 500 times */
void child_process_i (int p)
{
   	int tmp, i;  /* local vars */

   	for (i = 1; i <= 500; i++)
   	{
	  	enter_region (p);
		tmp = *pX;
	  	tmp = tmp + 1;
	    *pX = tmp;
		printf("Process %d: %d, ", p, *pX);
		leave_region (p);
   	}
}


/* enter_region() of Peterson's algorithm */
void enter_region (int process)
{
	int other = 1 - process;
	*(pinterested + process) = 1;
	*pturn = process;
	
	while ((*pturn == process) && (*(pinterested + other) == 1))
	   	;
}
 

/* leave_region() of Peterson's algorithm */
void leave_region (int process)
{
	*(pinterested + process) = 0;
}

