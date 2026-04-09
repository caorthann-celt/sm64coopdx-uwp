#pragma once

#include <direct.h>
#include <io.h>

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

#define access _access
#define chdir _chdir
#define getcwd _getcwd
