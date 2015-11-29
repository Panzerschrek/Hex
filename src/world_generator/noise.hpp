#pragma once

// Returns noise value in range [0; 65536).
int g_Noise2( int x, int y, int seed );

// Takes interpolated value
// on noise grid with size = 1 << shift.
// returns value in range [0; 65536).
int g_InterpolatedNoise( int x, int y, int seed, int shift );
int g_TriangularInterpolatedNoise( int x, int y, int seed, int shift );

// Returns value in range [0; 65536 + 65536/2 + 65536/4 + ... + 65536/(2^(octaves - 1)) ).
int g_OctaveNoise( int x, int y, int seed, int octaves );
int g_TriangularOctaveNoise( int x, int y, int seed, int octaves );
