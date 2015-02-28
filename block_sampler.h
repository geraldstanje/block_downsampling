#ifndef blocksampler_h
#define blocksampler_h

#include "boost/multi_array.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <thread>
using namespace std;

//#define THREADING
//#define DEBUG_OUTPUT

const int row_dim_index = 0;
const int col_dim_index = 1;
const int depth_dim_index = 2;

class BlockIndex {
public:
    int row;
    int col;
    int depth;
    BlockIndex(int row_, int col_, int depth_): row(row_), col(col_), depth(depth_) {} 
};

template <size_t L1, const size_t L2, const size_t L3>
class BlockSampler {
private:
  unsigned int common_value_original;
  std::mutex mtx;
  std::mutex mtx_stdout;
  unsigned int block_size; // the block_size will be set based on the number of dimensions of the array
  // these are the indices for the block, max block_size is 8 (3 dim array)
  const vector<BlockIndex> block_index = {{0, 0, 0},
                                          {1, 0, 0},
                                          {0, 1, 0},
                                          {1, 1, 0},
                                          {0, 0, 1}, 
                                          {1, 0, 1},
                                          {0, 1, 1},
                                          {1, 1, 1}};

  // Create a 3-dimensional array that is L1 x L2 x L3
  typedef boost::multi_array<int, 3> array_type;
  typedef typename array_type::index index;
  typedef boost::multi_array_types::index_range range;
  //array_type arr;
  vector<array_type> arr{1};
  unsigned int downsampled_index;
  vector<unsigned int> dim; // stores the dimension of the image: dim[0] = L1, dim[1] = L2, dim[2] = L3
  unsigned int hist_global[256] = {0};

public:
  BlockSampler(): common_value_original(0), downsampled_index(0), dim{L1, L2, L3} {
    if (L3 > 1) {
      block_size = 8;
    }
    else if (L2 > 1) {
      block_size = 4;
    }
    else {
      block_size = 2;
    }

    arr[0].resize(boost::extents[L1][L2][L3]);
  }

  int 
  rowDim() {
    return dim[row_dim_index];
  }

  int 
  colDim() {
    return dim[col_dim_index];
  }

  int 
  depthDim() {
    return dim[depth_dim_index];
  }

  void
  init(int data[]) {
    arr[0].assign(data, data + (dim[row_dim_index]*dim[col_dim_index]*dim[depth_dim_index]));
  }

  void
  resize() {
    dim[row_dim_index] /= 2;
    dim[col_dim_index] /= 2;
    dim[depth_dim_index] /= 2;
  }

  void
  allocNewImg(unsigned int row_size, unsigned int col_size, unsigned int depth_size) {
    if (row_size == 0) {
      row_size = 1;
    }
    if (col_size == 0) {
      col_size = 1;
    }
    if (depth_size == 0) {
      depth_size = 1;
    }

    arr.push_back(array_type(boost::extents[row_size][col_size][depth_size]));
  }

  // this functions resamples the original image by l number of times
  // if the array is of 2 dimensions, the function spins up multiple threads which
  // work on a row by row basis
  void
  downsample(int l) {
    int rows_per_thread = 2; // is a multiple of the block_size
    std::vector<std::thread> workers;
    bool calc_common_value_original = true;

    for (int i = 0; i < l; i++) {
      allocNewImg(rowDim()/2, colDim()/2, depthDim()/2);

      // this loop would spin up threads which, runs in parallel
      // rows_per_thread means how many rows a thread will process
      // e.g. rows_per_thread = 4 ... thread0 will process row0, row1, row2, row3
      for (int row = 0; row < rowDim(); row+=rows_per_thread) {
        // if the number of rows is smaller than the downsampled image
        if (rowDim() < 2) {
          rows_per_thread = 1;
        }

#ifdef THREADING
        workers.push_back(std::thread(&BlockSampler::thread_downsample,
                                      this, 
                                      row,
                                      rows_per_thread, 
                                      colDim(), 
                                      depthDim(),
                                      calc_common_value_original));
#else 
        thread_downsample(row, rows_per_thread, colDim(), depthDim(), calc_common_value_original);
#endif
      }

#ifdef THREADING
      for (std::thread &t: workers) {
        if (t.joinable()) {
          t.join();
        }
      }
#endif

      if (calc_common_value_original) {
        calc_common_value_original = false;
        common_value_original = get_most_common_value(hist_global);
      }

      resize();

      if (rowDim() < 2 &&
          colDim() < 2 && 
          depthDim() < 2) {
        arr[downsampled_index+1][0][0][0] = common_value_original;
        break;
      }

      downsampled_index++;
    }
  }

  // this funtion runs in the thread context
  // it calculates the mode of each block
  // and it also calculates the mode of the original image and stores it in the common_value_original variable
  void
  thread_downsample(const int &start_row, 
                    const int &rows_per_thread,
                    const int &col_size, 
                    const int &depth_size, 
                    const bool &calc_common_value_original) {
    unsigned int hist[256] = {0};

    for (int row = 0; row < rows_per_thread; row+=2) {
      for (int col = 0; col < col_size; col+=2) {
        for (int depth = 0; depth <= depth_size/2; depth+=2) {
          memset(&hist, 0, sizeof(hist));

#ifdef DEBUG_OUTPUT
          {
            std::unique_lock<std::mutex> lck(mtx_stdout);
            cout << "block: ";
          }
#endif

          // calculate the mode for a block of size 2x2x2 (3d array)
          for (int i = 0; i < block_size; i++) {
            int row_index = start_row + row + block_index[i].row;
            int col_index = col + block_index[i].col;
            int depth_index = depth + block_index[i].depth;

            incr_hist(arr[downsampled_index][row_index][col_index][depth_index], hist);

#ifdef DEBUG_OUTPUT
            assert(row_index < arr[downsampled_index].size());
            assert(col_index < arr[downsampled_index][row_index].size());
            assert(depth_index < arr[downsampled_index][row_index][depth_index].size());

            {
              std::unique_lock<std::mutex> lck(mtx_stdout);
              cout << arr[downsampled_index][row_index][col_index][depth_index] << ", ";
            }
#endif

            if (calc_common_value_original) {
              std::unique_lock<std::mutex> lck(mtx);
              incr_hist(arr[downsampled_index][row_index][col_index][depth_index], hist_global);
            }
          }

#ifdef DEBUG_OUTPUT
          {
            std::unique_lock<std::mutex> lck(mtx_stdout);
            cout << '\n';
          }

          assert((row+start_row)/2 < arr[downsampled_index+1].size());
          assert(col/2 < arr[downsampled_index+1][(row+start_row)/2].size());
          assert(depth/2 < arr[downsampled_index+1][(row+start_row)/2][col/2].size());
#endif

          // calculate the mode
          arr[downsampled_index+1][(row+start_row)/2][col/2][depth/2] = get_most_common_value(hist);
        }
      }
    }
  }

  int
  get_most_common_value(const unsigned int *hist) {
    return std::max_element(hist, hist+256) - hist;
  }

  inline void
  incr_hist(const int &value, unsigned int *hist) {
    hist[value]++;
  }

  void
  print(int img_index) {
    if (arr[img_index][0].size() <= 1) {
      for(index i = 0; i != arr[img_index].size(); ++i) {
        cout << arr[img_index][i][0][0] << ' ';
      }
      cout << "\n\n";
    }
    else if (arr[img_index][0][0].size() <= 1) {
      for(index i = 0; i != arr[img_index].size(); ++i) {
        for(index j = 0; j != arr[img_index][i].size(); ++j) {
          cout << arr[img_index][i][j][0] << ' ';
        }
        cout << '\n';
      }
      cout << '\n';
    }
    else {
      for(index i = 0; i !=  arr[img_index].size(); ++i) {
        for(index j = 0; j != arr[img_index][i].size(); ++j) {
          for(index k = 0; k != arr[img_index][i][j].size(); ++k) {
            cout << arr[img_index][i][j][k] << ' ';
          }
          cout << '\n';
        }
        cout << '\n';
      }
    }
  }

  void
  print_imgs() {
    for (int i = 0; i < arr.size(); i++) {
      print(i);
    }
  }
};

#endif