CONTRIBUTIONS

Jayden:
- implemented file processing
- implemented and debugged parallel mergesort
Nicolas:
- implemented error handling
- implemented and debugged parallel mergesort

REPORT

Experiment Results:

real	0m0.420s
user	0m0.407s
sys	0m0.011s

real	0m0.248s
user	0m0.428s
sys	0m0.024s

real	0m0.175s
user	0m0.491s
sys	0m0.030s

real	0m0.134s
user	0m0.567s
sys	0m0.055s

real	0m0.151s
user	0m0.591s
sys	0m0.068s

real	0m0.142s
user	0m0.561s
sys	0m0.083s

real	0m0.155s
user	0m0.586s
sys	0m0.130s

real	0m0.161s
user	0m0.606s
sys	0m0.156s

After performing this experiment a number of times, it seems that the results are quite consistent that as the threshold decreases, 
where we allow for more parallelism, the 'real' time tends to decrease until a certain point. 

In our merge sort, we recursively divide the array into halves until the subarrays reach a size that's small enough to sort using qsort, specified by our threshold value.
Each process is tasked with sorting a portion of the array, and because these portions are independent of one another, they can be sorted simultaneously without any need for synchronization. 
This is where the OS kernel can schedule different processes on different CPU cores, allowing for concurrent execution.

Intuitively this makes sense because as the threshold for switching to sequential sort decreases, more processes are spawned to handle smaller parts of the array. 
Initially, this increases efficiency as more cores get involved in the sorting process, leading to a reduction in real time, as distributing this work across multiple cores can reduce the overall time taken.

However, there are diminishing returns to increase parallelism, as we see since the 'real' time increases after a certain point.
This might be caused by the overhead of creating, scheduling, and terminating processes, which might be more expensive than the actual sorting operation once the number of 
parallel processes is large enough. 
 