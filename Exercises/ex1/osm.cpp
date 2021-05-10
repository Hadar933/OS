#include "osm.h"
#include <sys/time.h>
#include <iostream>
#include <math.h>

using namespace std;

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations)
{
	if (iterations == 0)
	{
		return -1;
	}
	struct timeval start, end;
	double x = 0;
	gettimeofday(&start, nullptr);
	for (unsigned int i = 0; i < iterations; i++)
	{ // loop unrolling: (5 operations instead of 1)
		x += 1;
		x += 1;
		x += 1;
		x += 1;
		x += 1;
	}
	gettimeofday(&end, nullptr);
	unsigned int start_nano = start.tv_sec * 1000000000 + start.tv_usec * 1000;
	unsigned int end_nano = end.tv_sec * 1000000000 + end.tv_usec * 1000;
	return end_nano - start_nano;
}


void empty_function()
{

}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations)
{
	if (iterations == 0)
	{
		return -1;
	}
	struct timeval start, end;
	gettimeofday(&start, nullptr);
	for (unsigned int i = 0; i < iterations; i++)
	{
		empty_function();
		empty_function();
		empty_function();
		empty_function();
		empty_function();
	}
	gettimeofday(&end, nullptr);
	unsigned int start_nano = start.tv_sec * 1000000000 + start.tv_usec * 1000;
	unsigned int end_nano = end.tv_sec * 1000000000 + end.tv_usec * 1000;
	return end_nano - start_nano;
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations)
{

	if (iterations == 0)
	{
		return -1;
	}
	struct timeval start, end;

	gettimeofday(&start, nullptr);
	for (unsigned int i = 0; i < iterations; i++)
	{

		// loop unrolling:
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
		OSM_NULLSYSCALL;
	}
	gettimeofday(&end, nullptr);
	unsigned int start_nano = start.tv_sec * 1000000000 + start.tv_usec * 1000;
	unsigned int end_nano = end.tv_sec * 1000000000 + end.tv_usec * 1000;
	return end_nano - start_nano;
}



//////////////////////////////////////////////////////////////////////////


const int DIR1_PIN = 4;
const int DIR2_PIN = 5;
const int PWM_PIN = 6;
const int LED_PIN = 9;

const int numSamples = 200;
const float pi = 3.1415;
float f = 1; // signal frequency
float T = 1 / f;
float dt = T / numSamples;
int sineSignal[numSamples];
int dir1Array[numSamples];
int dir2Array[numSamples];

// the setup function runs once when you press reset or power the board
const int OUTPUT = 1;
const int HIGH = 1;
const int LOW = 0;

/*
 * 1. array contains non zero elements (plaster with if)
 * 2. led oscilates as sine wave, regardless of the DIR pins
 * 3. motor amplitude is very small (perhaps because the motor is broken)
 */
void setup()
{


//	pinMode(LED_PIN, OUTPUT);
//	pinMode(DIR1_PIN, OUTPUT);
//	pinMode(DIR2_PIN, OUTPUT);
//	pinMode(PWM_PIN, OUTPUT);
//  digitalWrite(DIR1_PIN, HIGH);
//  digitalWrite(DIR2_PIN, LOW);


	for (int n = 0; n < numSamples; n++)
	{
		sineSignal[n] = abs((int) (255 * (sin(2 * pi * f * n * dt))));
		if (n < numSamples / 2)
		{
			dir1Array[n] = HIGH;
			dir2Array[n] = LOW;
		}
		else
		{
			dir1Array[n] = LOW;
			dir2Array[n] = HIGH;
		}
	}
}

void loop()
{
	cout << "\nsignal\n" << endl;
	for (int n = 0; n < numSamples; n++)
	{
		cout<<"("<<n<<","<<sineSignal[n]<<"),";
	}
	cout << "\ndir1\n" << endl;
	for (int n = 0; n < numSamples; n++)
	{
		cout<<"("<<n<<","<<dir1Array[n]<<"),";
	}
	cout << "\ndir2\n" << endl;
	for (int n = 0; n < numSamples; n++)
	{
		cout<<"("<<n<<","<<dir2Array[n]<<"),";
	}
//		analogWrite(LED_PIN, sineSignal[i]);
//		analogWrite(PWM_PIN, sineSignal[i]);
//		digitalWrite(DIR1_PIN, dir1Array[i]);
//		digitalWrite(DIR2_PIN, dir2Array[i]);
//		delay(100);
//	}
//	delay(1000);
}

int main(){
	setup();
	loop();
}