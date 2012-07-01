#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "def.h"
#include "embed.h"

struct request * req;
int n;
struct substrate_network sub;

struct s2v_node * s2v_n;
struct s2v_link * s2v_l;

struct req2sub * v2s;

struct shortest_path * spath;

#define TIME_INTERVAL 100
#define MAX_DOUBLE 1000000
#define MAX_LEN 1000

#define W_NODE_COST 1
#define W_LINK_COST 1

#define RAND_MAX 2147483647

void remove_node_map(struct s2v_node * s2v_n, int snode, int index);
void add_node_map(struct s2v_node * s2v_n, struct req2sub * v2s, int snode, int reqid, int nodeid);
void remove_link_map(struct s2v_link * s2v_l, int slink, int index);
void add_link_map(struct s2v_link * s2v_l, int slink, int reqid, int vlink);
void release_resource(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int index);
void calc_shortest_path();
void init_s2v_l(struct s2v_link * s2v_l);
void init_v2s(struct req2sub * v2s);
void print_map(struct req2sub * v2s);
void print_s2v_l(struct s2v_link * s2v_l);

struct s2v_node * s2v_ntmp;
struct s2v_link * s2v_ltmp;
struct req2sub * v2stmp;

struct s2v_node * s2v_ntmp2;
struct s2v_link * s2v_ltmp2;
struct req2sub * v2stmp2;

int find_proper_node(struct s2v_node * s2v_n, struct s2v_link * s2v_l, double cpu, int index) {
  int i, j, snode = -1;
  double min = MAX_CPU;
  for(i = 0; i < sub.nodes; i ++) {
    int exist = 0;
    for (j = 0; j < s2v_n[i].req_count; j ++) {
      if (s2v_n[i].req[j] == index) {
        exist = 1;
        break;
      }
    }
    if (exist == 1) continue;
    double diff = s2v_n[i].rest_cpu - cpu;
    if ( diff > 0 && diff < min) {
      min = diff;
      snode = i;
    }
  }

  return snode;
}

int find_MinNeighborResource_node(struct s2v_node * s2v_n, struct s2v_link * s2v_l, double cpu, int index) {
  int i, j, snode = -1;
  double min = -1;
  double diff, sum;
  for(i = 0; i < sub.nodes; i ++) {
    int exist = 0;
    for (j = 0; j < s2v_n[i].req_count; j ++) {
      if (s2v_n[i].req[j] == index) {
        exist = 1;
        break;
      }
    }
    if (exist == 1) continue;
    diff = s2v_n[i].rest_cpu - cpu;
    sum = 0;
    if (diff < 0) continue;
    for (j = 0; j < sub.links; j ++)
      if (sub.link[j].from == i || sub.link[j].to == i) {
        if (s2v_l[j].rest_bw > 0)
          sum += s2v_l[j].rest_bw;
      }
    diff *=sum;
    if (diff < min || min == -1) {
      min = diff;
      snode = i;
    }
  }

  return snode;
}

int find_MaxNeighborResource_node(struct s2v_node * s2v_n, struct s2v_link * s2v_l, double cpu, int index, int exclude) {
  int i, j, snode = -1;
  double max = -1;
  double diff, sum;
  for(i = 0; i < sub.nodes; i ++) {
    if (i == exclude) continue;
    int exist = 0;
    for (j = 0; j < s2v_n[i].req_count; j ++) {
      if (s2v_n[i].req[j] == index) {
        exist = 1;
        break;
      }
    }
    if (exist == 1) continue;
    diff = s2v_n[i].rest_cpu - cpu;
    sum = 0;
    if (diff <= 0) continue;
    for (j = 0; j < sub.links; j ++)
      if (sub.link[j].from == i || sub.link[j].to == i) {
        if (s2v_l[j].rest_bw > 0)
          sum += s2v_l[j].rest_bw;
      }
    diff *=sum;
    if (diff > max) {
      max = diff;
      snode = i;
    }
  }

  return snode;
}

int find_available_node(struct s2v_node * s2v_n, struct s2v_link * s2v_l, double cpu, int index) {
  int i, j, snode = -1;
  //double min = -1;
  double diff, sum;
  int r = (int)(rand()/(double)RAND_MAX * sub.nodes);
  int s;
  for(i = 0; i < sub.nodes; i ++) {
    s = (r+i)%sub.nodes;
    int exist = 0;
    for (j = 0; j < s2v_n[s].req_count; j ++) {
      if (s2v_n[s].req[j] == index) {
        exist = 1;
        break;
      }
    }
    if (exist == 1) continue;
    diff = s2v_n[s].rest_cpu - cpu;
    sum = 0;
    if (diff < 0) continue;
    return s;
  }

  return snode;
}

int map_node_greedy(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int index) {
  int i, j;
  int t;
  for (i = 0; i < req[index].nodes; i ++) {
    //t = find_proper_node(req[index].cpu[i], index);
    t = find_MaxNeighborResource_node(s2v_n, s2v_l, req[index].cpu[i], index, -1);
    if (t == -1) {
      v2s[index].map = STATE_MAP_NODE_FAIL;
      printf("req %d unsatisfied\n", index);
      for (j = 0; j < i; j ++) {
        remove_node_map(s2v_n, v2s[index].snode[j], j);
      }
      return -1;
    }
    //printf("map node req %d vnode %d snode %d\n", index, i, t);
    add_node_map(s2v_n, v2s, t, index, i);
  }
  
  v2s[index].map = STATE_MAP_NODE;
  
  return 0;
}

int map_node_star(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int index) {
  int i, j;
  int t;
  for (i = 0; i < req[index].nodes; i ++) {
    //t = find_proper_node(req[index].cpu[i], index);
    if (i == 0) {
      t = find_MaxNeighborResource_node(s2v_n, s2v_l, req[index].cpu[i], index, -1);
    } else {
      printf("find available node\n");
      t = find_available_node(s2v_n, s2v_l, req[index].cpu[i], index);
      //t = find_MinNeighborResource_node(req[index].cpu[i], index);
    }
    if (t == -1) {
      v2s[index].map = STATE_MAP_NODE_FAIL;
      printf("req %d unsatisfied\n", index);
      for (j = 0; j < i; j ++) {
        remove_node_map(s2v_n, v2s[index].snode[j], j);
      }
      return -1;
    }
    //printf("map node req %d vnode %d snode %d\n", index, i, t);
    add_node_map(s2v_n, v2s, t, index, i);
  }
  
  v2s[index].map = STATE_MAP_NODE;
  
  return 0;
}

int unsplittable_flow(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int start, int end, int option, int * slink, int * vlink, int time) {
  int i, j, k, t, index = -1;
  int flag, from, to, next; 

  struct s2v_link * s2v_ltmp3;
  s2v_ltmp3 = (struct s2v_link *)malloc(sizeof(struct s2v_link)* sub.links);
  memset(s2v_ltmp3, 0, sizeof(struct s2v_link)* sub.links);  

  memcpy(s2v_ltmp3, s2v_l, sizeof(struct s2v_link)*sub.links);
  //printf("*** begin unsplittalbe flow\n");

  for (i = start; i <= end; i ++) {
    if (v2s[i].map == STATE_MAP_NODE || (v2s[i].map == STATE_MAP_LINK && option == ROUTE_MIGRATION)) {
      flag = 0;
      if (req[i].split == LINK_UNSPLITTABLE) {
        for (j = 0; j < req[i].links; j ++) {
          from = v2s[i].snode[req[i].link[j].from];
          to = v2s[i].snode[req[i].link[j].to];
          while (from != to) {            
            next = spath[from * sub.nodes +to].next;
            for (k = 0; k < sub.links; k ++) {
              if ((sub.link[k].from == from && sub.link[k].to == next) || (sub.link[k].from == next && sub.link[k].to == from)) 
                break;
            }
            if (k >= sub.links || s2v_ltmp3[k].rest_bw < req[i].link[j].bw) {
              flag = 1;
              break;
            }
            s2v_ltmp3[k].rest_bw -= req[i].link[j].bw;
            from = next;
          }

          if (flag == 1) {
            *slink = k;
            *vlink = j;
            index = i;
            return index;
          }
        }
      }
    }
  }

  for (i = start; i <= end; i ++) {
    if (v2s[i].map == STATE_MAP_NODE || (v2s[i].map == STATE_MAP_LINK && option == ROUTE_MIGRATION)) {
      flag = 0;
      if (req[i].split == LINK_UNSPLITTABLE) {
        if (v2s[i].map == STATE_MAP_NODE) {
          v2s[i].maptime = time;
        }
        v2s[i].map = STATE_MAP_LINK;
        for (j = 0; j < req[i].links; j ++) {
          from = v2s[i].snode[req[i].link[j].from];
          to = v2s[i].snode[req[i].link[j].to];
          t = 0;
          while (from != to) {            
            next = spath[from * sub.nodes +to].next;
            for (k = 0; k < sub.links; k ++) {
              if ((sub.link[k].from == from && sub.link[k].to == next) || (sub.link[k].from == next && sub.link[k].to == from)) 
                break;
            }
            if (k >= sub.links || s2v_l[k].rest_bw < req[i].link[j].bw) {
              printf("i %d , j %d , k %d error!!!\n", i, j, k);
              flag = 1;
              break;
            }
            add_link_map(s2v_l, k, i, j);
            v2s[i].spath[j].link[t] = k;
            t ++;
            from = next;
          }

          v2s[i].spath[j].len = t;
          }
      }
    }
  }
  //printf("unsplittable success\n");
  return index;
}

int multicommodity_flow(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int start, int end, int option) {
  FILE * fp;
  fp = fopen("ltest.dat", "w");

  int i, j, k;
  int com_count = 0;
  int flag = 0;
  printf("req ");
  for (i = start; i <= end; i ++) {
    if (req[i].split == LINK_SPLITTABLE && (v2s[i].map == STATE_MAP_NODE || (v2s[i].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
      printf(" %d",i);
      com_count += req[i].links;
      flag = 1;
    }
  }
  printf("\n");
  if (flag == 0) 
    return -2;

  printf("com_count %d\n", com_count);

  int arcs = 2 * sub.links;// + com_count;
  fprintf(fp, "%d %d %d %d %d\n", sub.nodes, arcs, com_count, sub.links, sub.links * 2 * com_count);
  fprintf(fp, "ARC COSTS BY COMMODITIES\n");
  for (i = 0; i < com_count; i ++) {
    for (j = 0; j < 2*sub.links; j ++) {
      fprintf(fp, "1.0 ");
    }
    /*for (j = 0; j < com_count; j ++) {
      fprintf(fp, "-1+32 ");
      }*/
    fprintf(fp, "\n");
  }
  fprintf(fp, "ARC CAPACITIES BY COMMODITIES\n");
  for (j = start; j<=end; j ++) {
    if (req[j].split == LINK_SPLITTABLE && (v2s[j].map == STATE_MAP_NODE || (v2s[j].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
      for (k = 0; k < req[j].links; k ++) {
        for (i = 0; i < arcs; i ++) {
          if (s2v_l[i/2].rest_bw < 0) s2v_l[i/2].rest_bw = 0;
          fprintf(fp, "%.2f ", s2v_l[i/2].rest_bw);
        }
        fprintf(fp, "\n");          
      }
    }
  }
  fprintf(fp, "NODE INJECTIONS BY COMMODITIES\n");
  for (i = start; i <=end ; i ++) {
    if (req[i].split == LINK_SPLITTABLE && (v2s[i].map == STATE_MAP_NODE || (v2s[i].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
      for (j = 0; j < req[i].links; j ++) {
        for (k = 0; k < sub.nodes; k ++) {
          if (k == v2s[i].snode[req[i].link[j].from]) {
            fprintf(fp, "%.2f ", req[i].link[j].bw);
          } else {
            if (k == v2s[i].snode[req[i].link[j].to]) { 
              fprintf(fp, "%.2f ", - req[i].link[j].bw);
            } else {
              fprintf(fp, "0 ");
            }
          }
        }
        fprintf(fp, "\n");
      }  
    }
  }

  fprintf(fp, "ARC MUTUAL CAPACITIES\n");
  for (i = 0; i < arcs; i ++) {
    fprintf(fp, "1+32 ");
  }
  fprintf(fp, "\n");

  fprintf(fp, "NETWORK TOPOLOGY\n");
  for (i = 0; i < sub.links; i ++) {
    fprintf(fp, "%d %d\n", sub.link[i].from + 1, sub.link[i].to + 1);
    fprintf(fp, "%d %d\n", sub.link[i].to+1, sub.link[i].from+1);
  }
  /*for (i = start; i <= end; i ++) {
    if (v2s[i].map == 1) {
      for (j = 0; j < req[i].links; j ++) {
        fprintf(fp, "%d %d\n", v2s[i].snode[req[i].link[j].to]+1, v2s[i].snode[req[i].link[j].from]+1);
      }
    }
    }*/

  fprintf(fp, "LOWER AND UPPER BOUNDS\n");
  for (i = 0; i < sub.links; i ++)
    fprintf(fp, "0 ");
  fprintf(fp, "\n");
  for (i = 0; i < sub.links; i ++) {
    if (s2v_l[i].rest_bw < 0) s2v_l[i].rest_bw = 0;
    fprintf(fp, "%.2f ", s2v_l[i].rest_bw);
  }
  fprintf(fp, "\n");

  fprintf(fp, "SIDE CONSTRAINTS\n");
  for (i = 0; i < sub.links; i ++) {
    int p = 0;
    for (j = start; j <= end; j ++) {
      if (req[j].split == LINK_SPLITTABLE && (v2s[j].map == STATE_MAP_NODE || (v2s[j].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
        for (k = 0; k < req[j].links; k ++) {
          fprintf(fp, "%d %d %d 1\n", 2 * i + 1, p +1, i + 1);
          fprintf(fp, "%d %d %d 1\n", 2 * i + 2, p +1, i + 1);
          p +=1;
        }
      }
    }
  }

  fclose(fp);
  system("./lintest");
  sleep(1);
  return 0;
}

//check ltest output
//return -1 --success
//>0 suggest a node move/remove
int check_flow(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int start, int end, int option, int * slink, int * vlink, int time) {
  //printf("*** begin check_flow\n");

  FILE * fp;

  char str[MAX_LEN];
  struct s2v_link * s2v_ltmp;
  s2v_ltmp = (struct s2v_link *)malloc(sizeof(struct s2v_link)* sub.links*2);
  memset(s2v_ltmp, 0, sizeof(struct s2v_link)* sub.links*2);

  double flow, cap, diff;
  diff = 0;
  int index,i, j, k, p, rvalue = 0;
  
  *slink = -1;
  *vlink = -1;

  fp = fopen("ltest.lst", "r");

  while (fgets(str, MAX_LEN, fp) != NULL) {
    if (strstr(str, "STATUS") != NULL) {
      break;
    }
  }

  if (strstr(str, "INFEASIBLE") != NULL && strstr(str, "PHASE 0") != NULL) {
    return -2;
  }

  if (strstr(str, "INFEASIBLE") != NULL && strstr(str, "PHASE 1") != NULL) {
    rvalue = 0;
    while (fgets(str, MAX_LEN, fp) != NULL) {
      if (strstr(str, "VARIABLES") != NULL) {
        break;
      }
    }
    fgets(str, MAX_LEN, fp);
    fgets(str, MAX_LEN, fp);
    for (i = 0; i < sub.links * 2; i ++) {
      for (j = start; j <= end; j ++) {
        if (req[j].split == LINK_SPLITTABLE && (v2s[j].map == STATE_MAP_NODE || (v2s[j].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
          for (k = 0; k < req[j].links; k ++) {          
            fgets(str, MAX_LEN, fp);
            if (strstr(str, "LOWER") == NULL && strstr(str, "TREE") == NULL && strstr(str, "CMPLT") == NULL
                && strstr(str, "UPPER") == NULL && strstr(str, "SUPER") == NULL) {
              k --;
              continue;
            }
            int q;
            sscanf(str, "%d %d %*s %lf %*f", &p, &q, &flow);
            if (flow > 0) {
              int t = s2v_ltmp[i].count;
              s2v_ltmp[i].req[t] = j;
              s2v_ltmp[i].vlink[t] = k;
              s2v_ltmp[i].bw[t] = flow;
              s2v_ltmp[i].count ++;
            }
          }
        }
      }
    }    

    while (fgets(str, MAX_LEN, fp) != NULL) {
      if (strstr(str, "SIDE CONSTRAINTS") != NULL) {
        break;
      }
    }
    fgets(str, MAX_LEN, fp);
    fgets(str, MAX_LEN, fp);
    for (i = 0; i < sub.links; i ++) {
      fgets(str, MAX_LEN, fp);
      if (strstr(str, "BASIC") == NULL && strstr(str, "UPPER") == NULL && strstr(str, "LOWER") == NULL 
          && strstr(str, "SUPER") == NULL && strstr(str, "INFSB") == NULL) {
        i --;
        continue;
      }
      sscanf(str, "%d %*s %*f %lf %lf", &p, &flow, &cap); 
      if (i != p - 1) {
        printf("error %d %d\n", i, p);
      }
      if (flow - cap > diff) {
        diff = flow - cap;
        index = i;
      }
    }
    //index +=1;
    printf("most overload index %d\n", index);

    int maxid = -1;
    double maxbw = 0;
    for (i = 0; i < s2v_ltmp[2 * index].count; i ++) {
      //printf("bw %d %.2f\n", i, s2v_ltmp[2 * index].bw[i]);
      if (s2v_ltmp[2 * index].bw[i] > maxbw) {
        maxid = i;
        maxbw = s2v_ltmp[2*index].bw[i];
      }
    }
    for (i = 0; i < s2v_ltmp[2 * index+1].count; i ++) {
      if (s2v_ltmp[2 * index+1].bw[i] > maxbw) {
        maxid = i;
        maxbw = s2v_ltmp[2*index+1].bw[i];
      }
    }

    printf("most overload %d\n", maxid); //commodity number

    *slink = index -1;
    printf("check_flow start %d end %d\n", start, end);

    //find the request no. and its virtual link no. from commodity no. maxid
    int tmp = 0;
    for (i = start; i <= end; i ++) {
      if (req[i].split == LINK_SPLITTABLE && (v2s[i].map == STATE_MAP_NODE || (v2s[i].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
        if (tmp + req[i].links > maxid) {
          tmp = maxid - tmp;
          break;
        } else {
          tmp += req[i].links;
        }
      }
    }

    rvalue = i;
    *vlink = tmp;
    
    printf("check_flow return req %d vlink %d slink %d\n", rvalue, *vlink, *slink);
    //print_map(v2s);

    //find the bad slink index-1, delete req i, vlink t
    /*maxid = i;
    v2s[maxid].map = -1;
    for (j = 0; j < req[j].nodes; j ++) {
        t = v2s[maxid].snode[j];
        for (i = 0; i < s2v_n[t].req_count; i ++) {
          if (s2v_n[t].req[i] == maxid) {
            break;
          }
        }
        s2v_n[t].rest_cpu += req[maxid].cpu[i];
        for (k = i; k < s2v_n[t].req_count; k ++) {
          s2v_n[t].req[k] = s2v_n[t].req[k+1];
          s2v_n[t].vnode[k] = s2v_n[t].vnode[k+1];
          s2v_n[t].cpu[k] = s2v_n[t].cpu[k+1];
        }
        s2v_n[t].req_count --;
        }*/
  } else {
    if (strstr(str, "OPTIMAL") != NULL) {
      //printf("check_flow optimal solution\n");
      rvalue = -1;
      while (fgets(str, MAX_LEN, fp) != NULL) {
        if (strstr(str, "VARIABLES") != NULL) {
          break;
        }
      }
      fgets(str, MAX_LEN, fp);
      fgets(str, MAX_LEN, fp);
      p = 0;
      for (i = 0; i < sub.links * 2; i ++) {
        for (j = start; j <= end; j ++) {
          if (req[j].split == LINK_SPLITTABLE && (v2s[j].map == STATE_MAP_NODE || (v2s[j].map == STATE_MAP_LINK && option == ROUTE_MIGRATION))) {
            for (k = 0; k < req[j].links; k ++) {          
              fgets(str, MAX_LEN, fp);
              if (strstr(str, "LOWER") == NULL && strstr(str, "TREE") == NULL && strstr(str, "CMPLT") == NULL
                  && strstr(str, "UPPER") == NULL && strstr(str, "SUPER") == NULL) {
                k --;
                continue;
              }
              int q;
              sscanf(str, "%d %d %*s %lf %*f", &p, &q, &flow);
              //printf("flow %d %d %lf\n", p, q, flow);
              if (flow > 0) {
                int t = s2v_l[i/2].count;
                s2v_l[i/2].req[t] = j;
                s2v_l[i/2].vlink[t] = k;
                s2v_l[i/2].bw[t] = flow;
                s2v_l[i/2].count ++;
                if (flow > s2v_l[i/2].rest_bw + 0.001) {
                  //printf("slink %d flow %lf req.vlink %d.%d rest_bw %lf\n", i/2, flow, j, k, s2v_l[i/2].rest_bw);
                  s2v_l[i/2].rest_bw = 0;
                } else {
                  s2v_l[i/2].rest_bw -= flow;
                }
                int m = v2s[j].spath[k].len;
                v2s[j].spath[k].link[m] = i/2;
                v2s[j].spath[k].len ++;                
                //printf("v2s[%d].spath[%d].len %d\n", j, k, v2s[j].spath[k].len);
              }
            }
          }
        }
      }

      for (j = start; j <= end; j ++) {
        if (req[j].split == LINK_SPLITTABLE && v2s[j].map == STATE_MAP_NODE) {
          v2s[j].map = STATE_MAP_LINK;
          v2s[j].maptime = time;
        }
      }
              
    }
  }

  printf("check_flow ends\n");
  fclose(fp);
  return rvalue;
}

//load balance cost
/*
double calculate_cost(int start, int end) {
  //migration cost
  //operation cost
  double opcost = 0;
  int i, j, count = 0;
  double bwsum = 0, reqcost = 0;
  for (i = start; i <=end ; i ++) {
    bwsum = 0;
    reqcost = 0;
    if (v2s[i].map == 1) {
      for (j = 0; j < req[i].links; j ++) {
        bwsum += req[i].link[j].bw;
        reqcost += req[i].link[j].bw * v2s[i].spath[j].len;
      }
      opcost +=reqcost/bwsum;
    }
    count ++;
  }
  return opcost/(double)count;
}
*/

double calculate_cost(struct req2sub * v2s, int start, int end) {
  //migration cost
  //operation cost
  double opcost = 0;
  int i, j, count = 0;
  double bwsum = 0, reqcost = 0;
  for (i = start; i <=end ; i ++) {
    bwsum = 0;
    reqcost = 0;
    if (v2s[i].map == STATE_MAP_LINK) {
      for (j = 0; j < req[i].links; j ++) {
        bwsum += req[i].link[j].bw;
        reqcost += req[i].link[j].bw * v2s[i].spath[j].len;
      }
      opcost +=reqcost;
      count ++;
    }
  }
  if (count == 0) return 0;
  return opcost/(double)count;
}

int allocate(int start, int end, int time) {
  memset(s2v_ntmp, 0, sizeof(struct s2v_node)* sub.nodes);
  memset(s2v_ltmp, 0, sizeof(struct s2v_link)* sub.links);
  memset(v2stmp, 0, sizeof(struct req2sub) * n);

  memset(s2v_ntmp2, 0, sizeof(struct s2v_node)* sub.nodes);
  memset(s2v_ltmp2, 0, sizeof(struct s2v_link)* sub.links);
  memset(v2stmp2, 0, sizeof(struct req2sub) * n);

  //print_s2v_l(s2v_l);
  //print_map(v2s);

  /*double T, Te, a, b, c;
  a = 2;
  b = 0.97;
  c = 1.5;
  T = 100;*/
  //should choose different algo here.
  int i, trycount;

  for (i = 0; i < sub.nodes; i ++) {
    s2v_ntmp[i].req_count = 0;
    s2v_ntmp2[i].req_count = 0;
  }

  printf("allocate %d %d\n", start, end);
  for (i = start; i < end+1; i ++) {
    //printf("map node %d\n", i);
    if (v2s[i].map == STATE_NEW || v2s[i].map == STATE_MAP_NODE_FAIL) {
      switch (req[i].topo) {
      case TOPO_GENERAL:
        map_node_greedy(s2v_n, s2v_l, v2s, i);
        break;
      case TOPO_STAR:
        map_node_star(s2v_n, s2v_l, v2s, i);
        break;
      }
    }
  }
  
  double cost = -1, newcost = -1;

  double load, min_rest_load;
  int minid, reqid;
  int j, index, flag =1;
  int t = -1;
  int bottleneck_req, slink, vlink;
  int vnode, snode;
  int original;
      
  //simulated annealing
  //    while (T > 0.1) {
  // Te = T/(double)a;
  //  while ( T - Te > 0.1) {
      //TODO:checkpoint

  trycount = 0;

  bottleneck_req = unsplittable_flow(s2v_n, s2v_l, v2s, start, end, NO_MIGRATION, &slink, &vlink, time);
  if (bottleneck_req == -1) {
    int m = multicommodity_flow(s2v_n, s2v_l, v2s, start, end, NO_MIGRATION);
    if (m == -1) 
      return -1;
    if (m != -2)
      bottleneck_req = check_flow(s2v_n, s2v_l, v2s, start, end, NO_MIGRATION, &slink, &vlink, time);
    else bottleneck_req = -1;
  }

  //printf("bottleneck_req %d\n", bottleneck_req);

  while (flag) {    
    flag = 0;
    //printf("map link %d %d\n", start, end);

    memcpy(s2v_ntmp, s2v_n, sizeof(struct s2v_node)*sub.nodes);
    memcpy(s2v_ltmp, s2v_l, sizeof(struct s2v_link)*sub.links);
    memcpy(v2stmp, v2s, sizeof(struct req2sub)*n);

    if (bottleneck_req == -1) {  //success in get flow
      if (newcost == -1) {
        cost = calculate_cost(v2stmp, start, end);
        printf("cost %.2f\n", cost);
      } else {
        cost = newcost;
        printf("recalculating cost, old cost %.2f\n", cost);
      }
  
      min_rest_load = -1;
      minid = -1; //sub node no.
      for (i = 0; i < sub.nodes; i ++) {
        load = 0;
        for (j = 0; j < sub.links; j ++) {
          if (sub.link[j].from == i || sub.link[j].to ==i) {
            load += s2v_ltmp[j].rest_bw;
          }
        }
        load *= s2v_ntmp[i].rest_cpu;
        if (load < min_rest_load || min_rest_load == -1) {
          min_rest_load = load;
          minid = i;
        }
      }    

      reqid = (int)(rand()/(double)RAND_MAX * s2v_ntmp[minid].req_count); //find one req on this sub node
      index = s2v_ntmp[minid].req[reqid];//get the request number
      i = s2v_ntmp[minid].vnode[reqid];
      //now we get "index" req vnode "i" on sub node "minid"
      //remove old node
      snode = minid;
      vnode = i;
      printf("remove vnode %d %d on sub node %d\n", index, i, minid);
      remove_node_map(s2v_ntmp, minid, reqid);
      original = minid;
    } else {
      if (bottleneck_req == -2) {
        trycount = TIMES_TRY + 1;
        int m;
        for (m = start; m <=end; m ++) {
          if (v2s[m].map == STATE_MAP_NODE && req[m].split == LINK_SPLITTABLE) {
            bottleneck_req = m;
            break;
          } 
        }
      } 
        cost = -1;
        trycount ++;
        printf("trycount %d\n", trycount);
        if (trycount > TIMES_TRY) {
          release_resource(s2v_ntmp, s2v_ltmp, v2stmp, bottleneck_req);
          //printf("remove0\n");
          v2stmp[bottleneck_req].map = STATE_MAP_NODE_FAIL;
          /*for (i = 0; i < req[bottleneck_req].nodes; i ++) {
            int snode = v2stmp[bottleneck_req].snode[i];
            for (j = 0; j < s2v_ntmp[snode].req_count; j ++) {
            if (s2v_ntmp[snode].req[j] == bottleneck_req) {
            break;
            }
            }
            printf("remove1\n");
            remove_node_map(s2v_ntmp, snode, j);
            } */       
        } else {
          if (rand()/(double)RAND_MAX < 0.5) {
            vnode = req[bottleneck_req].link[vlink].from;
          } else {
            vnode = req[bottleneck_req].link[vlink].to;
        }     
          int snode = v2stmp[bottleneck_req].snode[vnode];
          for (i = 0; i < s2v_ntmp[snode].req_count; i ++) {
            if (s2v_ntmp[snode].req[i] == bottleneck_req) {
              break;
            }
          }
          //printf("remove2\n");
          remove_node_map(s2v_ntmp, snode, i);
          original = snode;
          reqid = i;
          index = bottleneck_req;
        }
      }
   

    //move node
    //find an optional node
    if (trycount <= TIMES_TRY) {
      t = find_MaxNeighborResource_node(s2v_ntmp, s2v_ltmp, s2v_ntmp[snode].cpu[reqid], index, original);
      if (t == -1) {
        printf("req %d unsatisfied\n", index);
        trycount = TIMES_TRY +1;
      } else {
        //add new mapping
        add_node_map(s2v_ntmp, v2stmp, t, index, vnode);
      }
    } else {
      trycount = 0;
    }

    bottleneck_req = unsplittable_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, NO_MIGRATION, &slink, &vlink, time);
    if (bottleneck_req == -1) {
      int m = multicommodity_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, NO_MIGRATION);
      if (m == -1) 
        return -1;
      if (m != -2)
        bottleneck_req = check_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, NO_MIGRATION, &slink, &vlink, time);
      else bottleneck_req = -1;
    }

    //printf("bottleneck_req %d\n", bottleneck_req);

    if (cost == -1) {
      memcpy(s2v_n, s2v_ntmp, sizeof(struct s2v_node)*sub.nodes);
      memcpy(s2v_l, s2v_ltmp, sizeof(struct s2v_link)*sub.links);
      memcpy(v2s, v2stmp, sizeof(struct req2sub)*n);
      flag = 1;
      continue;
    }

    newcost = calculate_cost(v2stmp, start, end);
    printf("new cost %.2f\n", newcost);
    
    if (newcost < cost || newcost == -1) {
      //TODO: copy new solution
      memcpy(s2v_n, s2v_ntmp, sizeof(struct s2v_node)*sub.nodes);
      memcpy(s2v_l, s2v_ltmp, sizeof(struct s2v_link)*sub.links);
      memcpy(v2s, v2stmp, sizeof(struct req2sub)*n);
      cost = newcost;
      flag = 1;
    } /*else {
        int t = rand()/(double)RAND_MAX;
        if (t < exp((cost - newcost)/T)) {
        //TODO:copy new solution
        memcpy(s2v_n, s2v_ntmp, sizeof(struct s2v_node)*sub.nodes);
          memcpy(s2v_l, s2v_ltmp, sizeof(struct s2v_link)*sub.links);
          memcpy(v2s, v2stmp, sizeof(struct req2sub)*n);
          }
          }*/
      // T = b* T;
      //  }
//  T = c*Te;
//}
  }

  //try route migration
#ifdef OPTION_ALLOW_MIGRATION
  cost = newcost; 
  newcost = -1;
    memcpy(s2v_ntmp, s2v_n, sizeof(struct s2v_node)*sub.nodes);
    memcpy(s2v_ltmp, s2v_l, sizeof(struct s2v_link)*sub.links);
    memcpy(v2stmp, v2s, sizeof(struct req2sub)*n);
    init_s2v_l(s2v_ltmp);
    init_v2s(v2stmp);

    //print_s2v_l(s2v_ltmp);

    bottleneck_req = unsplittable_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, ROUTE_MIGRATION, &slink, &vlink, time);
    if (bottleneck_req == -1) {
      int m = multicommodity_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, ROUTE_MIGRATION);
      if (m == -1) 
        return -1;
      if (m != -2) {
        bottleneck_req = check_flow(s2v_ntmp, s2v_ltmp, v2stmp, start, end, ROUTE_MIGRATION, &slink, &vlink, time);
      } else {
        bottleneck_req = -1;
      }
    }
    
    if (bottleneck_req == -1) {
      newcost = calculate_cost(v2stmp, start, end);
      printf("migrate cost %lf\n", newcost);
    } else {
      printf("route migration error\n");
      exit(0);
      //break;
    }

    if (newcost < cost && newcost != -1) {
      printf("migrate route\n");
      memcpy(s2v_n, s2v_ntmp, sizeof(struct s2v_node)*sub.nodes);
      memcpy(s2v_l, s2v_ltmp, sizeof(struct s2v_link)*sub.links);
      memcpy(v2s, v2stmp, sizeof(struct req2sub)*n);
    }
#endif
  
  return 0;
}

int main(int argc, char ** argv) {
  //init


  //fetch all requests
  n = atoi(argv[1]);
  FILE * fp;
  char filename[LEN_FILENAME];

  req = (struct request *)malloc(sizeof(struct request) * n);
  memset(req, 0, sizeof(struct request) *n);
  memset(&sub, 0, sizeof(struct substrate_network));

  v2s = (struct req2sub *)malloc(sizeof(struct req2sub)*n);
  memset(v2s, 0, sizeof(struct req2sub) * n);

  int i, j;

  for (i = 0; i < n; i ++) {
    sprintf(filename, "requests/req%d.txt", i);

    v2s[i].map = STATE_NEW;

    fp = fopen(filename, "r");
    fscanf(fp, "%d %d %d %d %d %d\n", &req[i].nodes, &req[i].links, &req[i].split, &req[i].time, &req[i].duration, &req[i].topo);
    
    for (j = 0; j < req[i].nodes; j ++) {
      fscanf(fp, "%lf\n", &req[i].cpu[j]);      
    }

    for (j = 0; j < req[i].links; j ++) {
      fscanf(fp, "%d %d %lf\n", &req[i].link[j].from, &req[i].link[j].to, &req[i].link[j].bw);
    }

    fclose(fp);
  }

  //fetch substrate network
  sprintf(filename, "sub.txt");
    fp = fopen(filename, "r");
    fscanf(fp, "%d %d\n", &sub.nodes, &sub.links);
    
    s2v_n = (struct s2v_node *)malloc(sizeof(struct s2v_node)*sub.nodes);
    memset(s2v_n, 0, sizeof(struct s2v_node)* sub.nodes);
    s2v_l = (struct s2v_link *)malloc(sizeof(struct s2v_link)*sub.links);
    memset(s2v_l, 0, sizeof(struct s2v_link)* sub.links);
    
    for (j = 0; j < sub.nodes; j ++) {
      fscanf(fp, "%lf\n", &sub.cpu[j]);      
      s2v_n[j].rest_cpu = sub.cpu[j];
      s2v_n[j].req_count = 0;
    }

    for (j = 0; j < sub.links; j ++) {
      fscanf(fp, "%d %d %lf\n", &sub.link[j].from, &sub.link[j].to, &sub.link[j].bw);
      s2v_l[j].rest_bw = sub.link[j].bw;
    }

    fclose(fp);

    s2v_ntmp = (struct s2v_node *)malloc(sizeof(struct s2v_node)*sub.nodes);
    s2v_ltmp = (struct s2v_link *)malloc(sizeof(struct s2v_link)*sub.links);
    v2stmp = (struct req2sub *)malloc(sizeof(struct req2sub)*n);

    s2v_ntmp2 = (struct s2v_node *)malloc(sizeof(struct s2v_node)*sub.nodes);
    s2v_ltmp2 = (struct s2v_link *)malloc(sizeof(struct s2v_link)*sub.links);
    v2stmp2 = (struct req2sub *)malloc(sizeof(struct req2sub)*n);

    spath = (struct shortest_path *)malloc(sizeof(struct shortest_path) * sub.nodes * sub.nodes);
    calc_shortest_path();

    int time = TIME_INTERVAL;
    int start, end, reqcount;
    start = 0; end = 0; 

    double opcost = 0;
    int count = 0;
    double bwsum = 0, reqcost = 0;
    reqcount = 0;

    while (end < n) {
      
      for (i = 0; i < end; i ++) {
        if (v2s[i].map == STATE_MAP_LINK && v2s[i].maptime + req[i].duration <= time) {
          printf("req %d done\n", i);
          v2s[i].map = STATE_DONE;

          bwsum = 0;
          reqcost = 0;
          for (j = 0; j < req[i].links; j ++) {
            bwsum += req[i].link[j].bw;
            if (v2s[i].spath[j].len <=0) printf("******error**********\n");
            reqcost += req[i].link[j].bw * v2s[i].spath[j].len;
          }
          opcost +=reqcost;
          count ++;

          release_resource(s2v_n, s2v_l, v2s, i);
        }
      }

      while (end < n && req[end].time < time) end ++;
      allocate(0, end - 1, time);
      
      /*reqcount = 0;
      for (i = 0; i < end; i ++) {
        if (v2s[i].map == STATE_MAP_LINK && v2s[i].maptime == time) reqcount++;
        }*/
      //fprintf(fp, "%d %d %d\n", time, end - start, reqcount);

      start = end;
      time += TIME_INTERVAL;
    }

    fp = fopen("stat.dat", "a");
  
    for (i = 0; i < n; i ++) {
      if (v2s[i].map == STATE_MAP_LINK) {
        bwsum = 0;
        reqcost = 0;
        for (j = 0; j < req[i].links; j ++) {
          bwsum += req[i].link[j].bw;
          if (v2s[i].spath[j].len <=0) printf("******error**********\n");
          reqcost += req[i].link[j].bw * v2s[i].spath[j].len;
        }
        opcost +=reqcost;
        count ++;
      }
    }
    fprintf(fp, "%d %lf\n", count, opcost/(double)count);

    /*double maxlinkload;
    for (i = 0; i < sub.links; i ++) {
      double t = sub.link[i].bw - s2v_l[i].rest_bw;
      if (t > maxlinkload) maxlinkload = t;
    }
    
    fprintf(fp, " %lf\n", maxlinkload);
    */

    fclose(fp);

  return 0;
}
