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

#include "EditorNode.h"
#include "InstanceMTI.h"

#ifndef MTI_NODE_H_INCLUDED
#define MTI_NODE_H_INCLUDED

class MTINode : public EditorNode {
	protected:
		Ogre::ManualObject* manual_object;
		Ogre::MaterialPtr material;
		LibGens::InstanceBrushNode* brush_node;
		unsigned char node_index;
		static std::map<std::string, std::string> texture_cache;
	public:
		MTINode(LibGens::InstanceBrushNode* node, const std::string& mti_name, Ogre::SceneManager* scene_manager);
		~MTINode();
		
		void createBillboardGeometry(const std::string& mti_name);
		void setMaterial(const std::string& texture_name);
		static std::string findTexture(const std::string& mti_name);
		
		unsigned char getNodeIndex() const { return node_index; }
};

#endif
