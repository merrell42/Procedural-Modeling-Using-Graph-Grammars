#pragma once

// We have different min and max functions for the command line version and
// the DLL version to fix some compiling issues.
int min(int a, int b);
float min(float a, float b);
double min(double a, double b);

int max(int a, int b);
float max(float a, float b);
double max(double a, double b);
