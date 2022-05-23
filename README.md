# fsbw: FileSystem BandWidth

A simple tool and set of scripts to simulate read and write operations on a Lustre filesystem.


## Usage

First, compile the program using `make`. The result is the executable `bin/fsbw`. Make sure you add `bin` to your `PATH` environment variable.

Executing `./bin/fsbw` without any option will display the program help:

```
cdipietrantonio@setonix-01:/software/projects/pawsey0001/cdipietrantonio/fsbw> ./bin/fsbw 
Program help:
----------------
        -f <filename>: path to the file to be read/written (if it does not exist, will be created).
        -B <block size>: size in bytes of each read/write operation. Can use K/M/G postfixes for Kilo/Mega/Giga multiplier, e.g. 32M.
        -c <block count>: number of blocks to read/write.
        -S <file size>: if the file does not exist, a file of size S bytes will be created.
                Can use K/M/G postfixes for Kilo/Mega/Giga multiplier, e.g. 32M.
        -p <read probability>: number in the [0, 1] interval indicating the probability the next operation to perform is a read operation.
        -r: enable random access to the file (default: file read/written sequentially).
        -j: produce output in json format.
        -m: interpret the value passed with -B as maximum number of blocks to be read or written.
```

The program is better used in combination with the Python scripts in the `scripts` directory. The directory contains the following files:

- utils.py: a Python module containing function and class definitions to call `fsbw` and to set Lustre's Progressive File Layouts.
- experiments.py: a script that runs a set of preferined experiments to measure the bandwidth of the filesystem for different use cases.
- plot_results.py: as the name says, a script to plot the output of `experiments.py`.