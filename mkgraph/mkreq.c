#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "../def.h"

#define LEN_CMD 100

#define POISSON_MEAN 5 //per 100 time
//#define POISSON_MEAN 50 //per 1000 time
#define TOTAL_TIME 10000 //for the 100 requests

#define RAND_MAX 2147483647

//#define STAR_NODES_MOST 4

//#define LONG_REQ_RATE    0.2
//#define SHORT_REQ_RATE   0.8

//#define LONG_REQ_DURATION   1800
//#define SHORT_REQ_DURATION  200

#define NUM_NODE 10

//#define SPLITTABLE_REQ_RATE 0.8
//#define UNSPLITTABLE_REQ_RATE 0.2

//#define TOPO_GENERAL_RATE 0.4
//#define TOPO_STAR_RATE 0.6

#define MIGRATION_RATE 0.9
#define NO_MIGRATION_RATE 0.1

int poisson(double lambda) {
  double p;
  int r;
  p=0;
  r=0;
  while (1) {
    p=p-log(rand()/(double)RAND_MAX);
    if (p<lambda) {
      r++;
    } else {
      break;
    }
  }
  return r;
}

int main(int argc, char **argv) {

  int xseed = (unsigned)time(NULL);  // Current Time
  srand( xseed ); 

  //Usage:
  //mkreq #req [splittable rate] [link rate]
  int n = atoi(argv[1]);
  double splittable_rate = atoi(argv[2])/(double)10;
  double link_rate = atoi(argv[3])/(double)10;
  double topo_general_rate = atoi(argv[4])/(double)10;
  //dir argv[5]
  //double node_rate = atoi(argv[5])/(double)10;
  printf("split %lf, link%lf topo_gen %lf\n", splittable_rate, link_rate, topo_general_rate);
  FILE * fp;
  FILE * reqfile;
  char filename[LEN_FILENAME], reqfilename[LEN_FILENAME];
  char cmd[LEN_CMD];
  int i, k = 0, countk = 0, p = 0, start;
  
  int num_nodes, num_edges, split, time, duration, from, to, topo;

  for (i = 0; i < n; i ++) {
    sprintf(filename, "spec/itm-spec%d", i);
    fp = fopen(filename, "w");
    fprintf(fp, "geo 1\n");
    int ran = rand();
    int t = (ran % (NUM_NODE - 2)) + 2;
    //printf("req%d rand %d, #nodes: %d\n", i, ran, t);
    fprintf(fp, "%d 100 2 0.5 0.2\n", t);
    //3 - pure random
    fclose(fp);
  }

  sleep(10);

  for (i = 0; i < n; i ++) {
    sprintf(cmd, "./itm spec/itm-spec%d",i);
    printf("%s\n", cmd);
    system(cmd);
  }

  for (i = 0; i < n; i ++) {
    sprintf(cmd, "./sgb2alt spec/itm-spec%d-0.gb alt/%d.alt", i, i);
    printf("%s\n", cmd);
    system(cmd);
  }
   
  char str[1000];
  int j;
  for (i = 0; i < n; i ++) {
    printf("generate req %d\n", i);
    sprintf(filename, "alt/%d.alt", i);
    fp = fopen(filename, "r");
    sprintf(reqfilename, "../%s/req%d.txt", argv[5], i);
    reqfile = fopen(reqfilename, "w");

    for (j = 0; j < 10; j ++)
      fscanf(fp, "%s", str);

    fscanf(fp, "%d %d %*d %*d", &num_nodes, &num_edges);
    num_edges /= 2;

    if (rand()/(double)RAND_MAX < splittable_rate) {
      split = LINK_SPLITTABLE;
    } else {
      split = LINK_UNSPLITTABLE;
    }

    if (countk == k) {
      k = 0;
      while( k==0)
        k = poisson(POISSON_MEAN);
      countk = 0;
      printf("k %d\n", k);
      start = (p * TOTAL_TIME*POISSON_MEAN)/n;
      p ++; 
    }

    time = start + (countk+1)*TOTAL_TIME*POISSON_MEAN/(n*(k+1));
    countk ++;

    /*if (rand()/(double)RAND_MAX < LONG_REQ_RATE) {
      duration = LONG_REQ_DURATION;
    } else {
      duration = SHORT_REQ_DURATION;
      }*/
    duration = (int)(-log(rand()/(double)RAND_MAX)*1000);

    if (rand()/(double)RAND_MAX < topo_general_rate) {
      topo = TOPO_GENERAL;
    } else {
      topo = TOPO_STAR;
      //num_nodes = (int)((rand()/(double)RAND_MAX) * STAR_NODES_MOST) + 1;
      num_edges = num_nodes - 1;
    }

    fprintf(reqfile, "%d %d %d %d %d %d\n", num_nodes, num_edges, split, time, duration, topo);
    printf("time %d, duration %d\n", time, duration);

    for (j = 0; j < 19; j ++)
      fscanf(fp, "%s", str);

    for (j = 0; j < num_nodes; j ++) {
      fscanf(fp, "%*d %*d %*d %*d");
      double t = rand();      
      fprintf(reqfile, "%lf\n", t/(double)RAND_MAX * (double)MAX_CPU*0.001);      
    }
    
    for (j = 0; j < 6; j ++) {
      fscanf(fp, "%s", str);
      //printf("%s\n", str);
    }       

    for (j = 0; j < num_edges; j ++) {
      fscanf(fp, "%d %d %*d %*d", &from, &to);
      if (topo == TOPO_GENERAL) 
        fprintf(reqfile, "%d %d %lf\n", from, to, rand()/(double)RAND_MAX * (double)MAX_BW * link_rate); 
    }

    if (topo == TOPO_STAR) {
      for (j = 0; j < num_nodes - 1; j ++) {
        fprintf(reqfile, "%d %d %lf\n", j, num_nodes-1, rand()/(double)RAND_MAX * (double)MAX_BW * link_rate); 
      }
    }

    fclose(fp);
    fclose(reqfile);
  }
  
  return 0;
}
