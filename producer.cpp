#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <random>
#include <time.h>


#define SHM "/shared_memory"
#define MAX_ENTRIES 1000

#define FULL "/sem_full"
#define EMPTY "/sem_empty"
#define MUTEX "/sem_mutex"

typedef struct {
    char name[11]; // 10 chars + '\0'
    double price;
} Entry;

typedef struct {
    int in;
    int out;
    Entry buffer[MAX_ENTRIES];
} SharedBuffer;

void log_message(const char *name,const char *msg)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm *t = localtime(&ts.tv_sec);

    char date[32];
    strftime(date, sizeof(date), "%m/%d/%Y %H:%M:%S", t);

    fprintf(stderr, "[%s.%03ld] %s: %s\n", date, ts.tv_nsec / 1000000, name, msg);
}

int main(int argc, char **argv)
{
  if(argc != 6)
  {
    fprintf(stderr, "Correct format: ./producer name mean stddev T N \n");
    exit(1);
  }

  char *name = argv[1];
  double mean = atof(argv[2]); // convert string to double
  double std_dev = atof(argv[3]);
  int T = atoi(argv[4]); // convert string to int
  int N = atoi(argv[5]); 

  if (strlen(name) > 10)
  {
      fprintf(stderr, "name must be at most 10 characters \n");
      exit(1);
  }

  if (std_dev <= 0)
  {
      fprintf(stderr, "standard deviation must be greater than 0 \n");
      exit(1);
  }

  if (T < 0)
  {
      fprintf(stderr, "T must be 0 or more \n");
      exit(1);
  }

  if (N <= 0 || N > MAX_ENTRIES)
  {
      fprintf(stderr, "N must be between 1 and %d \n", MAX_ENTRIES);
      exit(1);
  }

  std::random_device rd;                               
  std::mt19937 gen(rd());                              
  std::normal_distribution<double> dist(mean, std_dev);

  int shm_fd = shm_open(SHM, O_CREAT | O_RDWR, 0666);

  if (shm_fd == -1) 
  {
    perror("shm_open failed");
    exit(1);
  }

  if (ftruncate(shm_fd, sizeof(SharedBuffer)) == -1) 
  {  
    perror("ftruncate failed");
    exit(1);
  }

  SharedBuffer *shm = (SharedBuffer *) mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
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

  char buffer[100];
  while (1)
  {
    double price = dist(gen); 
    snprintf(buffer, sizeof(buffer), "generating a new value %.2f", price);
    log_message(name, buffer);

    log_message(name, "trying to get mutex on shared buffer");
    sem_wait(empty);
    sem_wait(mutex);

    strncpy(shm->buffer[shm->in].name, name, 10);
    shm->buffer[shm->in].name[10] = '\0';
    shm->buffer[shm->in].price = price;
    shm->in = (shm->in + 1) % N;

    snprintf(buffer, sizeof(buffer), "placing %.2f on shared buffer", price);
    log_message(name, buffer);

    sem_post(mutex);
    sem_post(full);

    snprintf(buffer, sizeof(buffer), "sleeping for %d ms", T);
    log_message(name, buffer);
    usleep(T * 1000); // sleep for T milliseconds
  }
}