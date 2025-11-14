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


void EditorApplication::updateNodeVisibility() {
	bool visible=true;

	// EDITOR_NODE_OBJECT
	visible=configuration->checkVisibilityFlag(EDITOR_NODE_OBJECT);
	if (visible) object_node_manager->showAll();
	else object_node_manager->hideAll();

	// EDITOR_NODE_TERRAIN
	visible=configuration->checkVisibilityFlag(EDITOR_NODE_TERRAIN);
	for (list<TerrainNode *>::iterator it=terrain_nodes_list.begin(); it!=terrain_nodes_list.end(); it++) {
		if (visible) (*it)->show();
		else (*it)->hide();
	}

	// EDITOR_NODE_HAVOK
	visible=configuration->checkVisibilityFlag(EDITOR_NODE_HAVOK);
	for (list<HavokNode *>::iterator it=havok_nodes_list.begin(); it!=havok_nodes_list.end(); it++) {
		if (visible) (*it)->show();
		else (*it)->hide();
	}

	// FIXME: EDITOR_NODE_PATH

	// FIXME: EDITOR_NODE_GHOST
}


void EditorApplication::toggleNodeVisibility(unsigned int flag) {
	configuration->toggleVisibilityFlag(flag);
	updateVisibilityGUI();
	updateNodeVisibility();
}


void EditorApplication::updateVisibilityGUI() {

}