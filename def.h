#ifndef DEF_H
#define DEF_H

#define OPTION_ALLOW_MIGRATION

#define MAX_REQUESTS 10000

#define MAX_REQ_NODES 100
#define MAX_REQ_LINKS 100

#define MAX_REQ_PER_NODE 1000

#define MAX_SUB_NODES 1000
#define MAX_SUB_LINKS 10000

#define LEN_FILENAME 100

#define MAX_CPU 100
#define MAX_BW 100

#define MAX_NECK_COUNT 100

#define TOPO_GENERAL 0
#define TOPO_STAR 1
#define TOPO_TREE 2
#define TOPO_HUBS 3

#define STATE_MAP_NODE_FAIL -1  //tried but failed
#define STATE_NEW 0   //first state
#define STATE_MAP_NODE  1  //success in mapping node
#define STATE_MAP_LINK  2  
#define STATE_DONE  3  //done
#define STATE_EXPIRE 4

#define NO_MIGRATION 0
#define ROUTE_MIGRATION 1
#define NODE_MIGRATION 2

#define LINK_SPLITTABLE 0
#define LINK_UNSPLITTABLE 1

#define TIMES_TRY 1

#endif//DEF_H
