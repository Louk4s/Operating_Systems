/**********************************************************/
/* 					   Program 1.D1.1.c  			      */
/* Parking application using binary semaphores            */
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


#define NUM_OF_CARS 5  /* total number of cars entering and leaving parking area */
#define N 3 /* number of parking places */


void initialize_shared();
void detach_shared();
void car ();
int enter_p ();
void leave_p ();
int select_place ();


typedef sem_t semaphore;
/* binary semaphores */
semaphore *free_aSem, *free_sSem;  

int *free_a, *free_s;  /* shared vars */	

int shmid1, shmid2; /* shared memory ids */



int main ()
{
	pid_t w; 
	int status;

	pid_t pid[NUM_OF_CARS];  /* fork pids */			  
    int i;	/* counter var */
						       
    /* initialize shared variables in shared memory */
	initialize_shared();
    
    // use current time as seed for random generator */
	srand(time(0)); 
	
	printf("\n\n");

	for (i = 0; i < NUM_OF_CARS; i++)
    {
    	pid[i] = fork();  /* creation of child processes */
    	
		if (pid[i] < 0)
    	{
       		printf ("\nFork error.\n");
       		return (-1);  
    	}
    	else if (pid[i] == 0)  /* child process */
			 {
				car (i);			
				exit (0);
		     }		
	}
     
    /* parent process */
    for (i = 0; i < NUM_OF_CARS; i++) 
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

	detach_shared();
	
	printf("\n\n");
	exit (0);
}


void initialize_shared() 
{	
	int i;
	
	shmid1 = shmget (IPC_PRIVATE, sizeof (int) * N, 0644 | IPC_CREAT);  
    shmid2 = shmget (IPC_PRIVATE, sizeof (int), 0644 | IPC_CREAT);
    
    /* shared memory error check */
    if (shmid1 < 0 || shmid2 < 0) 
	{
        perror ("\nShmget error.\n");
        exit (1);
    }
    							       
    /* attach free_a, free_s to shared memory */
    free_a = (int *) shmat (shmid1, NULL, 0);  
	free_s = (int *) shmat (shmid2, NULL, 0);

	/* initialize semaphores for shared processes */
    free_aSem = sem_open ("free_aSem", O_CREAT | O_EXCL, 0644, 1); 
    free_sSem = sem_open ("free_sSem", O_CREAT | O_EXCL, 0644, 1); 
    
    /* initialize shared vars */
    for (i = 0; i < N; i++)
	    *(free_a + i) = 1;  
	*free_s = N;  
}
      
    	
void detach_shared()	
{
    /* shared memory detach */
    shmdt (free_a);
    shmctl (shmid1, IPC_RMID, 0); 
    shmdt (free_s);
    shmctl (shmid2, IPC_RMID, 0); 
    
    /* unlink and close semaphores */
    sem_unlink ("free_aSem");   
    sem_close (free_aSem);  
    sem_unlink ("free_sSem");   
    sem_close (free_sSem); 
} 


/* car process */
void car (int k)
{
   
    int i, ticket;
   
    for (i = 0; i < 3; i++)
    {
	    usleep(rand()%100);  /* driver does something while his car is outside of the parking area */
		ticket = enter_p (k, i);
	    if (ticket != -1)  /* valid parking place */
		{
			usleep (rand()%100);  /* driver does something while his car is in the parking area */
	    	leave_p (k, ticket, i);
    	}
	}	    
}


/* enter to parking area */
int enter_p (int k, int visit)
{
    
    /*****************************/
	/* conditional region free_s */
	/*****************************/
	while (1)
    {
		/* acquire a lock to shared var free_s */
		sem_wait (free_sSem); 
		if ((*free_s) <= 0) 
		    /* release lock to free_s */
			sem_post (free_sSem);
		else
		    break;
    } 
    
	(*free_s)--;
 
    /* release lock to free_s */
    sem_post (free_sSem);
   
   
   
    /*****************/
	/* region free_a */
	/*****************/
    int free_p;
	
	/* acquire a lock to shared var free_a */
	sem_wait (free_aSem);
	
	free_p = select_place (free_a);
	if (free_p != -1) 
	{
		printf ("Car %d enters parking area with ticket %d after visit %d\n", k, free_p, visit);
		*(free_a + free_p) = 0;
	}
	else
		printf ("No valid ticket for car %d in visit %d\n", k, visit);
	
	/* release lock to free_a */
	sem_post(free_aSem);
	
	return free_p;
}
		
		
/* leave parking area */
void leave_p (int k, int free_p, int visit)
{
	
	/*****************/
	/* region free_a */
	/*****************/
	/* acquire a lock to shared var free_a */
	sem_wait (free_aSem);
		
	*(free_a + free_p) = 1;
	printf("Car %d with ticket %d leaves parking area after visit %d\n", k, free_p, visit);
	
	/* release lock to free_a */
	sem_post (free_aSem);
	
	
	
	/*****************/
	/* region free_s */
	/*****************/
	/* acquire a lock to shared var free_s */
    sem_wait (free_sSem);
	
	(*free_s)++;
	
	/* release lock to free_s */
	sem_post (free_sSem);
}


int select_place (int *free_a)
{
	int i, place;
	
	place = -1;  /* no valid parking place */
	
	i = 0;
	while (i < N)
	{
		if (*(free_a + i) == 1)
		{
		    place = i;
		    break;
	    }
	    else
	        i++;
	}
	
	return (place);
}
