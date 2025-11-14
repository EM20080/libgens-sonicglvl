//=========================================================================
//	  Copyright (c) 2016 SonicGLvl
//
//    This file is part of SonicGLvl, a community-created free level editor 
//    for the PC version of Sonic Generations.
//
//    SonicGLvl is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    SonicGLvl is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    
//
//    Read AUTHORS.txt, LICENSE.txt and COPYRIGHT.txt for more details.
//=========================================================================

#include "EmbeddedConfig.h"
#include <fstream>

namespace EmbeddedConfig {
	const char* getPluginsConfig() {
		return 
			"# Defines plugins to load\n"
			"\n"
			"# Define plugin folder\n"
			"PluginFolder=.\n"
			"\n"
			"# Define plugins\n"
			" Plugin=RenderSystem_Direct3D9\n"
			" Plugin=Plugin_OctreeSceneManager\n";
	}

	const char* getResourcesConfig() {
		return
			"[General]\n"
			"FileSystem=../database/resources/\n"
			"FileSystem=../database/resources/debug/\n"
			"FileSystem=../database/shaders/\n"
			"\n"
			"[Preview]\n"
			"FileSystem=../database/preview_resources/\n";
	}

	bool createConfigFile(const std::string& filename, const char* content) {
		std::ofstream file(filename);
		if (file.is_open()) {
			file << content;
			file.close();
			return true;
		}
		return false;
	}

	void initializeConfigs() {
		createConfigFile("plugins.cfg", getPluginsConfig());
		createConfigFile("resources.cfg", getResourcesConfig());
	}
}
