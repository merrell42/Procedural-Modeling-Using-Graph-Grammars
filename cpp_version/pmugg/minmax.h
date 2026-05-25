#pragma once

// Console (pmugg): use these wrappers so std::min/max are safe without Windows.h.
// DLL (pmugg dll): use bare min()/max() — Windows.h macros apply via pch.
int min(int a, int b);
float min(float a, float b);
double min(double a, double b);

int max(int a, int b);
float max(float a, float b);
double max(double a, double b);
