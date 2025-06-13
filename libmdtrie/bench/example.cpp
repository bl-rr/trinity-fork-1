#include "trie.h"
#include <climits>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>

#include <iostream>
#include <sstream>
#include <string>

#include <sys/stat.h>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

#define NUM_DIMENSION 16

std::size_t get_line_count_shell(const std::string &filename)
{
    std::string cmd = "wc -l < \"" + filename + "\""; // safer: only get line count
    std::array<char, 128> buffer;
    std::string result;

    // Open pipe to run the command
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed");
    }

    // Read the command output
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    try
    {
        return std::stoul(result);
    }
    catch (...)
    {
        return 0; // fallback on conversion error
    }
}

int main()
{

    // std::string filename = "/home/wuyue/Desktop/lbh/gpu-mdtrie/data/full-climate_fever-sentences.csv";
    std::string filename = "../minimal.csv";
    std::ifstream infile(filename);
    std::string line;

    size_t total_count = get_line_count_shell(filename);
    std::cout << "Total count: " << total_count << std::endl;

    dimension_t num_dimensions = NUM_DIMENSION;
    preorder_t max_tree_node = 128;
    level_t trie_depth = 1;
    level_t max_depth = 64;
    no_dynamic_sizing = true;
    bitmap::CompactPtrVector primary_key_to_treeblock_mapping(total_count);

    /* ---------- Initialization ------------ */
    std::vector<level_t> bit_widths = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};
    std::vector<level_t> start_bits = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    create_level_to_num_children(bit_widths, start_bits, max_depth);

    md_trie<NUM_DIMENSION> mdtrie(max_depth, trie_depth, max_tree_node);

    /* ----------- INSERT ----------- */
    TimeStamp start = 0, cumulative = 0;
    // int num_inserted = 0;
    int primary_key = 0;

    // std::vector<int> indices = {
    //     262, 263, 265, 266, 268, 269, 270, 271, 272, 273,
    //     275, 276, 277, 278, 279, 280, 283, 284, 285, 286,
    //     287, 289, 290, 291, 292, 293, 294, 295, 296, 297,
    //     298, 299, 300, 301, 302, 303, 304, 305, 306, 307,
    //     309, 310, 311, 312, 313, 314, 315, 316, 317, 320,
    //     321, 322, 323, 324, 325, 326, 327, 328, 334, 335,
    //     336, 337, 338, 340, 341, 342};

    for (int primary_key = 0; primary_key < total_count && std::getline(infile, line); primary_key++)
    // for (int i : indices)
    {
        std::stringstream ss(line);

        // num_inserted++;
        // if (num_inserted % (1000) == 0)
        // {

        // if (primary_key != 0 && primary_key != 343)
        //     continue;

        // std::cout << "Inserting: " << primary_key << " out of " << total_count << std::endl;
        // }
        data_point<NUM_DIMENSION> point;
        // For lookup correctness checking.
        point.set_coordinate(0, primary_key);

        std::string cell;
        // std::cout << "primary_key: " << primary_key << ", values: \n\t";
        for (dimension_t i = 1; i < num_dimensions && std::getline(ss, cell, ','); ++i)
        {
            point.set_coordinate(i, std::stoull(cell));
            // std::cout << point.get_coordinate(i) << " ";
        }
        // std::cout << std::endl;

        start = GetTimestamp();
        mdtrie.insert_trie(&point, primary_key, &primary_key_to_treeblock_mapping);
        cumulative += GetTimestamp() - start;
    }
    std::cout << "Insertion Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    // /* ---------- LOOKUP ------------ */
    // cumulative = 0;
    // int num_lookup = 0;
    // for (int primary_key = 0; primary_key < total_count; primary_key++)
    // {
    //     num_lookup++;
    //     if (num_lookup % (total_count / 10) == 0)
    //     {
    //         std::cout << "Looking up: " << num_lookup << " out of " << total_count << std::endl;
    //     }

    //     start = GetTimestamp();
    //     data_point<NUM_DIMENSION> *pt = mdtrie.lookup_trie(primary_key, &primary_key_to_treeblock_mapping);
    //     if ((int)pt->get_coordinate(0) != primary_key)
    //     {
    //         std::cerr << "Wrong point retrieved!" << std::endl;
    //     }
    //     cumulative += GetTimestamp() - start;
    // }
    // std::cout << "Lookup Latency per point: " << (float)cumulative / total_count << " us" << std::endl;

    // /* ---------- RANGE QUERY ------------ */
    // cumulative = 0;
    // int num_queries = 3;
    // std::cout << "Creating range queries that return every point. " << std::endl;
    // for (int c = 0; c < num_queries; c++)
    // {
    //     data_point<NUM_DIMENSION> start_range;
    //     data_point<NUM_DIMENSION> end_range;
    //     std::vector<uint64_t> found_points;
    //     /* We used the first coordinate to be the primary key. */
    //     start_range.set_coordinate(0, 0);
    //     end_range.set_coordinate(0, total_count);
    //     for (dimension_t i = 1; i < num_dimensions; i++)
    //     {
    //         start_range.set_coordinate(i, 0);
    //         end_range.set_coordinate(i, (int)(1 << 16));
    //     }

    //     start = GetTimestamp();
    //     mdtrie.range_search_trie(&start_range, &end_range, mdtrie.root(), 0, found_points);
    //     // Coordinates are flattened into one vector.
    //     if ((int)(found_points.size() / num_dimensions) != total_count)
    //     {
    //         std::cerr << "Wrong number of points found!" << std::endl;
    //         std::cerr << "found: " << (int)(found_points.size() / num_dimensions) << std::endl;
    //         std::cerr << "total count: " << total_count << std::endl;
    //     }
    //     else
    //     {
    //         std::cout << "Query - " << c << ", found: " << (int)(found_points.size() / num_dimensions) << ", expected: " << total_count << std::endl;
    //     }
    //     cumulative += GetTimestamp() - start;
    // }
    // std::cout << "Per-query Latency: " << (cumulative / 1000.0) / num_queries << " ms" << std::endl;
    return 0;
}
