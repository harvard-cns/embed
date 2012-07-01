#ifndef EMBED_H
#define EMBED_H

struct struct_link {
  int from;
  int to;
  double bw;
};

struct request {
  int split;
  int time;
  int topo;
  int duration;
  int nodes;
  int links;
  double revenue;
  double cpu[MAX_REQ_NODES];
  struct struct_link link[MAX_REQ_LINKS];
};

struct substrate_network {
  int nodes;
  int links;
  double cpu[MAX_SUB_NODES];
  struct struct_link link[MAX_SUB_LINKS];
};

struct s2v_node {
  int req_count;
  int req[MAX_REQ_PER_NODE];
  int vnode[MAX_REQ_PER_NODE];
  double cpu[MAX_REQ_PER_NODE];
  double rest_cpu;
};

struct s2v_link {
  int count;
  int req[MAX_REQ_LINKS]; 
  int vlink[MAX_REQ_LINKS];
  double bw[MAX_REQ_LINKS];
  double rest_bw;
};

#define MAX_SNODE_PER_PATH 100
struct path {
  int len; //# of nodes on the path
  int link[MAX_SNODE_PER_PATH];
  double bw;
};

#define MAX_SLINK_PER_VLINK 100
struct req2sub {
  int map; 
  int maptime;
  int snode[MAX_REQ_NODES];
  struct path spath[MAX_REQ_LINKS];
};

struct shortest_path {
  int length;
  int next;
};

struct bneck {
  int slink;
  int req;
  int vlink;
};

#endif//EMBED_H

