#include "bfs.h"

#include <cstddef>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <chrono>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

class ver_set {
public:
  unsigned max_size;
  int *vertices;
  std::atomic<unsigned> count;

  ver_set(size_t _max_size = 10) {
    max_size = _max_size;
    vertices = new int[max_size];
    count = 0;
  }
  ~ver_set() { delete[] vertices; }

  void clear() { count = 0; }

  int &operator[](int idx) { return vertices[idx]; }
  const int &operator[](int idx) const { return vertices[idx]; }
};

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// new_frontier.
void top_down_step(Graph g, ver_set *frontier, ver_set *new_frontier,
                   int *distances) {
//  make parallel
#pragma omp parallel for schedule(guided)
  for (int i = 0; i < frontier->count; i++) {

    int node = (*frontier)[i];

    int start_edge = g->outgoing_starts[node];
    int end_edge = (node == g->num_nodes - 1) ? g->num_edges
                                              : g->outgoing_starts[node + 1];

    // attempt to add all neighbors to the new frontier
    for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
      int outgoing = g->outgoing_edges[neighbor];
      // add compare and swap
      if (distances[outgoing] == NOT_VISITED_MARKER &&
          __sync_bool_compare_and_swap(&(distances[outgoing]),
                                       NOT_VISITED_MARKER,
                                       distances[node] + 1)) {
        (*new_frontier)[new_frontier->count++] = outgoing;
      }
    }
  }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol) {

  ver_set list1(graph->num_nodes);
  ver_set list2(graph->num_nodes);

  ver_set *frontier = &list1;
  ver_set *new_frontier = &list2;

// initialize all nodes to NOT_VISITED
#pragma omp parallel for
  for (int i = 0; i < graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;

  // setup frontier with the root node
  (*frontier)[frontier->count++] = ROOT_NODE_ID;
  sol->distances[ROOT_NODE_ID] = 0;

  while (frontier->count != 0) {

#ifdef VERBOSE
    double start_time = CycleTimer::currentSeconds();
#endif

    new_frontier->clear();

    top_down_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
    double end_time = CycleTimer::currentSeconds();
    printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

    // swap pointers
    std::swap(frontier, new_frontier);
  }
}

void btm_up_step(Graph g, ver_set *un_vis, ver_set *un_vis_next,
                 ver_set &new_frontier, int *distances, int &unvis_cnt) {
#pragma omp parallel for schedule(guided)
  for (int i = 0; i < un_vis->count; i++) {
    if (distances[(*un_vis)[i]] == NOT_VISITED_MARKER) {
      int node = (*un_vis)[i];
      int start_edge = g->incoming_starts[node];
      int end_edge = (node == g->num_nodes - 1) ? g->num_edges
                                                : g->incoming_starts[node + 1];

      // attempt to find node's parent
      for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
        int incoming = g->incoming_edges[neighbor];
        if (distances[incoming] != NOT_VISITED_MARKER) {
          new_frontier[new_frontier.count++] = node;
          unvis_cnt--;
          break;
        }
      }
    }
  }
}

void garbage_collect(ver_set *un_vis, ver_set *un_vis_next, int *distances) {
  un_vis_next->clear();
  int cnt = 0;
  // #pragma omp parallel for
  for (int i = 0; i < un_vis->count; i++) {
    if (distances[(*un_vis)[i]] == NOT_VISITED_MARKER) {
      (*un_vis_next)[cnt++] = (*un_vis)[i];
    }
  }
  un_vis_next->count = cnt;
}

void bfs_bottom_up(Graph graph, solution *sol) {
  ver_set l1(graph->num_nodes), l2(graph->num_nodes);
  ver_set *un_vis = &l1, *un_vis_next = &l2, new_frontier(graph->num_nodes);

#pragma omp parallel for
  for (int i = 0; i < graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;

#pragma omp parallel for
  for (int i = 0; i < graph->num_nodes - 1; i++)
    (*un_vis)[i] = i + 1;

  un_vis->count = graph->num_nodes - 1;
  sol->distances[ROOT_NODE_ID] = 0;
  int depth_cnt = 1;
  int unvis_cnt = graph->num_nodes - 1;

  while (unvis_cnt > 0) {
    new_frontier.clear();
    // auto start = std::chrono::steady_clock::now();
    btm_up_step(graph, un_vis, un_vis_next, new_frontier, sol->distances,
                unvis_cnt);
    // auto end = std::chrono::steady_clock::now();
    // std::chrono::duration<double> elapsed_seconds = end-start;
    // std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
    if (new_frontier.count == 0)
      return;

#pragma omp parallel for
    for (int i = 0; i < new_frontier.count; i++) {
      sol->distances[new_frontier[i]] = depth_cnt;
    }

    if (unvis_cnt < un_vis->count / 2) {
      garbage_collect(un_vis, un_vis_next, sol->distances);
      std::swap(un_vis, un_vis_next);
    }
    depth_cnt++;
  }
}

void bfs_hybrid(Graph graph, solution *sol) {
  ver_set l1(graph->num_nodes), l2(graph->num_nodes), l3(graph->num_nodes),
      l4(graph->num_nodes);
  ver_set *front = &l1, *new_frontier = &l2;
  ver_set *un_vis = &l3, *un_vis_next = &l4;

#pragma omp parallel for
  for (int i = 0; i < graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;
#pragma omp parallel for
  for (int i = 0; i < graph->num_nodes - 1; i++)
    (*un_vis)[i] = i + 1;
  un_vis->count = graph->num_nodes - 1;

  (*front)[front->count++] = ROOT_NODE_ID;
  sol->distances[ROOT_NODE_ID] = 0;
  int depth_cnt = 1;
  int unvis_cnt = (graph->num_nodes - 1);

  while (front->count > 0) {
    new_frontier->clear();
    if (un_vis->count > front->count) {
      top_down_step(graph, front, new_frontier, sol->distances);
      unvis_cnt -= new_frontier->count;
    } else {
      btm_up_step(graph, un_vis, un_vis_next, *new_frontier, sol->distances,
                  unvis_cnt);
#pragma omp parallel for
      for (int i = 0; i < new_frontier->count; i++) {
        sol->distances[(*new_frontier)[i]] = depth_cnt;
      }
    }
    if (unvis_cnt < un_vis->count / 4) {
      garbage_collect(un_vis, un_vis_next, sol->distances);
      std::swap(un_vis, un_vis_next);
    }
    depth_cnt++;
    std::swap(front, new_frontier);
  }
}