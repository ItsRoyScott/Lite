#pragma once

#include "Assimp/Importer.hpp"
#include "Assimp/mesh.h"
#include "Assimp/postprocess.h"
#include "Assimp/scene.h"

#ifdef _DEBUG
#pragma comment(lib, "Assimp/assimpd.lib")
#pragma comment(lib, "Assimp/zlibstaticd.lib")
#else
#pragma comment(lib, "Assimp/assimp.lib")
#pragma comment(lib, "Assimp/zlibstatic.lib")
#endif