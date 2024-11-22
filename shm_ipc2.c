#include <ctype.h>
#include <fcntl.h> // Pentru constantele de mod de acces la fisiere O_RDONLY, O_RDWR etc.
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // Pentru functii de gestionare a memoriei precum "mmap"
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_NAME "/shm_counter"
#define COUNTER_LIMIT 1000

struct shared_mem
{
	int counter;
	sem_t sem;
};

void count(struct shared_mem* shm_ptr, pid_t pid)
{
	for (int i = 0; i < COUNTER_LIMIT; ++i)
        {
                sem_wait(&shm_ptr->sem);
                if (rand() % 2 == 0)
                {
                        printf((pid == 0) ? "Child" : "Parent");
                        printf(" process incremented counter by 1. (Current value: %d)\n", ++shm_ptr->counter);
                }
                sem_post(&shm_ptr->sem);
		usleep(10000); // sleep for 10ms
	}

}

int main()
{
	// Initializam seedul pentru functia rand()
	srand(time(NULL));

	struct shared_mem* shm_ptr = mmap(NULL, sizeof(*shm_ptr), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shm_ptr == MAP_FAILED)
	{
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	shm_ptr->counter = 0;
	if (sem_init(&shm_ptr->sem, 1, 1) == -1)
	{
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0)
		count(shm_ptr, pid);
	else
	{
		count(shm_ptr, pid);
		wait(NULL);
		printf("Final counter value: %d\n", shm_ptr->counter);
	}

	munmap(shm_ptr, sizeof(*shm_ptr));
	shm_unlink(SHM_NAME);

	return 0;
}
