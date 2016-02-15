#include <stdio.h>			/* for printf */
#include <gmp.h>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

	// declare mpz structs
	mpz_t n; 
	mpz_t zero;
	mpz_t r;
	mpz_t d;
	mpz_t start;
	mpz_t end;
	mpz_t temp;
	mpz_t my_rank_mpz;
	mpz_t local_end;
	mpz_t current_prime;

	mpz_t global_end;

	mpz_init_set_ui(zero, 0);
	mpz_init(d);

	double t1, t2, totaltime;

	int tag = 0;
	int my_rank, p;
	int source, dest, i;

	mpz_init_set_str(n, argv[1], 10); // initialize the key (n)

	mpz_init(global_end);
	mpz_sqrt(global_end, n);

	// MPI boilerplate
	MPI_Status status;
	MPI_Request request;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	t1 = MPI_Wtime();

	// initialize mpz structs
	mpz_init(r);
	mpz_init(start);
	mpz_init(local_end);
	mpz_init(end);
	mpz_init(temp);
	mpz_init(current_prime);
	mpz_init(my_rank_mpz);
	mpz_init_set_ui(start, 2);
	mpz_set(temp, start);

	size_t temp_window_size = mpz_sizeinbase(temp, 2);

	mpz_add_ui(temp, temp, temp_window_size);

	for (i = 0; i < my_rank; i++){
		temp_window_size = mpz_sizeinbase(temp, 2);
		mpz_add_ui(temp, temp, temp_window_size);	
	}

	mpz_set(start, temp);
	size_t window_size =  mpz_sizeinbase(start, 2);
	mpz_add_ui(local_end, start, window_size);	


	mpz_init_set_ui(my_rank_mpz, my_rank);
	mpz_init_set(end, global_end);
	mpz_nextprime(current_prime, start);
	mpz_init_set_str(n, argv[1], 10);

	int check = 0;
	int flag = 0;
	int found = 0;

	MPI_Irecv(&found, 1, MPI_INTEGER, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &request);
	MPI_Barrier(MPI_COMM_WORLD);

	while (mpz_cmp(start, end) <= 0){
		while(mpz_cmp(current_prime, local_end) <= 0 && mpz_cmp(current_prime, end) <= 0){
			mpz_mod(r, n, current_prime);
			if (mpz_cmp(zero, r) == 0){
				mpz_divexact(d, n, current_prime);
				gmp_printf("%Zd %Zd\n", d, current_prime);
				found = 1;
				for (dest = 0; dest < p; dest++){
					MPI_Isend(&found, 1, MPI_INTEGER, dest, tag, MPI_COMM_WORLD, &request);
				}
				break;		
			}
			check++;
			if (check == 500){
				check = 0;
				MPI_Test(&request, &flag, &status);
				if (flag){
					found = 1;
					break;
				}
			}
			mpz_nextprime(current_prime, current_prime);	
		}
		if( found == 1){
			break;
		}

		// calculate new start index

		mpz_set(temp, start);
		size_t temp_window_size = mpz_sizeinbase(start, 2);
		mpz_add_ui(temp, temp, temp_window_size);
		mpz_set(start, local_end);
		for (i = my_rank + 1; i < p; i++){
			temp_window_size = mpz_sizeinbase(temp, 2);
			mpz_add_ui(temp, temp, temp_window_size);
		}

		for (i = 0; i < my_rank; i++){
			temp_window_size = mpz_sizeinbase(temp, 2);
			mpz_add_ui(temp, temp, temp_window_size);
		}

		mpz_set(start, temp);

		temp_window_size = mpz_sizeinbase(start, 2);
		mpz_add_ui(local_end, start, temp_window_size);	

		mpz_nextprime(current_prime, start);
	}

	t2 = MPI_Wtime();
	totaltime = t2 - t1;
	MPI_Barrier(MPI_COMM_WORLD);
	if (my_rank == 0){
		char buf[0x1000];
		snprintf(buf, sizeof(buf), "time_%s", argv[1]);
		FILE *f = fopen(buf, "w");
		double other_totaltime = 0;
		fprintf(f, "0\t%1.2e\n", totaltime);
		for (source = 1; source < p; source++){
			MPI_Recv(&other_totaltime, 1, MPI_DOUBLE, source, tag, MPI_COMM_WORLD, &status);
			fprintf(f, "%d\t%1.2e\n", source, other_totaltime);
		}	
		fclose(f); 
	}
	else{
		MPI_Send(&totaltime, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);
	}	
	MPI_Finalize();
	return 0;
}
