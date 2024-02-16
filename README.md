
# farm

Multi-threaded and multi-process application written in C, utilizing the Master-Worker design pattern.

Project for the Operating Systems Laboratory Course - Final Grade: 30/30

## System Architecture Overview

![architecture](https://github.com/Faxatos/farm/assets/57751761/86cbd879-e477-4b81-ba92-3c526573b83d)

The farm program consists primarily of two processes:

### <a name="opts"></a>MasterWorker

A multi-threaded process composed of one Master thread and *n* Worker threads. The program takes a list of binary files (treating its contents as a list of long integers) and a certain number of optional arguments. The optional arguments that can be passed to the MasterWorker process are as follows:
   + **-n** *\<nthread>*: specifies the number of Worker threads for the MasterWorker process (default value: 4; max value: 256)
   + **-q** *\<qlen>*: length of the concurrent queue between the Master thread and Worker threads (default value: 8; max value: 512)
   + **-d** *\<directory-name>*: specifies a directory containing binary files and possibly other directories containing binary files; the binary files will be used as input files for calculation
   + **-t** *\<delay>*: time in milliseconds between sending two consecutive requests to Worker threads by the Master thread (default value 0; max value: 4096 ms)
   
The name of the generic input file, after checking that the file associated with the name is a regular file and that it has the required extension (taking *.dat* as a reference, but it can be changed), is sent to one of the Worker threads via a shared concurrent queue. The generic Worker thread reads the entire contents of the file whose name it received as input from the disk, performs a calculation on the elements, and sends the result (along with the file name) to the Collector process via a local socket connection.The process also performs signal management.

### Collector

A process that waits for the result of various calculations from the Worker threads of MasterWorker, and upon completion, prints the obtained values to standard output, ordering the print based on the result in ascending order. The two processes communicate through a local socket connection.

More details related to implementation requirements and implementation choices are described in the *report.pdf* file.

## Getting Started

### Prerequisites

+ OS: [Linux Ubuntu 20.04 LTS](https://releases.ubuntu.com/)
+ Hardware: multi-core machine (at least 2 cores)

### Installation

1. Navigate to the root folder of the project.
2. Run the following command to trigger the Makefile:
  ```sh
  make
  ```

### Usage

Run the program using the optional arguments described in the [previous section](#opts). The optional arguments must be inserted before the list of binary files (exept the *-d* flag, which can be inserted in any point)
```sh
farm [optional args] file1.dat file2.dat ...
  ```

To test the program, it's possible to use pre-written tests located inside the `test.sh` file. Run the following command to execute the tests:
```sh
make test
  ```
  
For a detailed understanding of the pre-written tests, please refer to the comments in the *test.sh* file and the *report.pdf*.

## License

Distributed under the MIT License. See `LICENSE.txt` for more information.
