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

#include "MTINode.h"

std::map<std::string, std::string> MTINode::texture_cache;

std::string MTINode::findTexture(const std::string& mti_name) {
	if (texture_cache.find(mti_name) != texture_cache.end()) {
		return texture_cache[mti_name];
	}
	
	Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
	for (const auto& group : groups) {
		Ogre::StringVectorPtr resources = Ogre::ResourceGroupManager::getSingleton().findResourceNames(group, "*.dds");
		
		if (!resources.isNull()) {
			for (const auto& resource : *resources) {
				if (resource.find(mti_name) != std::string::npos) {
					texture_cache[mti_name] = resource;
					return resource;
				}
			}
		}
	}
	
	texture_cache[mti_name] = "";
	return "";
}

MTINode::MTINode(LibGens::InstanceBrushNode* node, const std::string& mti_name, Ogre::SceneManager* scene_manager) : EditorNode() {
	brush_node = node;
	node_index = node->getIndex();
	type = EDITOR_NODE_TERRAIN;
	
	LibGens::Vector3 pos = node->getPosition();
	position = Ogre::Vector3(pos.x, pos.y, pos.z);
	
	scene_node = scene_manager->getRootSceneNode()->createChildSceneNode();
	scene_node->setPosition(position);
	scene_node->getUserObjectBindings().setUserAny(EDITOR_NODE_BINDING, Ogre::Any(this));
	
	manual_object = scene_manager->createManualObject();
	setMaterial(mti_name);
	createBillboardGeometry(mti_name);
	scene_node->attachObject(manual_object);
}

MTINode::~MTINode() {
	if (manual_object) {
		if (scene_node) {
			scene_node->detachObject(manual_object);
		}
		manual_object->_getManager()->destroyManualObject(manual_object);
		manual_object = NULL;
	}
}

void MTINode::createBillboardGeometry(const std::string& mti_name) {
	if (!manual_object) return;
	
	manual_object->clear();
	
	std::string material_name = "MTI_" + mti_name;
	manual_object->begin(material_name, Ogre::RenderOperation::OT_TRIANGLE_LIST);
	
	float size = 2.0f;
	float half_size = size * 0.5f;
	
	LibGens::Color node_color = brush_node->getColor();
	Ogre::ColourValue color(node_color.r, node_color.g, node_color.b, node_color.a);
	
	manual_object->position(-half_size, 0, 0);
	manual_object->textureCoord(0, 1);
	manual_object->colour(color);
	
	manual_object->position(half_size, 0, 0);
	manual_object->textureCoord(1, 1);
	manual_object->colour(color);
	
	manual_object->position(half_size, size, 0);
	manual_object->textureCoord(1, 0);
	manual_object->colour(color);
	
	manual_object->position(-half_size, size, 0);
	manual_object->textureCoord(0, 0);
	manual_object->colour(color);
	
	manual_object->triangle(0, 1, 2);
	manual_object->triangle(0, 2, 3);
	
	manual_object->end();
	manual_object->convertToMesh("MTI_" + mti_name + "_" + std::to_string((size_t)brush_node));
	manual_object->setBoundingBox(Ogre::AxisAlignedBox(-half_size, 0, -half_size, half_size, size, half_size));
}

void MTINode::setMaterial(const std::string& texture_name) {
	if (!manual_object) return;
	
	std::string material_name = "MTI_" + texture_name;
	
	if (Ogre::MaterialManager::getSingleton().resourceExists(material_name)) {
		material = Ogre::MaterialManager::getSingleton().getByName(material_name);
	} else {
		material = Ogre::MaterialManager::getSingleton().create(material_name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
		pass->setLightingEnabled(false);
		pass->setVertexColourTracking(Ogre::TVC_DIFFUSE);
		pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		pass->setDepthWriteEnabled(false);
		pass->setAlphaRejectSettings(Ogre::CMPF_GREATER, 128);
		pass->setCullingMode(Ogre::CULL_NONE);
		
		std::string tex_name = findTexture(texture_name);
		if (!tex_name.empty()) {
			Ogre::TextureUnitState* tex_unit = pass->createTextureUnitState(tex_name);
			tex_unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
		} else {
			pass->setDiffuse(1.0f, 0.0f, 1.0f, 1.0f);
		}
	}
}
