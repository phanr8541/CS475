#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <omp.h>

#define CSV

// Global Variables
unsigned int seed = 0;

// Changable states
int	NowYear;		// 2025 - 2030
int	NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	NowNumDeer;		// number of deer in the current population
int NowNumWolves; // wolf agent

// Constants 
const float GRAIN_GROWS_PER_MONTH = 12.0;
const float ONE_DEER_EATS_PER_MONTH = 1.0;

const float AVG_PRECIP_PER_MONTH = 7.0;	// average
const float AMP_PRECIP_PER_MONTH = 6.0;	// plus or minus
const float RANDOM_PRECIP = 2.0;	// plus or minus noise

const float AVG_TEMP = 60.0;	// average
const float AMP_TEMP = 20.0;	// plus or minus
const float RANDOM_TEMP = 10.0;	// plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

// InitBarrier(n) global variables
omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

// Function prototypes
void InitBarrier(int);
void WaitBarrier();

// InitBarrier function code
// specify how many threads will be in the barrier:
//	(also init's the Lock)

// Ranf function copied from proj01
float Ranf(float low, float high) {
	float r = (float)rand();
	float t = r / (float)RAND_MAX;

	return low + t * (high - low);
}


float
SQR(float x)
{
	return x * x;
}

void setMonth() {
	float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);	// angle of earth around the sun

	float temp = AVG_TEMP - AMP_TEMP * cos(ang);
	NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
	NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
	if (NowPrecip < 0.)
		NowPrecip = 0.;
}


void
InitBarrier(int n)
{
	NumInThreadTeam = n;
	NumAtBarrier = 0;
	omp_init_lock(&Lock);
}


// WaitBarrier( ) function code
void WaitBarrier()
{
	omp_set_lock(&Lock);
	{
		NumAtBarrier++;
		if (NumAtBarrier == NumInThreadTeam)
		{
			NumGone = 0;
			NumAtBarrier = 0;
			// let all other threads get back to what they were doing
// before this one unlocks, knowing that they might immediately
// call WaitBarrier( ) again:
			while (NumGone != NumInThreadTeam - 1);
			omp_unset_lock(&Lock);
			return;
		}
	}
	omp_unset_lock(&Lock);

	while (NumAtBarrier != 0);	// this waits for the nth thread to arrive

#pragma omp atomic
	NumGone++;			// this flags how many threads have returned
}



// Watcher function to increase the month
void Watcher()
{

	while (NowYear < 2031) {
		// compute a temporary next-value for this quantity
		  // based on the current state of the simulation:
		  //. . .

		  // DoneComputing barrier:
		// Do Nothing
		WaitBarrier();

		// DoneAssigning barrier:
	  // Do Nothing
		WaitBarrier();
		//. . .

	  // Write out the "Now" State of Data
		NowMonth++;
		if (NowMonth % 12 == 0) {
			NowYear++;
		}

		// Advance time and re-compute all environmental variables
		setMonth();

		// DonePrinting barrier:
		WaitBarrier();
		//. . .
#ifdef CSV
		fprintf(stderr, "%2.2f, %2.2f, %2.2f, %2d, %2d\n",
			NowTemp, NowPrecip, NowHeight, NowNumDeer, NowNumWolves);
#else
		fprintf(stderr, "temp: %2.2f; precip: %2.2f ; height: %2.2f ; deer: %2d; wolves: %2d \n",
			NowTemp, NowPrecip, NowHeight, NowNumDeer, NowNumWolves);
#endif
	}

}

void Deer()
{
	while (NowYear < 2031) {
		// compute a temporary next-value for this quantity
		  // based on the current state of the simulation:
		  // function of what all states are right Now
		int nextNumDeer = NowNumDeer;
		int carryingCapacity = (int)(NowHeight);

		// DoneComputing barrier:
		WaitBarrier();
		//. . .
	  // Copy the computed next state to the Now State
		if (nextNumDeer < carryingCapacity)
			nextNumDeer++;
		else
			if (nextNumDeer > carryingCapacity)
				nextNumDeer--;

		if (nextNumDeer < 0)
			nextNumDeer = 0;

		NowNumDeer = nextNumDeer;

		// DoneAssigning barrier:
		WaitBarrier();
		//. . .

		// DonePrinting barrier:
	  // Do Nothing
		WaitBarrier();
		//. . .

	}
}

void Grain()
{
	while (NowYear < 2031) {
		// compute a temporary next-value for this quantity
		  // based on the current state of the simulation:
		  //. . .
		float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
		float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));
		float nextHeight = NowHeight;

		// DoneComputing barrier:
		WaitBarrier();
		//. . .
		nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
		nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		if (nextHeight < 0.)
			nextHeight = 0.;
		NowHeight = nextHeight;
		// DoneAssigning barrier:
		WaitBarrier();
		//. . .

		// DonePrinting barrier:
		WaitBarrier();
		//. . .

	}
}

void Wolf() {
	while (NowYear < 2031) {
		int nextNumWolves = NowNumWolves;
		int wolfCapacity = NowNumDeer / 2;

		// DoneComputing barrier:
		WaitBarrier();

		if (nextNumWolves < wolfCapacity) {
			nextNumWolves++;
		}
		else if (nextNumWolves > wolfCapacity) {
			nextNumWolves--;
		}
		if (nextNumWolves < 0)
			nextNumWolves = 0;

		NowNumWolves = nextNumWolves;

		// DoneAssigning barrier:
		WaitBarrier();

		// DonePrinting barrier:
		WaitBarrier();
	}
}


int main(int argc, char* argv[]) {
#ifdef _OPENMP
#ifndef CSV
	fprintf(stderr, "OpenMP is supported -- version = %d\n", _OPENMP);
#endif
#else
	fprintf(stderr, "No OpenMP support!\n");
	return 1;
#endif

	// starting date and time:
	NowMonth = 0;
	NowYear = 2025;

	// starting state (feel free to change this if you want):
	NowNumDeer = 2;
	NowHeight = 5.;
	NowNumWolves = 1;


	omp_set_num_threads(4);	// or 4
	InitBarrier(4);		// or 4
#pragma omp parallel sections
	{
#pragma omp section
		{
			Deer();
		}

#pragma omp section
		{
			Grain();
		}

#pragma omp section
		{
			Watcher();

		}

#pragma omp section
		{
			Wolf();
		}
	}       // implied barrier -- all functions must return in order
	  // to allow any of them to get past here


}