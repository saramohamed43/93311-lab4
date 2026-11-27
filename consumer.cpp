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

#define COMMODITIES_NUMBER 11
#define WINDOW 5

typedef struct {
    char name[11];
    double price;
} Entry;

typedef struct {
    int in;
    int out;
    Entry buffer[MAX_ENTRIES];
} SharedBuffer;

typedef struct {
    const char *name;
    double price;
    double readings[WINDOW];
    int count;
    int next;
    double avg;
    double prev_price;
    double prev_avg;
} Commodity;

Commodity table[COMMODITIES_NUMBER] = {
    {"ALUMINIUM"}, {"COPPER"}, {"COTTON"}, {"CRUDEOIL"}, {"GOLD"}, {"LEAD"},
    {"MENTHAOIL"}, {"NATURALGAS"}, {"NICKEL"}, {"SILVER"}, {"ZINC"}
};

int find_commodity(const char *name) //find commodity by name, -1 if unknown
{
    for (int i = 0; i < COMMODITIES_NUMBER; i++)
    {
        if (strcmp(table[i].name, name) == 0)
            return i;
    }
    return -1;
}

void update_commodity(int i, double price)
{
    Commodity *c = &table[i];

    c->prev_avg = c->avg;
    c->prev_price = c->price;
    c->price = price;

    c->readings[c->next] = price;
    c->next = (c->next + 1) % WINDOW;
    if (c->count < WINDOW)
        c->count++;

    double sum = 0;
    for (int k = 0; k < c->count; k++)
        sum += c->readings[k];
    c->avg = sum / c->count;
}

void print_cell(double value, double prev)
{
    const char *color = "\033[36m";   // cyan: no data yet or unchanged
    const char *arrow = " ";

    if (value > 0 && value > prev)
    {
        color = "\033[32m";           // green up
        arrow = "\u2191";
    }
    else if (value > 0 && value < prev)
    {
        color = "\033[31m";           // red down
        arrow = "\u2193";
    }

    printf("%s%7.2lf%s\033[0m", color, value, arrow);
}

void dashboard()
{
    printf("\033[1;1H\033[2J");   // move cursor to top-left, clear the screen

    printf("+--------------+----------+----------+\n");
    printf("| %-12s | %-8s | %-8s |\n", "Currency", "Price", "AvgPrice");
    printf("+--------------+----------+----------+\n");

    for (int i = 0; i < COMMODITIES_NUMBER; i++)
    {
        Commodity *c = &table[i];

        printf("| %-12s | ", c->name);
        print_cell(c->price, c->prev_price);
        printf(" | ");
        print_cell(c->avg, c->prev_avg);
        printf(" |\n");
    }

    printf("+--------------+----------+----------+\n");
    fflush(stdout);
}

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

dashboard();

  while (1)
  {
    sem_wait(full);
    sem_wait(mutex);

    Entry e = shm->buffer[shm->out];
    shm->out = (shm->out + 1) % N;

    sem_post(mutex);
    sem_post(empty);

    int i = find_commodity(e.name);
    if (i != -1){
        update_commodity(i, e.price);
        dashboard();
    }
  }
}