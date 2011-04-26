
#ifndef _PARTICLES_KERNEL_H_
	#define _PARTICLES_KERNEL_H_

	#include <stdio.h>
	#include <math.h>
	#include "cutil_math.h"
	#include "math_constants.h"
	
	// Insert particles in grid
	
	__global__ void insertParticles ( char* pntData, uint pntStride )
	{
		int index = __mul24(blockIdx.x,blockDim.x) + threadIdx.x;
		float4 p = *(float4*) (pntData + index*pntStride);

		// get address in grid
		int3 gridPos = calcGridPos(p);

		addParticleToCell(gridPos, index, gridCounters, gridCells);
	}


#endif
