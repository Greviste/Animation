#ifndef LOADER_H
#define LOADER_H

#include <filesystem>
#include "Model.h"


ModelData decodeFbx(const std::filesystem::path& file);

#endif
