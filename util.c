#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "def.h"
#include "embed.h"

extern struct request * req;
extern struct substrate_network sub;
extern int n;

void remove_node_map(struct s2v_node * s2v_n, int snode, int index) {
  int i;
  //printf("remove node map %d %d %d %d %d\n", snode, index, s2v_n[snode].req_count, s2v_n[snode].req[index], s2v_n[snode].vnode[index]);
  s2v_n[snode].rest_cpu += s2v_n[snode].cpu[index];
  for (i = index; i < s2v_n[snode].req_count - 1; i ++) {
    s2v_n[snode].req[i] = s2v_n[snode].req[i+1];
    s2v_n[snode].vnode[i] = s2v_n[snode].vnode[i+1];
    s2v_n[snode].cpu[i] = s2v_n[snode].cpu[i+1];
  }    
  s2v_n[snode].req_count --;
}

void add_node_map(struct s2v_node * s2v_n, struct req2sub * v2s, int snode, int reqid, int nodeid) {
  //printf("add node map %d %d %d %d\n", snode, reqid, nodeid, s2v_n[snode].req_count);
  int i;
  s2v_n[snode].rest_cpu -= req[reqid].cpu[nodeid];
  i = s2v_n[snode].req_count;
  s2v_n[snode].req[i] = reqid;
  s2v_n[snode].vnode[i] = nodeid;
  s2v_n[snode].cpu[i] = req[reqid].cpu[nodeid];
  s2v_n[snode].req_count ++;

  v2s[reqid].snode[nodeid] = snode;
}

void remove_link_map(struct s2v_link * s2v_l, int slink, int index) {
  int l;
  s2v_l[slink].rest_bw += s2v_l[slink].bw[index];

  //printf("slink %d rv bw %lf rest_bw %lf\n", slink, s2v_l[slink].bw[index], s2v_l[slink].rest_bw);

  for (l = index; l < s2v_l[slink].count -1; l ++) {
    s2v_l[slink].req[l] = s2v_l[slink].req[l+1];
    s2v_l[slink].vlink[l] = s2v_l[slink].req[l+1];
    s2v_l[slink].bw[l] = s2v_l[slink].bw[l+1];
  }
  s2v_l[slink].count --;
  //printf("s2v_l %d\n", s2v_l[slink].count);
}

void add_link_map(struct s2v_link * s2v_l, int slink, int reqid, int vlink) {
  int t = s2v_l[slink].count;

  //printf("slink %d s2v_l %d\n", slink, t);

  s2v_l[slink].req[t] = reqid;
  s2v_l[slink].vlink[t] = vlink;
  s2v_l[slink].bw[t] = req[reqid].link[vlink].bw;
  s2v_l[slink].rest_bw -= req[reqid].link[vlink].bw;

  /*if (s2v_l[slink].rest_bw < 0) 
    printf("add bw reqid %d vlink %d bw %lf\n", reqid, vlink, req[reqid].link[vlink].bw);
  printf("slink %d rest_bw %lf\n", slink, s2v_l[slink].rest_bw);
  */
  s2v_l[slink].count ++;
}

void release_split_link(struct s2v_link * s2v_l, struct req2sub * v2s, int index) {
  int i, j, k, t;
  //printf("release resource %d\n", index);

  for (i = 0; i < req[index].links; i ++) {
    for (j = 0; j < v2s[index].spath[i].len; j ++) {
      t = v2s[index].spath[i].link[j];
      for (k = 0; k < s2v_l[t].count; k ++) {
        if (s2v_l[t].req[k] == index) {
          remove_link_map(s2v_l, t, k);
          break;
        }
      }
    }
  }
}

void release_resource(struct s2v_node * s2v_n, struct s2v_link * s2v_l, struct req2sub * v2s, int index) {
  int i, j, k, t;
  //printf("release resource %d\n", index);
    for (i = 0; i < req[index].nodes; i ++) {
      t = v2s[index].snode[i];
      for (j = 0; j < s2v_n[t].req_count; j ++) {
        if (s2v_n[t].req[j] == index) {
          remove_node_map(s2v_n, t, j);
          break;
        }
      }    
    }

  if (v2s[index].map == STATE_MAP_NODE) 
    return;

  for (i = 0; i < req[index].links; i ++) {
    for (j = 0; j < v2s[index].spath[i].len; j ++) {
      t = v2s[index].spath[i].link[j];
      for (k = 0; k < s2v_l[t].count; k ++) {
        if (s2v_l[t].req[k] == index) {
          remove_link_map(s2v_l, t, k);
          break;
        }
      }
    }
  }
}

void calc_shortest_path(struct shortest_path * spath, struct substrate_network * sub) {
  // Floyd's All pairs shortest path algorithm (O (n^3) ) 
  // input is adjacency matrix output is matrix of shortest paths
  // C is the adjacency matrix
  // n is the order of the square matrix 
  // A is the all pairs shortest paths matrix 
  // we assume that A is allocated by the caller

  int i,j,k;
  int n = sub->nodes;

  // set all connected positions to 1
  // and all unconnected positions to infinity 
  for (i = 0; i < n; i ++) {
    for (j = 0; j < n; j ++) {
      if (i == j) {
        (*(spath+i*n+j)).length = 0;
        (*(spath+i*n+j)).next = i;
      } else {
        (*(spath+i*n+j)).length = 999999999; // does this look like infinity ?
        (*(spath+i*n+j)).next = -1;
      }
    }
  }
  for (k=0; k<sub->links; k++) {
    if (sub->link[k].from == -1 && sub->link[k].to == -1) continue;
    i = sub->link[k].from;
    j = sub->link[k].to;
    (*(spath+i*n+j)).length = 1;
    (*(spath+i*n+j)).next = j;
    (*(spath+j*n+i)).length = 1;
    (*(spath+j*n+i)).next = i;
  }

  /*
  for (i = 0; i < n; i ++) {
    for (j = 0; j < n; j ++) {
      printf("%d %d %d %d\n", i, j, (*(spath+i*n+j)).length, (*(spath+i*n+j)).next);
    }
  }
  */

  //printf("**********\n");

  // for each route via k from i to j pick 
  // any better routes and replace A[i][j]
  // path with sum of paths i-k and j-k
  for (k=0; k<n; k++)
    {
      for (i=0; i<n; i++)
        {
          for (j=0; j<n; j++)
            {
              if ( (*(spath+i*n+k)).length + (*(spath+k*n+j)).length < (*(spath+i*n+j)).length)
                {
                  // A[i][j] = A[i][k] + A[k][j];
                  (*(spath+i*n+j)).length = (*(spath+i*n+k)).length+ (*(spath+k*n+j)).length;
                  (*(spath+i*n+j)).next = (*(spath+i*n+k)).next;
                }
            }
        }
    }
  /*
  for (i = 0; i < n; i ++) {
    for (j = 0; j < n; j ++) {
      printf("%d %d %d %d\n", i, j, (*(spath+i*n+j)).length, (*(spath+i*n+j)).next);
    }
  }
  */
} // Floyd's algorithm

void init_s2v_l(struct s2v_link * s2v_l) {
  memset(s2v_l, 0, sizeof(struct s2v_link)* sub.links);
  int i;
  for (i = 0; i < sub.links; i ++) {
    s2v_l[i].rest_bw = sub.link[i].bw;
  }
}

void init_v2s(struct req2sub * v2s) {
  int i;
  for (i = 0; i < n; i ++) {
    memset(v2s[i].spath, 0, sizeof(struct path)*MAX_REQ_LINKS);
  }
}

void print_map(struct req2sub * v2s) {
  int i, j, k;
  for (i = 0; i < n; i ++) {
    if (v2s[i].map == STATE_MAP_LINK) {
      for (j = 0; j < req[i].nodes; j ++) {
        printf(" %d.%d <--> %d ", i, j, v2s[i].snode[j]);
      }
      printf("\n");
      for (j = 0; j < req[i].links; j ++) {
        printf("link %d ", j);
        for (k = 0; k < v2s[i].spath[j].len; k ++) {
          printf("%d->", v2s[i].spath[j].link[k]);
        }
        printf("\n");
      }
    }
  }
}

void print_s2v_l(struct s2v_link * s2v_l) {
  int i;
  for (i = 0; i < sub.links; i ++) {
    if (s2v_l[i].rest_bw < 100) {
      printf("sublink %d rest_bw %lf\n",i, s2v_l[i].rest_bw); 
    }
  }
}
