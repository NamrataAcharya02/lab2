# You Spin Me Round Robin

Given a set of processes with their arrival time and burst time, and a quantum length, this program simulates round robin scheduling. It outputs the average waiting time and average response time as well.

## Building

```
make
```

## Running

To run the program with a process file "file" and a quantum length "quantum", use the following format:
```
./rr file quantum
```
For instance, assume the process file is named "processes.txt" and contains each process' pid, arrival time and burst time. Say the quantum length is 3. Then the program can be run as:
```
./rr processes.txt 3
```

The output of running the program looks something like this:

```
Average waiting time: <average waiting time>
Average response time: <average response time>

```

## Testing

The program can be tested by running

```
python -m unittest
```
The result is that all tests pass. 

## Cleaning up

```shell
make clean
```
