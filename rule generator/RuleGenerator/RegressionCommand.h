#pragma once

// Compare the grammars RuleGenerator writes against the ones recorded in
// `regression/` before one-sided edges stopped throwing from getTwin.
int runRegression(const char* exeArg);
