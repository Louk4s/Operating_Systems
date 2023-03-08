/*************************************************************/
/* 						Program 1.C2.c  			         */
/* Synchronization of 5 system calls SC1, SC2, SC3, SC4, SC5 */
/* using a binary semaphore and two shared vars              */
/*************************************************************/

#include <stdio.h>          /* printf()                   */
#include <stdlib.h>         /* exit()                     */
#include <sys/types.h>      /* key_t, sem_t, pid_t        */
#include <sys/shm.h>        /* shmat(), IPC_RMID          */
#include <sys/wait.h>       /* waitpid()                  */
#include <semaphore.h>      /* sem_open(), sem_wait() ... */
#include <errno.h>          /* errno, ECHILD              */
#include <fcntl.h>          /* O_CREAT, O_EXEC            */


void initialize_shared ();
void detach_shared ();


typedef sem_t semaphore;
/* binary semaphore */
semaphore *s;

/* shared vars */
int *s4, *s5;  	   

int shmid1, shmid2; /* shared memory ids */


int main ()
{
    int   i;  /* loop variable */
    pid_t pid[5];  /* fork pids */
    int status;  /* child process status */  
	
    initialize_shared ();
	  
    /* fork child processes */                               
    for (i = 0; i < 5; i++)
    {
        pid[i] = fork ();

        if (pid[i] < 0)
    	{
       		printf ("\nFork error.\n");
       		return (-1);  
    	}
		else if (pid[i] == 0) 
            	 break;
    }


    if (pid[0] == 0)  /* SC1 */
    {
   		system("echo Hi there! >> output.txt");
	    sem_wait (s);
		(*s4)++;  
		sem_post (s);
    }
    else if (pid[1] == 0) /* SC2 */
    {
   		system("echo Have a nice day. >> output.txt"); 
	    sem_wait (s);
		(*s4)++;  
		sem_post (s);
    }
    else if (pid[2] == 0)  /* SC3 */
    { 
      	system("ls -l");
	   	sem_wait (s);
		(*s5)++;  
		sem_post(s);
    }
    else if (pid[3] == 0)  /* SC4 */
    {
		/* acquire a lock to shared var s4 */
		while (1)
		{
			sem_wait (s);  
			if ((*s4) <= 0)
			    /* release lock to s4 */
				sem_post (s);
			else		
				break;
		}

		system("cat output.txt"); 
		(*s5)++;  
		
		/* release lock to s4 */
		sem_post (s);		

    }
   	else if (pid[4] == 0)  /* SC5 */
    {
		while (1)
		{
			/* acquire a lock to shared var s4 */
			sem_wait (s);  
			if ((*s5) <= 0)
		        /* release lock to s5 */
				sem_post (s);
		    else
				break;
	    }
		system("rm output.txt"); 		
		
		/* release lock to s5 */
		sem_post (s);
    }
    else  /* Parent process */
    {
		for (i = 0; i < 5; i++)
		{
	    	pid_t wpid = waitpid(pid[i], &status, 0);  /* wait for child processes to end */

	    	if (wpid == -1) 
	  	    {
          	    perror ("\nWaitpid error.\n");
          	    status = -1;	
          	    return status;
      	    } 
			// if (WIFEXITED(status))  /* check if all child processes end normally */
                // printf("\nChild %d terminated with exit status %d.\n", wpid, WEXITSTATUS(status));
            // else
                // printf("\nChild: %d terminated abnormally.\n", wpid);
        }

		detach_shared();
        
        printf("\n\n");
    }

    exit (0);
}


/* initialize shared variables in shared memory */
void initialize_shared ()
{
	int i;
	
	shmid1 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);  
    shmid2 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT); 

    /* shared memory error check */
    if (shmid1 < 0 || shmid2 < 0) 
	{
        perror ("\nShmget error.\n");
        exit (1);
    }
    							       
    /* attach ss1, ss2 to shared memory */
    s4 = (int *) shmat (shmid1, NULL, 0);  
	s5 = (int *) shmat (shmid2, NULL, 0);
	
	/* initialize semaphore for shared processes */
    s = sem_open ("Sem", O_CREAT | O_EXCL, 0644, 1); 
	
	/* initialize shared vars */
	*s4 = -1;
	*s5 = -1;
}


/* shared memory detach and close semaphores */
void detach_shared ()
{
	/* shared memory detach */
    shmdt (s4);
    shmctl (shmid1, IPC_RMID, 0); 
    shmdt (s5);
    shmctl (shmid2, IPC_RMID, 0); 
   
    /* unlink and close semaphores */
    sem_unlink ("Sem");   
    sem_close (s);  
}
