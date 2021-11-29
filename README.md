# MPI Hw4 Pthread Smooth

E94086107 張娟鳴

## Building

- Compile

```bash=
g++ -g -Wall -o [Output Filename] hw4.cpp
```

- Execute

  - n: numbers of threads want to have

```bash=
./[Output Filename] n
```

- Result:  
![](/result_screenshot/execute_res.png)

- Compare with original Smooth process:
![](/result_screenshot/diff.png)

## Implementation

- Read "input.bmp", get starting time.
- Initialize variable.
- Create thread, in each thread:
  1. Calculate the size and starting point I need to smooth.
  2. id=0 get correct bmp data.
  3. Smoothing.
  4. Waiting each thread complete.
  5. Repeat 2~4 1000 times.
- Waiting thread end.
- Save "output.bmp", get end time.
- Get total time.

- the barrier method I implement is "busy waiting and mutex".

## Result Analysis

- The execution time will be influenced by the number of processor in the computer and the number of thread.  
- The picture below is the different number of thread with the CPU usage percentage.
  - thread = 2
    - ![](/result_screenshot/thread_2.png)
  
  - thread = 4
    - ![](/result_screenshot/thread_4.png)
  
  - thread = 6
    - ![](/result_screenshot/thread_6.png)
  
  - thread = 8  
    - ![](/result_screenshot/thread_8.png)
- We could found that if our thread number < total processor, we can't have the best usage of CPU.
  - If our thread number > total processor, we will split CPU resources equally.
- However, by the execution time, we found that the smallest time is occur when thread number = 2. But all the other thread still faster then single thread.
- I guess this is because the mutex instruction will cost a little bit more time, I will try other method later.

## Difficulties

- calculating total time
  - In the beginning, I use `clock()` to get start and end time.  However, `clock()` will measure your CPU time use by my process, so I only get total time.  
  - Then, I changed my function into `clock_gettime()`.  Then I measure execution time correctly.  
  - By the way, when I use `clock()` in my program, I noticed that when I increasing my thread number, the execution time will also increase. The only two of the execution time which are close is `./hw4 1` and `./hw4 2`. I speculate this is because of the `pthread_mutex_lock()` is the main cause.  

- swap function
  - Each time when we smooth, we need to swap "BMPSaveData" and "BMPData" in the beginning to get the data we calculate.  
  - At first, I forgot that I just need to swap at one of the threads, I swap it at all threads, then I got a strange answer.
  - After, discussing with my classmate, I found the mistake.  
  - The wrong code could also produce a correct answer when total thread = 1. When the total thread > 1, it would produce a  file without nothing inside it.

## References

- [並行程式設計](https://hackmd.io/@sysprog/concurrency)
