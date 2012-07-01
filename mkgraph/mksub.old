#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "../def.h"

#define LEN_CMD 100

#define RAND_MAX 2147483647

int main(int argc, char **argv) {
  int n = atoi(argv[1]);
  FILE * fp;
  FILE * reqfile;
  char filename[LEN_FILENAME], reqfilename[LEN_FILENAME];
  char cmd[LEN_CMD];
  //int i, k = 0, countk = 0, p = 0, start;
  
  int num_nodes, num_edges, from, to;

    sprintf(filename, "itm-specsub");
    fp = fopen(filename, "w");
    fprintf(fp, "geo 1\n");
    fprintf(fp, "%d 100 2 0.5 0.2\n", n);
    //fprintf(fp, "ts 1\n");
    //fprintf(fp, "100 1 10\n");
    
    //3 - pure random
    fclose(fp);

  sleep(10);

    sprintf(cmd, "./itm itm-specsub");
    printf("%s\n", cmd);
    system(cmd);

    sprintf(cmd, "./sgb2alt itm-specsub-0.gb sub.alt");
    printf("%s\n", cmd);
    system(cmd);
   
  char str[1000];
  int j;
    sprintf(filename, "sub.alt");
    fp = fopen(filename, "r");
    sprintf(reqfilename, "../sub.txt");
    reqfile = fopen(reqfilename, "w");

    for (j = 0; j < 10; j ++)
      fscanf(fp, "%s", str);

    fscanf(fp, "%d %d %*d %*d", &num_nodes, &num_edges);
    num_edges /= 2;

    fprintf(reqfile, "%d %d\n", num_nodes, num_edges);

    for (j = 0; j < 19; j ++)
      fscanf(fp, "%s", str);

    for (j = 0; j < num_nodes; j ++) {
      fscanf(fp, "%*d %*d %*d %*d");
      fprintf(reqfile, "%lf\n", (rand()/(double)RAND_MAX +1)*(double)MAX_CPU*0.5);      
    }
    
    for (j = 0; j < 6; j ++) {
      fscanf(fp, "%s", str);
      //printf("%s\n", str);
    }       

    for (j = 0; j < num_edges; j ++) {
      fscanf(fp, "%d %d %*d %*d", &from, &to);
      fprintf(reqfile, "%d %d %lf\n", from, to, (rand()/(double)RAND_MAX +1) * (double)MAX_BW * 0.5); 
    }

    fclose(fp);
    fclose(reqfile);
  
  return 0;
}
