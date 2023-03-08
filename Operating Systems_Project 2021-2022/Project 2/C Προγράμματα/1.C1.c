/*************************************************************/
/* 						Program 1.C1.c  			         */
/* Synchronization of 5 system calls SC1, SC2, SC3, SC4, SC5 */
/* using 4 binary semaphores                                 */
/*************************************************************/

#include <stdio.h>             /* printf()                   */
#include <stdlib.h>            /* exit(), malloc(), free()   */
#include <sys/types.h>         /* key_t, sem_t, pid_t        */
#include <sys/shm.h>           /* shmat(), IPC_RMID          */
#include <errno.h>             /* errno, ECHILD              */
#include <semaphore.h>         /* sem_open(), sem_wait() ... */
#include <fcntl.h>             /* O_CREAT, O_EXEC            */
#include <sys/wait.h>          /* wait_pid()                 */


typedef sem_t semaphore;
/* binary semaphores */
semaphore *s14, *s24, *s35, *s45;


int main ()
{
    int   i;  /* loop variable */
    pid_t pid[5];  /* fork pids */
    int status;  /* child process status */  
	

    /* initialize semaphores for shared processes */          
    s14 = sem_open ("Sem14", O_CREAT | O_EXCL, 0644, 0); 
    s24 = sem_open ("Sem24", O_CREAT | O_EXCL, 0644, 0);   
	s35 = sem_open ("Sem35", O_CREAT | O_EXCL, 0644, 0); 
    s45 = sem_open ("Sem45", O_CREAT | O_EXCL, 0644, 0);  
	  

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
	    sem_post (s14);
    }
    else if (pid[1] == 0) /* SC2 */
    {
   		system("echo Have a nice day. >> output.txt"); 
	    sem_post (s24);
    }
    else if (pid[2] == 0)  /* SC3 */
    { 
      	system("ls -l");
	   	sem_post (s35);
    }
    else if (pid[3] == 0)  /* SC4 */
    {
		sem_wait (s14);
		sem_wait (s24);
		system("cat output.txt"); 
		sem_post (s45);
    }
   	else if (pid[4] == 0)  /* SC5 */
    {
		sem_wait(s35);
		sem_wait(s45);
		system("rm output.txt"); 
    }
    else  /* Parent process */
    {
		for (i = 0; i < 5; i++)
		{
	    	pid_t wpid = waitpid(pid[i], &status, 0);  /* wait for child processes to end */

	    	/* if (wpid == -1) 
	  	    {
          	    perror ("\nWaitpid error.\n");
          	    status = -1;	
          	    return status;
      	    } */ 
		// if (WIFEXITED(status))  /* check if all child processes end normally */
           // printf("\nChild %d terminated with exit status %d.\n", wpid, WEXITSTATUS(status));
        // else
           // printf("\nChild: %d terminated abnormally.\n", wpid);
        }

        /* unlink and close semaphores */
        sem_unlink ("Sem14");   
        sem_close (s14);  
        sem_unlink ("Sem24");   
        sem_close (s24);  
        sem_unlink ("Sem35");   
        sem_close (s35);  
        sem_unlink ("Sem45");   
        sem_close (s45);  
    
        printf("\n\n");
    }

    exit (0);
}
