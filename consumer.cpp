#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM "/shared_memory"
#define MAX_ENTRIES 1000

#define FULL "/sem_full"
#define EMPTY "/sem_empty"
#define MUTEX "/sem_mutex"

typedef struct {
    char name[11];
    double price;
} Entry;

typedef struct {
    int in;
    int out;
    Entry buffer[MAX_ENTRIES];
} SharedBuffer;

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Correct format: ./consumer N \n");
    exit(1);
  }

  int N = atoi(argv[1]);
  if (N <= 0 || N > MAX_ENTRIES)
  {
    fprintf(stderr, "N must be between 1 and %d \n", MAX_ENTRIES);
    exit(1);
  }

  size_t size = sizeof(SharedBuffer);

  int shm_fd = shm_open(SHM, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1)
  {
    perror("shm_open failed");
    exit(1);
  }

  if (ftruncate(shm_fd, size) == -1)
  {
    perror("ftruncate failed");
    exit(1);
  }

  SharedBuffer *shm = (SharedBuffer *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm == MAP_FAILED)
  {
    perror("mmap failed");
    exit(1);
  }

  close(shm_fd);

  sem_t *full = sem_open(FULL, O_CREAT, 0666, 0);
  sem_t *empty = sem_open(EMPTY, O_CREAT, 0666, N);
  sem_t *mutex = sem_open(MUTEX, O_CREAT, 0666, 1);

  if (full == SEM_FAILED || empty == SEM_FAILED || mutex == SEM_FAILED)
  {
    perror("sem_open failed");
    exit(1);
  }

  printf("Consumer ready, N=%d\n", N);

  sem_close(full);
  sem_close(empty);
  sem_close(mutex);
  munmap(shm, size);
  return 0;
}