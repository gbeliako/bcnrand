#ifndef _BCN_KERNEL_H_
#define _BCN_KERNEL_H_

#include "stdio.h"
#include "bcnrand.h"

/*	
 * Main Program, illustrates the usage of bcnrand
 
 * Call from the command line:  ./bcnrand 268435456 1 512 112 1
 * first argument is the length of sequence, then repeats (for accurate timing), block size, block count, seed (a number > 0)
 * 	
 * This function gives two examples for usage of BCN_RAND
 * The first example is for timing the alternative methods proposed in the paper [1]
 * 
 *
 *
 * The second example shows how to use BCN_RAND inline rather than writing to memory with a simple case of counting
 * the randomly generated real values under 0.9.
 *
 * The last example is how to use and time the combined generator, same process as in the second example
 *
 *
 *	This program is freeware. 
 *
 *	Please cite our work
	[1] Beliakov, G., Creighton, D., Johnstone, M. and Wilkin, T. 2013, Efficient implementation of Bailey and Borwein 
	pseudo-random number generator based on normal numbers, 
	Computer physics communications, vol. 184, no. 8, pp. 1999-2004.

	[2] G. Beliakov, M. Johnstone, D. Creighton, T. Wilkin, 2012, An efficient implementation of 
	Bailey and Borwein's algorithm for parallel random number generation on graphics processing
	units, Computing 94(5): 433-447.

	http://arxiv.org/abs/1206.1187, http://www.deakin.edu.au/~gleb/bcn_random.html

	J. Borwein and D. Bailey's work is available from:
	http://crd.lbl.gov/~dhbailey/dhbpapers/normal-pseudo.pdf
 *
 *	Copyright Gleb Beliakov, Tim Wilkin and Michael Johnstone, 2012
 */

int main(int argc, char **argv)
{
	if ( argc != 6 ) 
	{
		printf("Usage ./bcnrand <Number Elements> <iterations> <block size> <block count> <Seed> \nNow Exiting\n");
		exit(0);
	}
	unsigned int numElements = atoi(argv[1]);
	unsigned int numIterations = atoi(argv[2]);
	unsigned int numThreadsPerBlock = atoi(argv[3]);
	unsigned int numBlocks = atoi(argv[4]);
	double seed = atof(argv[5]);

	unsigned int workPerThread = numElements/numBlocks/numThreadsPerBlock;
	
	while(numBlocks*numThreadsPerBlock*workPerThread < numElements)
		++workPerThread;
	
	//calc even number of elements
	numElements = numBlocks*numThreadsPerBlock*workPerThread;
	
	printf("elemens, repeats, blokcsize, numblocks, seed: %d, %d, %d, %d, %f\n", numElements,numIterations,numThreadsPerBlock,numBlocks,seed);
	

	TimeBCNMethod(numElements, seed, numThreadsPerBlock, numBlocks, numIterations, workPerThread);
	
// Example 2: count the number of generated elements under 0.9 in parallel
	
	InlineGeneration(numElements, seed, numThreadsPerBlock, numBlocks,  workPerThread);

// Example 3: count the number of generated elements under 0.9 in parallel, using combined generator
	
	GenerationCombined(numElements, seed, numThreadsPerBlock, numBlocks,  workPerThread);	
}

#endif // #ifndef _BCN_KERNEL_H_
