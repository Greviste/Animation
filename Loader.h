#ifndef LOADER_H
#define LOADER_H

#include <filesystem>
#include "Model.h"


using LoadedData = std::tuple<ModelData, Skeleton>;
LoadedData decodeFbx(const std::filesystem::path& file);

#endif
