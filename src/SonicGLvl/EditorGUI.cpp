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
#include <Windows.h>
#include <commdlg.h>


void EditorApplication::copySelection() {
	// Create and populate XML
	TiXmlDocument doc;

	TiXmlElement *root=new TiXmlElement(LIBGENS_OBJECT_SET_ROOT);

		// Retrieve Object pointers from Object Nodes
		for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
			if ((*it)->getType() == EDITOR_NODE_OBJECT) {
				ObjectNode *object_node = static_cast<ObjectNode *>(*it);
				LibGens::Object *object = object_node->getObject();

				if (object) {
					object->writeXML(root);
				}
			}
		}

		doc.LinkEndChild(root);

		// Set up XML Printer
		TiXmlPrinter printer;
		printer.SetIndent("  ");
		doc.Accept(&printer);

		SDL_SetClipboardText(printer.CStr());
}

void EditorApplication::pasteSelection() {
	if(SDL_HasClipboardText()) {
		char *buffer = SDL_GetClipboardText();

		TiXmlDocument doc;
		doc.Parse((const char*) buffer, 0, TIXML_ENCODING_UTF8);

		TiXmlHandle hDoc(&doc);
		TiXmlElement* pElem;
		TiXmlHandle hRoot(0);

		pElem=hDoc.FirstChildElement().Element();
		if (!pElem) {
			return;
		}

		// Check if a root node named "SetObject" exists, and skip to its child root if it does
		if (pElem->ValueStr() == LIBGENS_OBJECT_SET_ROOT) {
			pElem=pElem->FirstChildElement();
		}
		
		list<LibGens::Object *> paste_objects;
		for(pElem; pElem; pElem=pElem->NextSiblingElement()) {
			if (pElem->ValueStr() != LIBGENS_OBJECT_SET_LAYER_DEFINE) {
				LibGens::Object *obj=new LibGens::Object(pElem->ValueStr());

				obj->readXML(pElem);
				obj->learnFromLibrary(library);

				paste_objects.push_back(obj);
			}
		}

		overrideObjectsPalettePreview(paste_objects);
		SDL_free(buffer);
	}
}

void EditorApplication::openLevelGUI() {
	if (current_level != NULL) {
		ERROR_MSG("You have already opened a level!");
		return;
	}

	HWND hwnd = NULL;
	window->getCustomAttribute("WINDOW", &hwnd);
	
	OPENFILENAMEA ofn;
	char szFile[260] = {0};
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "#Level.ar.00\0*.ar.00;\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	
	if (GetOpenFileNameA(&ofn) == TRUE) {
		openLevel(string(ofn.lpstrFile));
	}
}


void EditorApplication::openLostWorldLevelGUI() {
	// TODO: Implement native folder dialog with SDL2
	LOG_MSG("Open Lost World level dialog - to be implemented with ImGui file browser");
}

void EditorApplication::exportSceneFBXGUI() {
	HWND hwnd = NULL;
	window->getCustomAttribute("WINDOW", &hwnd);
	
	OPENFILENAMEA ofn;
	char szFile[260] = {0};
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "FBX Files\0*.fbx\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = "fbx";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	
	if (GetSaveFileNameA(&ofn) == TRUE) {
		exportSceneFBX(string(ofn.lpstrFile));
	}
}

void EditorApplication::importLevelTerrainFBXGUI() {
	HWND hwnd = NULL;
	window->getCustomAttribute("WINDOW", &hwnd);
	
	OPENFILENAMEA ofn;
	char szFile[260] = {0};
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "FBX Files\0*.fbx\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	
	if (GetOpenFileNameA(&ofn) == TRUE) {
		importLevelTerrainFBX(string(ofn.lpstrFile));
	}
}

void EditorApplication::loadAllTerrain() {
	if (terrain_streamer) {
		terrain_streamer->forceLoad();
	}
}

void EditorApplication::saveLevelDataGUI() {
	if (current_level) saveLevelData(current_level_filename);
}

void EditorApplication::saveLevelResourcesGUI() {
	if (current_level) saveLevelResources();
}

void EditorApplication::saveLevelTerrainGUI() {
	if (terrain_streamer) {
		if (CONFIRM_MSG("To do any terrain operations you have to wait for the terrain streamer to finish first. Do you want load all the terrain?") == IDYES) {
			terrain_streamer->forceLoad();
		}
		return;
	}

	if (current_level) saveLevelTerrain();
}