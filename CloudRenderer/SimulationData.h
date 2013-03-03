// Stores the simulation data, in particular a grid representing various cloud
// data

#ifndef SIMULATIONDATA_H
#define SIMULATIONDATA_H

#include "Metaball.h"

#include <vector>

typedef std::vector<Metaball *> metaVector;

class SimulationData {
	friend class SimulatorModule;
public:
	SimulationData( int, int, int, float ); 
	~SimulationData();
		
	metaVector metaballs;

	float *** nextDen; // Continous density distribution (protected with mutex)
	GLfloat *** prevDen; // Previous continous density distribution

	double nextTime; // Time when nextDen was computed
	double prevTime; // Time when prevDen was computed

	inline int getGridLength() { return x; }
	inline int getGridWidth() { return y; }
	inline int getGridHeight() { return z; }
private:

	const int x; // The length of the grid
	const int y; // The width of the grid
	const int z; // The height of the grid
	
	bool *** hum; // Humidity
	bool *** cld; // Clouds
	bool *** act; // Phase transition/activation
	bool *** fAc;

	float *** workDen; // Private continous density distribution

	const float metaR; // The radius of Metaballs
	float metaA; // The distance between two metaball centers
	float metaV; // The the horizontal distance between two metaball centers

	// Creates a dynamic 3d boolean array of size x*y*z
	template <typename T>
	void make3Darray( T ***, int, int, int);
	
	// Frees the 3d boolean array memory
	template <typename T>
	void delete3Darray( T ***, int, int, int);
};

#endif