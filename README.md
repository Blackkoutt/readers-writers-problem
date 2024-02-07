# Table Of Content

- [General info](#general-info)
- [Technologies](#technologies)
- [Getting Started](#getting-started)

# General info
The project is a solution to the problem of readers and writers - the problem of thread synchronization.
Below is a description of the problem being solved:

"The reading room is used all the time by a certain number of readers and writers (two types of threads),
while at the same time there can be either any number of readers in it,
or one writer, or no one - never otherwise."

The project consists of 3 possible solutions to the problem:
- starving writers
- starving readers
- no starvation (optimal solution)

The POSIX Threads library from the LINUX API was used to synchronize the threads.

# Technologies
The project was written using the following technologies:

![Static Badge](https://img.shields.io/badge/C%20programming%20language-%23004283?style=for-the-badge&logo=C)

![Static Badge](https://img.shields.io/badge/Linux%20API-%23000000?style=for-the-badge&logo=linux&logoColor=%23000000&labelColor=%23ffe15c)

![Static Badge](https://img.shields.io/badge/POSIX%20THREADS%20LIBRARY-%23000000?style=for-the-badge&logoColor=%23000000&labelColor=%23ffe15c)

# Getting Started

The entire project consists of 12 files, with individual solutions to the problem of
readers and writers are located, respectively, in the files no_starvation.c,
readers_starvation.c, writers_starvation.c.

The remaining files contain the structures and functions auxiliary functions used in the project such as:
- implementation of FIFO queue (file fifo.c, fifo.h)
- implementation of linked list (file list.c, list.h)
- implementation of the TicketLock mechanism (file ticket_lock.c ticket_lock.h)
- validation of given arguments of a program (file validate.c, validate.h)

**To compile each of the programs, just use the makefile by typing the "make" command in the console.**
**Then each of the programs can be run by the following command:**
   ```
   ./program_name {number_of_reader_threads} {number_of_writer_threads} {option "-info"}
   ```

> [!NOTE]
> When running the program with the "-info" parameter, detailed
information such as the queues of readers and writers and the list of people in the
reading room.
