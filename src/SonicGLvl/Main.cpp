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

#include "EditorApplication.h"
#include <SDL.h>
#include <SDL_main.h>
#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

EditorApplication *editor_application;
 
int main(int argc, char *argv[])
{
	// Allocate console for debug output
	AllocConsole();
	
	// Redirect stdout and stderr to the console
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	

	SetConsoleTitleA("SonicGLvl");
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Failed", SDL_GetError(), NULL);
		return 1;
	}

	EditorApplication editor_app;
	editor_application = &editor_app;

	try {
		editor_app.go();
	} catch( Ogre::Exception& e ) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "An exception has occurred!", e.getFullDescription().c_str(), NULL);
	}

	SDL_Quit();
	return 0;
}