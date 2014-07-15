#pragma once

// AssImp headers
#include "Assimp/Importer.hpp"
#include "Assimp/mesh.h"
#include "Assimp/postprocess.h"
#include "Assimp/scene.h"

// AssImp libraries (depends on zlib)
#ifdef _DEBUG
#pragma comment(lib, "Assimp/assimpd.lib")
#pragma comment(lib, "Assimp/zlibstaticd.lib")
#else
#pragma comment(lib, "Assimp/assimp.lib")
#pragma comment(lib, "Assimp/zlibstatic.lib")
#endif