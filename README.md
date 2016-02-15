#Summary (Credits to Dr. Ned Nedialkov)
An RSA public key (see e.g. https://en.wikipedia.org/wiki/RSA_(cryptosystem)) is the product of two distinct large prime numbers. Cracking such a key involves finding the two prime numbers.

This program takes as an input a number n that is the product of two prime numbers and efficiently finds these numbers.
The program is run as
```mpirun -np P ./factor n```
where P is the number of processes, factor is the name of the executable, and n is a decimal string representing an integer number that is the product of two prime numbers.

# Algorithm description
My algorithm attempts to distribute an equal amount of work to processors while decrypting the RSA key quickly and efficiently. 

The algorithm was designed to address several important observations:

1. The number of primes below some integer x can be approximated as x/log(x). 
This implies that the gap between primes increases with the natural logarithm of the integer.
2. Finding the next prime number higher than some integer becomes increasing difficult as the integer becomes larger.
3. The prime factorization of a number is unique. This implies that if the given key (n) is divisible by some prime p, then the 
decryption primes are p and n/p.
4. It is only necessary to determine if any prime in the range [2, sqrt(n)] is divisible by n. The rest of the primes in [sqrt(n), n] will
be complements in the division of n.
5. If one processor finds the keys, it must signal all other processors that the key has been found in order to prevent the other processors from 
doing redundant work.


My algorithm works as follows:

Given p processes, each process will be given a start_index, end_index and window_size. The process will search primes in the range [start_index, end_index]. end_index
is equal to start_index + window_size. window_size is equal to the number of bits needed to represent the start index (this makes the search space of each process increase 
logarithmically). No ranges will overlap. For example, given 4 processes, work will be distributed as follows:

* P0 will search \[2, 4\] (2 bits needed to represent 2)
* P1 will search \[4, 7\] (3 bits needed to represent 4)
* P2 will search \[7, 10\] (3 bits needed to represent 7)
* P3 will search \[10, 14\] (4 bits needed to represent 10)
* P0 will search \[14, 18\] (4 bits needed to represent 14)
* P1 will search \[18, 23\] (5 bits needed to represent 18)
* P2 will search \[23, 29\] (5 bits needed to represent 23)
* P3 will search \[29, 34\] (5 bits needed to represent 29)
* P0 will search \[34, 40\] (6 bits needed to represent 34)	
* ...

The widening of the window_size attempts to address observation 1) and 2).

To search for a prime in the range [start, end], the process uses a brute force approach. The process tests if every prime in the range fully divides n. If so, the
key has been decrypted. This addresses observation 3).

The processes keep searching ranges until start_index is greater than sqrt(n). This addresses observation 4).

When a process decrypts n, it sends a message to all other processes. Each process checks if it has received a message every 500 prime check iterations. If a message has been
received, the process stops searching ranges. This addresses observation 5).

Each process tracks how long it runs for and sends its time to process 0, which writes the results to a file.

Pseudocode:

```
function decrypt(n){
	(process_number, num_processes) = Initial_MPI() // fork processes
	t1 = current_time()
	
	// Determine start index
	start = 2 
	temp = start
	for (i = 0; i < process_number; i++){
		temp += bits(temps)
	}	
	start = temp
	
	// range bound
	windowSize = bits(start)
	end = start + windowSize

	// global range bound
	global_end = sqrt(n)

	found = 0 // flag if solution is found 
	current_prime = nextPrime(start) // find the primes higher than start

	Initialize_Receive_From_Any_Channel() // initialize channel to receive a message indicating that key has been found	
	check = 0 // int to flag if process should check the receive channel
	
	while (current_prime < global_end) {
		while (current_prime < global_end and current_prime < end){
			if (n % currentPrime == 0){
				print currentPrime, n/currentPrime
				Send_Found_Message_To_All_Processes();
				found = 1
				break
			}
			check++
			if (check == 500){
				check = 0
				if (Message_Received_from_Channel()){
					found = 1
					break
				}
			}
			current_prime = nextPrime(current_prime)
		}

		if (found == 1){
			break
		}

		// calculate new start index and range bounds
		temp = start
        	for (i = 0; i < process_number; i++){
                	temp += bits(temps)
        	}
        	start = temp
		window_size = bits(start)
		end = start + window_size
		current_prime = nextPrime(start)

	}
	t2 = current_time()
	total_time = t2 - t1
	barrier() // make sure all processes are here (in order to clear all communication channels)
	if (process_number == 0){
		Write_To_File(total_time)
		for (other_process = 1; other_process < num_processes; other_process++) {
			other_time = Receive_From(other_process)
			Write_To_file(other_time)
		}
	}
	else{
		Send_To_P0(total_time)
	}
	Finalize_MPI()
}
```

#Make
Run make to build the program. You must have MPI and GMP installed to build the program.
