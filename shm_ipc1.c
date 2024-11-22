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

	// Pas 1: Creem un obiect memorie cu ajutorul functiei "shm_open"
	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (shm_fd == -1)
	{
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	// Implicit, dimensiunea regiunii de memorie desemnata de obiectul de
	// memorie creat cu "shm_open" este zero. 
	// Setam dimensiunea acestei regiuni folosind functia "ftruncate"
	if (ftruncate(shm_fd, sizeof(struct shared_mem)) == -1)
	{
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}

	// Pas 2: Mapam obiectul memorie desemnat de descriptorul de fisier shm_fd
	// in spatiul de adrese al procesului, cu ajutorul functiei "mmap"
	struct shared_mem* shm_ptr = mmap(NULL, sizeof(*shm_ptr), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm_ptr == MAP_FAILED)
	{
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	// Initializam contorul si semaforul
	shm_ptr->counter = 0;
	if (sem_init(&shm_ptr->sem, 1, 1) == -1)
	{
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	// Cream un proces fiu
	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}

	// Instructiunile de program din acest punct in jos vor fi executate concurent
	// atat de procesul parinte, cat si de procesul fiu
	if (pid == 0)
		count(shm_ptr, pid);
	else
	{
		count(shm_ptr, pid);
		wait(NULL);
		printf("Final counter value: %d\n", shm_ptr->counter);
	}


	// Pas 3: Demapam obiectul memorie din spatiul de adrese al proceselor
	// cu ajutorul functiei "munmap"
	munmap(shm_ptr, sizeof(*shm_ptr));

	// Pas 4: Inchidem obiectul identificat prin descriptor de fisier
	// folosind apelul de sistem "close"
	close(shm_fd);
	
	// Pas 5: In final, eliminam obiectul memorie identificat prin nume
	// folosind functia "shm_unlink"
	shm_unlink(SHM_NAME);


	return 0;
}
