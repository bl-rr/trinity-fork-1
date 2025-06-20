#ifndef MD_TRIE_DEFS_H
#define MD_TRIE_DEFS_H

#include "compact_vector.h"
#include <assert.h>
// #include <boost/bimap.hpp>
#include <cinttypes>
#include <compressed_bitmap.h>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <map>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint64_t preorder_t;
typedef uint64_t n_leaves_t;
typedef uint64_t node_pos_t;
typedef uint8_t level_t;
typedef uint64_t morton_t;
typedef uint64_t dimension_t;
typedef uint64_t point_t;

const preorder_t null_node = -1;

template <dimension_t DIMENSION>
class tree_block;

template <dimension_t DIMENSION>
class data_point;

/**
 * node_info and subtree info are used when splitting the treeblock to create a
 * new frontier node node_info stores the additional information of number of
 * children each node has subtree_info stores the additional information of the
 * subtree size of that node
 */

struct node_info
{
  preorder_t preorder_ = 0;
  preorder_t n_children_ = 0;
};

struct subtree_info
{
  preorder_t preorder_ = 0;
  preorder_t subtree_size_ = 0;
};

template <dimension_t DIMENSION>
struct frontier_node
{
  preorder_t preorder_;
  tree_block<DIMENSION> *pointer_;
};

typedef unsigned long long int TimeStamp;

TimeStamp
GetTimestamp()
{
  struct timeval now;
  gettimeofday(&now, nullptr);

  return now.tv_usec + (TimeStamp)now.tv_sec * 1000000;
}

/**
 * total_points_count: total number of points in the data set
 * discount_factor: only consider total_points_count/discount_factor number of
 * points total_treeblock_num: total number of treeblocks in MdTrie. Used for
 * size calculation level_to_num_children: maps level to number of children a
 * node has at that level
 */

n_leaves_t total_points_count = 0;
int discount_factor = 1;
level_t trie_depth_;
morton_t level_to_num_children[64] = {0};
preorder_t max_tree_nodes_ = 512;
level_t max_depth_;

/**
 * p_key_to_treeblock_compact: maps primary key to treeblock pointers in a
 * compact pointer vector dimension_to_num_bits: maps the attribute index to bit
 * widths of that attribute start_dimension_bits: the level to which we start
 * considering bits from that attribute no_dynamic_sizing: flag to indicate
 * whether we set the treeblock size to the same value.
 */

bitmap::CompactPtrVector *p_key_to_treeblock_compact;
std::vector<level_t> dimension_to_num_bits;
std::vector<level_t> start_dimension_bits;
bool no_dynamic_sizing = false;
std::map<void *, void *> old_ptr_to_new_ptr;
std::map<void *, size_t> ptr_to_file_offset;
size_t current_file_offset = 0;
n_leaves_t treeblock_ctr = 0;

std::mutex cache_lock;

std::unordered_map<uint64_t, uint64_t> client_to_server;
bool enable_client_cache_pkey_mapping = false;
bool REUSE_RANGE_SEARCH_CHILD = true;

int current_dataset_idx = 0;
int num_treeblock_expand = 0;
int lookup_scanned_nodes = 0;

uint64_t bare_minimum_count = 0;
uint64_t checked_points_count = 0;
int query_optimization = 2;
bool is_collapsed_node_exp = false;

void create_level_to_num_children(std::vector<level_t> bit_widths,
                                  std::vector<level_t> start_bits,
                                  level_t max_level)
{

  dimension_to_num_bits = bit_widths;
  start_dimension_bits = start_bits;
  dimension_t num_dimensions = bit_widths.size();

  for (level_t level = 0; level < max_level; level++)
  {

    dimension_t dimension_left = num_dimensions;
    for (dimension_t j = 0; j < num_dimensions; j++)
    {

      if (level + 1 > bit_widths[j] || level < start_dimension_bits[j])
        dimension_left--;
    }
    level_to_num_children[level] = dimension_left;
  }
}

// #define USE_LINEAR_SCAN // Possibly disable at microbenchmark
#endif // MD_TRIE_DEFS_H
