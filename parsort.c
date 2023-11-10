#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int compare_i64(const void *left_, const void *right_) {
  int64_t left = *(int64_t *)left_;
  int64_t right = *(int64_t *)right_;
  if (left < right) return -1;
  if (left > right) return 1;
  return 0;
}

void seq_sort(int64_t *arr, size_t begin, size_t end) {
  size_t num_elements = end - begin;
  qsort(arr + begin, num_elements, sizeof(int64_t), compare_i64);
}

// Merge the elements in the sorted ranges [begin, mid) and [mid, end),
// copying the result into temparr.
void merge(int64_t *arr, size_t begin, size_t mid, size_t end, int64_t *temparr) {
  int64_t *endl = arr + mid;
  int64_t *endr = arr + end;
  int64_t *left = arr + begin, *right = arr + mid, *dst = temparr;

  for (;;) {
    int at_end_l = left >= endl;
    int at_end_r = right >= endr;

    if (at_end_l && at_end_r) break;

    if (at_end_l)
      *dst++ = *right++;
    else if (at_end_r)
      *dst++ = *left++;
    else {
      int cmp = compare_i64(left, right);
      if (cmp <= 0)
        *dst++ = *left++;
      else
        *dst++ = *right++;
    }
  }
}

void fatal(const char *msg) __attribute__ ((noreturn));

void fatal(const char *msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(1);
}

void merge_sort(int64_t *arr, size_t begin, size_t end, size_t threshold) {
  assert(end >= begin);
  size_t size = end - begin;

  if (size <= threshold) {
    seq_sort(arr, begin, end);
    return;
  }

  // recursively sort halves in parallel

  size_t mid = begin + size/2;

  pid_t pid_left = fork();//parallel merge for left half of array
  if (pid_left == -1) {
    fprintf(stderr, "Error: fork failed to start a new process");
    exit(1);
  } else if (pid_left == 0) {
      // this is now in the child process
      merge_sort(arr, begin, mid, threshold);
      exit(0);
  }
  // if pid is not 0, we are in the parent process

  pid_t pid_right = fork();//parallel merge for right half of array
  if (pid_right == -1) {
      // fork failed to start a new process
      // handle the error and exit
      fprintf(stderr, "Error: fork failed to start a new process");
      exit(1);
  } else if (pid_right == 0) {
      // this is now in the child process
      merge_sort(arr, mid, end, threshold);
      exit(0);
  }  
  // if pid is not 0, we are in the parent process

  //wait for left and right child to be done
  int left_wstatus; 
  // blocks until the process indentified by pid_to_wait_for completes
  pid_t actual_left_pid = waitpid(pid_left, &left_wstatus, 0);
  if (actual_left_pid == -1) {
    fprintf(stderr, "Error: waitpid failure");
    // handle the error and exit
    exit(1);
    
  }
  if (!WIFEXITED(left_wstatus)) {
    fprintf(stderr, "Error: subprocess crashed, was interrupted, or did not exit normally");
    exit(1);
  }
  if (WEXITSTATUS(left_wstatus) != 0) {
    fprintf(stderr, "Error: subprocess returned a non-zero exit code");
    exit(1);
  }

  int right_wstatus;
  pid_t actual_right_pid = waitpid(pid_right, &right_wstatus, 0);
  if (actual_right_pid == -1) {
    fprintf(stderr, "Error: waitpid failure");
    exit(1);
  }
  if (!WIFEXITED(right_wstatus)) {
    fprintf(stderr, "Error: subprocess crashed, was interrupted, or did not exit normally");
    exit(1);
  }
  if (WEXITSTATUS(right_wstatus) != 0) {
    fprintf(stderr, "Error: subprocess returned a non-zero exit code");
    exit(1);
  }

  // allocate temp array now, so we can avoid unnecessary work
  // if the malloc fails
  int64_t *temp_arr = (int64_t *) malloc(size * sizeof(int64_t));
  if (temp_arr == NULL) {
    fatal("malloc() failed");
    // handle the error and exit
    exit(1);
  }

  // child processes completed successfully, so in theory
  // we should be able to merge their results
  merge(arr, begin, mid, end, temp_arr);

  // copy data back to main array
  for (size_t i = 0; i < size; i++)
    arr[begin + i] = temp_arr[i];

  // now we can free the temp array
  free(temp_arr);

  // success!
}

int main(int argc, char **argv) {
  // check for correct number of command line arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <filename> <sequential threshold>\n", argv[0]);
    return 1;
  }

  // process command line arguments
  const char *filename = argv[1];
  char *end;
  size_t threshold = (size_t) strtoul(argv[2], &end, 10);
  if (end != argv[2] + strlen(argv[2])) {
    fprintf(stderr, "Error: Invalid threshold: %s\n", argv[2]);
  }

  //open the file
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
      // file couldn't be opened: handle error and exit
      fprintf(stderr, "Error: file couldn't be opened\n");
      return 1;
    }

  //determine the size of the file
    struct stat statbuf;
    int rc = fstat(fd, &statbuf);
    if (rc != 0) {
        // handle fstat error and exit
      fprintf(stderr, "Error: fstat error\n");
      return 1;
    }
    size_t file_size_in_bytes = statbuf.st_size;

  //map the file into memory using mmap
    int64_t *data = mmap(NULL, file_size_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (data == MAP_FAILED) {
        // handle mmap error and exit
      fprintf(stderr, "Error: mmap error\n");
      return 1;
    }

  merge_sort(data, 0, file_size_in_bytes / 8, threshold);

  //unmap and close the file
  munmap(data, file_size_in_bytes);

  return 0;
}
