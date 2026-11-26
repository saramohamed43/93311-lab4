# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

int main (int argc, char **argv)
{
  if(argc != 6)
  {
    fprintf(stderr, "Correct format: ./producer name mean stddev T N \n");
    exit(1);
  }

  if(strlen(argv[1]) > 10)
  {
    fprintf(stderr, "Producer name must be less than 10 characters\n");
    exit(1);
  }

  if(atoi(argv[4]) < 0 || atoi(argv[5]) < 0)
  {
    fprintf(stderr, "T and N must be non-negative integers\n");
    exit(1);
  }
  
  char *name = argv[1];
  double mean = atof(argv[2]); // convert string to double
  double std_dev = atof(argv[3]);
  int T = atoi(argv[4]); // convert string to int
  int N = atoi(argv[5]); 

  printf("Producer name: %s\n", name);
  printf("Mean: %f\n", mean);
  printf("Standard Deviation: %f\n", std_dev);
  printf("T: %d\n", T);
  printf("N: %d\n", N);

  return 0;
}
