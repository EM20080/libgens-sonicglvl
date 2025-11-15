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
#include "EditorNodeHistory.h"
#include "ObjectNodeHistory.h"
#include "ObjectSet.h"
#include "ObjectLibrary.h"
#include "MessageTypes.h"
#include "Material.h"
#include "Texture.h"
#include "Parameter.h"

#ifdef _WIN32
#include <d3d9.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#endif

// Forward declare Ogre D3D9 classes to get the device
namespace Ogre {
	class D3D9RenderSystem {
	public:
		static IDirect3DDevice9* getActiveD3D9Device();
	};
}

Ogre::Rectangle2D* mMiniScreen=NULL;

void Game_ProcessMessage(PipeClient* client, PipeMessage* msg);

EditorApplication::EditorApplication(void)
{
	show_left_panel = true;
	show_bottom_panel = true;
	show_properties_editor = false;
	show_material_editor = false;
	show_physics_editor = false;
	show_find_dialog = false;
	show_look_at_dialog = false;
	show_multiset_dialog = false;
	show_terrain_info = false;
	show_quick_overview = false;

	cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	cursor_hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	cursor_sizeall = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	cursor_cross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);

	bottom_pos_x = bottom_pos_y = bottom_pos_z = 0.0f;
	bottom_rot_x = bottom_rot_y = bottom_rot_z = 0.0f;
	multiset_vec_x = multiset_vec_y = multiset_vec_z = 0.0f;
	multiset_spacing = 0.0f;
	multiset_count = 0;

	game_client = new PipeClient();
	game_client->AddMessageProcessor(Game_ProcessMessage);
	ghost_data = nullptr;
	isGhostRecording = false;
	checked_shader_library = false;
	current_level = NULL;
}

EditorApplication::~EditorApplication(void) {
	delete game_client;
}

ObjectNodeManager* EditorApplication::getObjectNodeManager()
{
	return object_node_manager;
}

void EditorApplication::selectNode(EditorNode* node)
{
	if (node)
	{
		node->setSelect(true);
		selected_nodes.push_back(node);
		viewport->focusOnPoint(node->getPosition());
	}
}

void EditorApplication::updateSelection() {
	Ogre::Vector3 center=Ogre::Vector3::ZERO;
	Ogre::Quaternion rotation=Ogre::Quaternion::IDENTITY;

	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		center += (*it)->getPosition();
	}
	center /= selected_nodes.size();

	if ((selected_nodes.size() == 1) && !world_transform) {
		rotation = (*selected_nodes.begin())->getRotation();
	}

	axis->setVisible(selected_nodes.size() > 0);
	axis->setRotationFrozen((selected_nodes.size() > 1) || world_transform);
	axis->setPosition(center);
	axis->setRotation(rotation);
	axis->resetState();

	updateNodeVisibility();

	if (editor_mode != EDITOR_NODE_QUERY_VECTOR) {
		updateObjectsPropertiesGUI();
	}

	updateBottomSelectionGUI();
}

void EditorApplication::deleteSelection() {
	if (!selected_nodes.size()) return;

	bool msp_deleted = false;

	if (editor_mode == EDITOR_NODE_QUERY_OBJECT) {
		HistoryActionWrapper *wrapper = new HistoryActionWrapper();

		for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
			// Cast to appropiate types depending on the type of editor node
			if ((*it)->getType() == EDITOR_NODE_OBJECT) {
				ObjectNode *object_node=static_cast<ObjectNode *>(*it);

				LibGens::Object *object=object_node->getObject();
				if (object) {
					LibGens::ObjectSet *object_set=object->getParentSet();
					if (object_set) {
						object_set->eraseObject(object);
					}

					object_node_manager->hideObjectNode(object, true);
					HistoryActionDeleteObjectNode *action = new HistoryActionDeleteObjectNode(object, object_node_manager);
					wrapper->push(action);

					HistoryActionSelectNode *action_select = new HistoryActionSelectNode((*it), true, false, &selected_nodes);
					(*it)->setSelect(false);
					wrapper->push(action_select);
				}
			}
			else if ((*it)->getType() == EDITOR_NODE_OBJECT_MSP) {
				ObjectMultiSetNode* object_msp_node = static_cast<ObjectMultiSetNode*>(*it);
				HistoryActionSelectNode* action_select = new HistoryActionSelectNode((*it), true, false, &selected_nodes);
				object_msp_node->setSelect(false);
				wrapper->push(action_select);
			}
		}

		removeAllTrajectoryNodes();
		selected_nodes.clear();
		axis->setVisible(false);

		pushHistory(wrapper);
	}
}

void EditorApplication::clearSelection() {
	if (!selected_nodes.size()) return;

	bool stuff_deselected = false;
	HistoryActionWrapper *wrapper = new HistoryActionWrapper();

	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		if ((*it)->isSelected()) {
			stuff_deselected = true;

			HistoryActionSelectNode *action_select = new HistoryActionSelectNode((*it), true, false, &selected_nodes);
			(*it)->setSelect(false);
			wrapper->push(action_select);
		}
	}

	if (stuff_deselected) {
		pushHistory(wrapper);
	}
	else {
		delete wrapper;
	}

	removeAllTrajectoryNodes();
	selected_nodes.clear();
	axis->setVisible(false);
}

void EditorApplication::showSelectionNames() {
	string message = "";
	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		if ((*it)->isSelected()) {
			if ((*it)->getType() == EDITOR_NODE_TERRAIN) {
				TerrainNode *terrain_node = (TerrainNode *) (*it);
				message += terrain_node->getTerrainInstance()->getName() + "\n";
			}
		}
	}

	SHOW_MSG(message.c_str());
}

void EditorApplication::selectAll() {
	bool stuff_selected = false;
	HistoryActionWrapper *wrapper = new HistoryActionWrapper();

	if (editor_mode == EDITOR_NODE_QUERY_OBJECT) {
		list<ObjectNode *> object_nodes = object_node_manager->getObjectNodes();
		for (list<ObjectNode *>::iterator it=object_nodes.begin(); it!=object_nodes.end(); it++) {
			if (!(*it)->isSelected() && !(*it)->isForceHidden()) {
				stuff_selected = true;
				HistoryActionSelectNode *action_select = new HistoryActionSelectNode((*it), false, true, &selected_nodes);
				(*it)->setSelect(true);
				wrapper->push(action_select);
				selected_nodes.push_back(*it);
			}
		}
	}

	if (stuff_selected) {
		pushHistory(wrapper);
	}
	else {
		delete wrapper;
	}

	updateSelection();
}

void EditorApplication::rememberCloningNodes()
{
	for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); ++it)
	{
		if ((*it)->getType() == EDITOR_NODE_OBJECT)
			cloning_nodes.push_back(*it);
	}
}

list<EditorNode*> EditorApplication::getSelectedNodes()
{
	return selected_nodes;
}

void EditorApplication::cloneSelection() {
	if (!selected_nodes.size()) return;

	list<EditorNode *> nodes_to_clone = selected_nodes;
	clearSelection();
	
	HistoryActionWrapper *wrapper = new HistoryActionWrapper();
	for (list<EditorNode *>::iterator it=nodes_to_clone.begin(); it!=nodes_to_clone.end(); it++) {
		// Cast to appropiate types depending on the type of editor node
		if ((*it)->getType() == EDITOR_NODE_OBJECT) {
			ObjectNode *object_node=static_cast<ObjectNode *>(*it);

			LibGens::Object *object=object_node->getObject();
			if (object) {
				LibGens::Object *new_object = new LibGens::Object(object);

				if (current_level) {
					if (current_level->getLevel()) {
						new_object->setID(current_level->getLevel()->newObjectID());
					}
				}
			
				LibGens::ObjectSet *parent_set = object->getParentSet();
				if (parent_set) {
					parent_set->addObject(new_object);
				}

				// Create
				ObjectNode *new_object_node = object_node_manager->createObjectNode(new_object);

				// Push to History
				HistoryActionCreateObjectNode *action = new HistoryActionCreateObjectNode(new_object, object_node_manager);
				wrapper->push(action);

				// Add to current selection
				HistoryActionSelectNode *action_select = new HistoryActionSelectNode(new_object_node, false, true, &selected_nodes);
				new_object_node->setSelect(true);
				selected_nodes.push_back(new_object_node);
				wrapper->push(action_select);
			}
		}
	}
	pushHistory(wrapper);

	updateSelection();
}

void EditorApplication::temporaryCloneSelection() {
	if (!selected_nodes.size()) return;

	list<EditorNode*> nodes_to_clone = selected_nodes;
	clearSelection();

	for (list<EditorNode*>::iterator it = nodes_to_clone.begin(); it != nodes_to_clone.end(); it++) {
		// Cast to appropiate types depending on the type of editor node
		if ((*it)->getType() == EDITOR_NODE_OBJECT) {
			ObjectNode* object_node = static_cast<ObjectNode*>(*it);

			LibGens::Object* object = object_node->getObject();
			if (object) {
				LibGens::Object* new_object = new LibGens::Object(object);

				if (current_level) {
					if (current_level->getLevel()) {
						new_object->setID(current_level->getLevel()->newObjectID());
					}
				}

				LibGens::ObjectSet* parent_set = object->getParentSet();
				if (parent_set) {
					parent_set->addObject(new_object);
				}

				// Create
				ObjectNode* new_object_node = object_node_manager->createObjectNode(new_object);

				// Add to current selection
				new_object_node->setSelect(true);
				selected_nodes.push_back(new_object_node);
			}
		}
	}

	updateSelection();
}

void EditorApplication::translateSelection(Ogre::Vector3 v) {
	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		(*it)->translate(v);
	}
}


void EditorApplication::rotateSelection(Ogre::Quaternion q) {
	if (selected_nodes.size() == 1 || local_rotation) {
		for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); ++it) {
			(*it)->rotate(q);
		}
		//node->setRotation(q);
	}
	else {
		Ogre::Matrix4 matrix;
		matrix.makeTransform(axis->getPosition(), Ogre::Vector3::UNIT_SCALE, q);

		for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
			Ogre::Vector3 new_pos = matrix * ((*it)->getPosition() - axis->getPosition());
			(*it)->setPosition(new_pos);
			//(*it)->rotate(q);
		}
	}
}


void EditorApplication::setSelectionRotation(Ogre::Quaternion q) {
	if (selected_nodes.size() == 1) {
		EditorNode *node = *selected_nodes.begin();
		node->setRotation(q);
	}
}


void EditorApplication::rememberSelection(bool mode) {
	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		if (!mode) (*it)->rememberPosition();
		else {
			if (selected_nodes.size() > 1) (*it)->rememberPosition();
			(*it)->rememberRotation();
		}
	}
}

void EditorApplication::makeHistorySelection(bool mode) {
	HistoryActionWrapper *wrapper = new HistoryActionWrapper();
	int index = 0;
	bool is_list = current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_VECTOR_LIST;
	for (list<EditorNode *>::iterator it=selected_nodes.begin(); it!=selected_nodes.end(); it++) {
		if (!mode) {
			HistoryActionMoveNode *action = new HistoryActionMoveNode((*it), (*it)->getLastPosition(), (*it)->getPosition());
			wrapper->push(action);
			if (editor_mode == EDITOR_NODE_QUERY_VECTOR) {
				VectorNode* vector_node = static_cast<VectorNode*>(*it);
				if (!show_look_at_dialog)
				{
					while (property_vector_nodes[index] != vector_node)
						++index;

					updateEditPropertyVectorGUI(index, is_list);
				}
				else
				{
					updateLookAtVectorGUI();
				}
			}
		}
		else {
			// Push only a rotation history if it's only one node. 
			// If it's more, push a Rotation/Move wrapper
			if (selected_nodes.size() == 1) {
				HistoryActionRotateNode *action = new HistoryActionRotateNode((*it), (*it)->getLastRotation(), (*it)->getRotation());
				wrapper->push(action);
			}
			else {
				HistoryActionWrapper *sub_wrapper = new HistoryActionWrapper();

				HistoryActionMoveNode *action_mov   = new HistoryActionMoveNode((*it), (*it)->getLastPosition(), (*it)->getPosition());
				sub_wrapper->push(action_mov);

				HistoryActionRotateNode *action_rot = new HistoryActionRotateNode((*it), (*it)->getLastRotation(), (*it)->getRotation());
				sub_wrapper->push(action_rot);

				wrapper->push(sub_wrapper);
			}
		}
		index = 0;
	}

	if (is_list && editor_mode == EDITOR_NODE_QUERY_VECTOR)
	{
		updateEditPropertyVectorList(temp_property_vector_list);
		if (show_properties_editor && isVectorListSelectionValid())
		{
			Ogre::Vector3 v = property_vector_nodes[current_vector_list_selection]->getPosition();
			edit_vector_x = v.x;
			edit_vector_y = v.y;
			edit_vector_z = v.z;
		}
	}
	pushHistory(wrapper);
}


void EditorApplication::undoHistory() {
	if (editor_mode == EDITOR_NODE_QUERY_VECTOR) {
		if (show_look_at_dialog)
		{
			look_at_vector_history->undo();
			updateLookAtVectorGUI();
		}
		else
		{
			property_vector_history->undo();
			bool is_list = current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_VECTOR_LIST;

			for (int index = 0; index < property_vector_nodes.size(); ++index)
				updateEditPropertyVectorGUI(index, is_list);
			if (is_list)
			{
				updateEditPropertyVectorList(temp_property_vector_list);
				if (show_properties_editor && isVectorListSelectionValid())
				{
					Ogre::Vector3 v = property_vector_nodes[current_vector_list_selection]->getPosition();
					edit_vector_x = v.x;
					edit_vector_y = v.y;
					edit_vector_z = v.z;
				}
			}
		}
	}
	else history->undo();
}


void EditorApplication::redoHistory() {
	if (editor_mode == EDITOR_NODE_QUERY_VECTOR) {
		if (show_look_at_dialog)
		{
			look_at_vector_history->redo();
			updateLookAtVectorGUI();
		}
		else
		{
			property_vector_history->redo();
			bool is_list = current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_VECTOR_LIST;

			for (int index = 0; index < property_vector_nodes.size(); ++index)
				updateEditPropertyVectorGUI(index, is_list);
			if (is_list)
			{
				updateEditPropertyVectorList(temp_property_vector_list);
				if (show_properties_editor && isVectorListSelectionValid())
				{
					Ogre::Vector3 v = property_vector_nodes[current_vector_list_selection]->getPosition();
					edit_vector_x = v.x;
					edit_vector_y = v.y;
					edit_vector_z = v.z;
				}
			}
		}
	}
	else history->redo();
}


void EditorApplication::pushHistory(HistoryAction *action) {
	if (editor_mode == EDITOR_NODE_QUERY_VECTOR)
	{
		property_vector_history->push(action);
		look_at_vector_history->push(action);
	}
	else history->push(action);
}


void EditorApplication::toggleWorldTransform() {
	world_transform = !world_transform;
	// ImGui menu handles checkmark state automatically
}


void EditorApplication::togglePlacementSnap() {
	if (placement_grid_snap > 0.0f) {
		placement_grid_snap = 0.0f;
	}
	else {
		placement_grid_snap = 0.5f;
	}
	// ImGui menu handles checkmark state automatically
}

void EditorApplication::toggleLocalRotation() {
	local_rotation = !local_rotation;
	// ImGui menu handles checkmark state automatically
}

void EditorApplication::toggleRotationSnap() {
	axis->setRotationSnap(!axis->isRotationSnap());
	// ImGui menu handles checkmark state automatically
}

void EditorApplication::snapToClosestPath() {
	if (!current_level || selected_nodes.empty()) {
		return;
	}

	vector<float> closest_distances(selected_nodes.size(), FLT_MAX);
	vector<LibGens::Vector3> closest_positions(selected_nodes.size());

	for (LibGens::Path *path : current_level->getLevel()->getPaths()) {
		LibGens::PathNodeList path_node_list = path->getNodes();

		for (auto& pair : path_node_list) {
			size_t editor_node_index = 0;

			for (EditorNode *editor_node : selected_nodes) {
				if ((editor_node->getType() == EDITOR_NODE_OBJECT) || (editor_node->getType() == EDITOR_NODE_OBJECT_MSP)) {
					Ogre::Vector3 position = editor_node->getPosition();
					float closest_distance = FLT_MAX;
					LibGens::Vector3 closest_position = pair.first->findClosestPoint(pair.second, LibGens::Vector3(position.x, position.y, position.z), &closest_distance);

					if (closest_distance < closest_distances[editor_node_index]) {
						closest_distances[editor_node_index] = closest_distance;
						closest_positions[editor_node_index] = closest_position;
					}
				}

				++editor_node_index;
			}
		}
	}

	HistoryActionWrapper *wrapper = new HistoryActionWrapper();
	size_t editor_node_index = 0;

	for (EditorNode *editor_node : selected_nodes) {
		if (closest_distances[editor_node_index] != FLT_MAX) {
			LibGens::Vector3 closest_position = closest_positions[editor_node_index];

			Ogre::Vector3 previous_position = editor_node->getPosition();
			Ogre::Vector3 new_position(closest_position.x, closest_position.y, closest_position.z);

			editor_node->setPosition(new_position);

			HistoryActionMoveNode *action_move = new HistoryActionMoveNode(editor_node, previous_position, new_position);
			wrapper->push(action_move);
		}

		++editor_node_index;
	}

	pushHistory(wrapper);
	updateSelection();
}

void EditorApplication::createScene(void) {
	// Initialize LibGens Managers
	havok_enviroment = new LibGens::HavokEnviroment(100 * 1024 * 1024);
	fbx_manager      = new LibGens::FBXManager();

	havok_property_database    = new LibGens::HavokPropertyDatabase(SONICGLVL_HAVOK_PROPERTY_DATABASE_PATH);
	history                    = new History();
	property_vector_history    = new History();
	look_at_vector_history     = new History();
	level_database             = new EditorLevelDatabase(SONICGLVL_LEVEL_DATABASE_PATH);
	material_library           = new LibGens::MaterialLibrary(SONICGLVL_RESOURCES_PATH);
	model_library              = new LibGens::ModelLibrary(SONICGLVL_RESOURCES_PATH);
	generations_shader_library = new LibGens::ShaderLibrary(SONICGLVL_SHADERS_PATH);
	unleashed_shader_library   = new LibGens::ShaderLibrary(SONICGLVL_SHADERS_PATH);
	uv_animation_library       = new LibGens::UVAnimationLibrary(SONICGLVL_RESOURCES_PATH);
	generations_library        = new LibGens::ObjectLibrary(SONICGLVL_LIBRARY_PATH);
	unleashed_library          = new LibGens::ObjectLibrary(SONICGLVL_LIBRARY_PATH);
	library                    = generations_library;
	animations_list            = new EditorAnimationsList();

	bool loaded_generations_shader_library = 
		generations_shader_library->loadShaderArchive("shader_r.ar.00") &&
		generations_shader_library->loadShaderArchive("shader_r_add.ar.00");

	if (!loaded_generations_shader_library) {
		delete generations_shader_library;
		generations_shader_library = NULL;
	}

	bool loaded_unleashed_shader_library = 
		unleashed_shader_library->loadShaderArchive("shader.ar") &&
		unleashed_shader_library->loadShaderArchive("shader_d3d9.ar");

	if (!loaded_unleashed_shader_library) {
		delete unleashed_shader_library;
		unleashed_shader_library = NULL;
	}

	generations_library->loadDatabase(SONICGLVL_GENERATIONS_OBJECTS_DATABASE_PATH);
	unleashed_library->loadDatabase(SONICGLVL_UNLEASHED_OBJECTS_DATABASE_PATH);

	configuration    = new EditorConfiguration();
	configuration->load(SONICGLVL_CONFIGURATION_FILE);

	object_production = new LibGens::ObjectProduction();
	object_production->load(configuration->getObjectProductionPath());

	havok_enviroment->addFolder(SONICGLVL_RESOURCES_PATH);
	
	updateVisibilityGUI();
	updateObjectCategoriesGUI();
	updateObjectsPaletteGUI();
	createObjectsPropertiesGUI();

	current_category_index     = 0;
	palette_cloning_mode       = false;
	ignore_mouse_clicks_frames = 0;
	last_palette_selection     = NULL;
	current_palette_selection  = NULL;
	current_set                = NULL;
	current_single_property_object = NULL;
	history_edit_property_wrapper = NULL;
	cloning_mode = SONICGLVL_MULTISETPARAM_MODE_CLONE;
	is_pick_target = false;
	is_pick_target_position = false;
	is_update_look_at_vector = 

	// Set up Scene Managers
	scene_manager = root->createSceneManager("OctreeSceneManager");
	axis_scene_manager = root->createSceneManager(Ogre::ST_GENERIC);
	
	// Set up Node Managers
	object_node_manager = new ObjectNodeManager(scene_manager, model_library, material_library, object_production);

	scene_manager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
	axis_scene_manager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

	viewport = new EditorViewport(scene_manager, axis_scene_manager, window, SONICGLVL_CAMERA_NAME);
	axis = new EditorAxis(axis_scene_manager);

	color_listener = new ColorListener(scene_manager);
	depth_listener = new DepthListener(scene_manager);

	/*
	Ogre::TexturePtr rtt_texture = Ogre::TextureManager::getSingleton().createManual("ColorTex", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, window->getWidth()/2.0, window->getHeight()/2.0, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
	color_texture = rtt_texture->getBuffer()->getRenderTarget();
	color_texture->addViewport(viewport->getCamera());
	color_texture->getViewport(0)->setClearEveryFrame(true);
	color_texture->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
	color_texture->getViewport(0)->setOverlaysEnabled(false);
	color_texture->addListener(color_listener);

	rtt_texture = Ogre::TextureManager::getSingleton().createManual("DepthTex", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, window->getWidth()/2.0, window->getHeight()/2.0, 0, Ogre::PF_FLOAT32_R, Ogre::TU_RENDERTARGET);
	depth_texture = rtt_texture->getBuffer()->getRenderTarget();
	Ogre::Viewport *depthViewport=depth_texture->addViewport(viewport->getCamera());
	depth_texture->getViewport(0)->setClearEveryFrame(true);
	depth_texture->getViewport(0)->setBackgroundColour(Ogre::ColourValue::White);
	depth_texture->getViewport(0)->setOverlaysEnabled(false);
	depth_texture->addListener(depth_listener);
	*/

	global_illumination_listener = new GlobalIlluminationListener();
	//global_illumination_listener->setPassToIgnore(depth_listener->getDepthPass());
	scene_manager->addRenderObjectListener(global_illumination_listener); 

	current_node = NULL;
	editor_mode = EDITOR_NODE_QUERY_OBJECT;
	world_transform = false;
	
	terrain_streamer = NULL;
	terrain_update_counter = 0;
	current_level = NULL;
	ghost_node = NULL;
	camera_manager = NULL;
	global_directional_light = NULL;
	placement_grid_snap = 0.0f;
	dragging_mode = 0;

	farPlaneChange = 0.0f;

	// Initialize Material Editor Variables
	material_editor_mode = SONICGLVL_MATERIAL_EDITOR_MODE_MODEL;
	material_editor_material_library = NULL;
}

void EditorApplication::windowResized(Ogre::RenderWindow* rw) {
	BaseApplication::windowResized(rw);

	// Calculate viewport dimensions based on ImGui panel visibility
	float left = 0.0f;
	float top = 0.0f;
	float width = 1.0f;
	float height = 1.0f;
	
	// Account for main menu bar (approximately 20 pixels)
	float menu_bar_height = 20.0f;
	top = menu_bar_height / (float)screen_height;
	height -= top;
	
	// Account for left panel if visible
	if (show_left_panel) {
		left = (float)SONICGLVL_GUI_LEFT_WIDTH / (float)screen_width;
		width -= left;
	}
	
	// Account for bottom panel if visible
	if (show_bottom_panel) {
		float bottom_height = (float)SONICGLVL_GUI_BOTTOM_HEIGHT / (float)screen_height;
		height -= bottom_height;
	}

	viewport->resize(left, top, width, height);
}


bool EditorApplication::keyPressed(const OIS::KeyEvent &arg) {
	ImGuiIO& io = ImGui::GetIO();
	
	if (axis->isHolding()) return true;
	
	if (!io.WantCaptureKeyboard) {
		viewport->keyPressed(arg);
	}

	bool regular_mode = isRegularMode();

	if(arg.key == OIS::KC_NUMPAD4) {
		farPlaneChange = -1.0f;
	}

	if(arg.key == OIS::KC_NUMPAD6) {
		farPlaneChange = 1.0f;
	}

	if(arg.key == OIS::KC_NUMPAD8) {
		farPlaneChange = 10.0f;
	}

	if(arg.key == OIS::KC_NUMPAD2) {
		farPlaneChange = -10.0f;
	}

	// Quit Special Placement Modes
	if(arg.key == OIS::KC_ESCAPE) {
		if (isPalettePreviewActive()) {
			clearObjectsPalettePreviewGUI();
		}

		if (editor_mode == EDITOR_NODE_QUERY_VECTOR) {
			closeVectorQueryMode();
		}

		if (editor_mode == EDITOR_NODE_QUERY_NODE) {
			closeVectorQueryMode();
		}

		if (is_pick_target)
		{
			openQueryTargetMode(false);
		}

		if (is_pick_target_position)
		{
			queryLookAtObject(false);
		}
	}

	// Regular Mode Shorcuts
	if (regular_mode && inFocus() && !viewport->isMoving()) {
		if(arg.key == OIS::KC_DELETE) {
			deleteSelection();
			updateSelection();
		}

		if (keyboard->isModifierDown(OIS::Keyboard::Ctrl)) {
			if(arg.key == OIS::KC_C) {
				copySelection();
			}

			if(arg.key == OIS::KC_V) {
				clearSelection();
				pasteSelection();
			}

			if(arg.key == OIS::KC_P) {
				if (ghost_node) ghost_node->setPlay(true);
			}

			if(arg.key == OIS::KC_R) {
				if (ghost_node) {
					ghost_node->setPlay(false);
					ghost_node->setTime(0);
				}
			}

			if(arg.key == OIS::KC_F) {
				if (!show_find_dialog)
					openFindGUI();
			}

			if(arg.key == OIS::KC_D) {
				clearSelection();
				updateSelection();
			}

			if(arg.key == OIS::KC_Z) {
				undoHistory();
				updateSelection();
			}

			if(arg.key == OIS::KC_Y) {
				redoHistory();
				updateSelection();
			}

			if(arg.key == OIS::KC_E) {
				toggleWorldTransform();
				updateSelection();
			}

			if(arg.key == OIS::KC_I) {
				//SHOW_MSG(ToString(farPlane).c_str());
			}

			if(arg.key == OIS::KC_A) {
				//saveXNAnimation();
				selectAll();
			}

			if(arg.key == OIS::KC_T) {
				clearSelection();
				editor_mode = (editor_mode == EDITOR_NODE_QUERY_TERRAIN ? EDITOR_NODE_QUERY_OBJECT : EDITOR_NODE_QUERY_TERRAIN);
			}

			if(arg.key == OIS::KC_I) {
				showSelectionNames();
			}

			if(arg.key == OIS::KC_G) {
				setupGhost();
				clearSelection();
				editor_mode = (editor_mode == EDITOR_NODE_QUERY_GHOST ? EDITOR_NODE_QUERY_OBJECT : EDITOR_NODE_QUERY_GHOST);
			}

			if (arg.key == OIS::KC_O) {
				editor_application->openLevelGUI();
			}

			if (arg.key == OIS::KC_S)
			{
				editor_application->saveLevelDataGUI();
			}
			if (arg.key == OIS::KC_R)
			{
				if (editor_mode == EDITOR_NODE_OBJECT || EDITOR_NODE_QUERY_GHOST)
				{
					toggleRotationSnap();
					updateSelection();
				}
			}
		}
		else if (keyboard->isModifierDown(OIS::Keyboard::Alt))
		{
			if (arg.key == OIS::KC_F) {
				if (camera_manager) {
					camera_manager->setForceCamera(!camera_manager->getForceCamera());
				}
			}

			if (arg.key == OIS::KC_G) 
			{
				if (editor_mode == EDITOR_NODE_QUERY_GHOST)
				{
					ghost_node->setPosition(Ogre::Vector3(viewport->getCamera()->getPosition() + viewport->getCamera()->getDirection() * 10));
					updateSelection();
				}
			}
		}
		else {
			if (arg.key == OIS::KC_T) {
				axis->setMode(false);
			}
			if (arg.key == OIS::KC_R) {
				axis->setMode(true);
			}
		}
	}

	// Global Mode Shortcuts
	if (keyboard->isModifierDown(OIS::Keyboard::Ctrl)) {
		if(arg.key == OIS::KC_1) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_OBJECT);
			editor_application->toggleNodeVisibility(EDITOR_NODE_OBJECT_MSP);
		}

		if(arg.key == OIS::KC_2) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_TERRAIN);
		}

		if(arg.key == OIS::KC_3) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_TERRAIN_AUTODRAW);
		}

		if(arg.key == OIS::KC_4) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_HAVOK);
		}

		if(arg.key == OIS::KC_5) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_PATH);
		}

		if(arg.key == OIS::KC_6) {
			editor_application->toggleNodeVisibility(EDITOR_NODE_GHOST);
		}
		if (arg.key == OIS::KC_O)
		{
			editor_application->openLevelGUI();
		}
	}

    return true;
}


bool EditorApplication::keyReleased(const OIS::KeyEvent &arg) {
	viewport->keyReleased(arg);

	if(arg.key == OIS::KC_NUMPAD4) {
		farPlaneChange = 0;
	}

	if(arg.key == OIS::KC_NUMPAD6) {
		farPlaneChange = 0;
	}

	if(arg.key == OIS::KC_NUMPAD8) {
		farPlaneChange = 0;
	}

	if(arg.key == OIS::KC_NUMPAD2) {
		farPlaneChange = 0;
	}

    return true;
}


bool EditorApplication::mouseMoved(const OIS::MouseEvent &arg) {
	ImGuiIO& io = ImGui::GetIO();
	
	viewport->setQueryFlags(editor_mode);

	if (!axis->isHolding() && !io.WantCaptureMouse) {
		viewport->mouseMoved(arg);
	}

	if (editor_mode == EDITOR_NODE_QUERY_NODE) {
		if (viewport->isMouseInLocalScreen(arg)) {
			global_cursor_state = 3;
		}
		else {
			global_cursor_state = 0;
		}
	}
	else if (!isPalettePreviewActive()) {
		Ogre::Entity *entity=viewport->getCurrentEntity();
		if (entity) {
			Ogre::SceneNode *node=entity->getParentSceneNode();
			if (node) {
				Ogre::Any ptr_container=node->getUserObjectBindings().getUserAny(EDITOR_NODE_BINDING);
				if (!ptr_container.isEmpty()) {
					EditorNode *editor_node=Ogre::any_cast<EditorNode *>(ptr_container);
					if (current_node && (editor_node != current_node)) current_node->setHighlight(false);
					current_node = editor_node;
				}
			}
			else {
				if (current_node) current_node->setHighlight(false);
				current_node = NULL;
			}
		}
		else {
			if (current_node) current_node->setHighlight(false);
			current_node = NULL;
		}

		if (current_node) current_node->setHighlight(true);
	
		if (axis->mouseMoved(viewport, arg)) {
			if (!axis->getMode())
				translateSelection(axis->getTranslate());
			else
				rotateSelection(axis->getRotate());

			updateBottomSelectionGUI();
		}

		if (axis->isHighlighted()) {
			global_cursor_state = 2;
		}
		else if (current_node) {
			global_cursor_state = 1;
		}
		else {
			global_cursor_state = 0;
		}
	}
	else {
		mouseMovedObjectsPalettePreview(arg);

		if (viewport->isMouseInLocalScreen(arg)) {
			global_cursor_state = 2;
		}
		else {
			global_cursor_state = 0;
		}
	}

    return true;
}

bool EditorApplication::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id) {
	viewport->mousePressed(arg, id);
	
	if (viewport->isMouseInLocalScreen(arg)) {
		focus();

		if (ignore_mouse_clicks_frames) {
			return true;
		}

		if (isRegularMode()) {
			if (axis->mousePressed(viewport, arg, id)) {
				dragging_mode = 0;

				if (keyboard->isModifierDown(OIS::Keyboard::Shift) && !show_multiset_dialog) {
					dragging_mode = 1;
					rememberCloningNodes();
					temporaryCloneSelection();
				}

				if (keyboard->isModifierDown(OIS::Keyboard::Ctrl)) {
					dragging_mode = 2;
					cloneSelection();
				}

				rememberSelection(axis->getMode());
			}
			else if (id == OIS::MB_Left) {
				if (current_node) {
					if (!is_pick_target && !is_pick_target_position) {
						if (!keyboard->isModifierDown(OIS::Keyboard::Ctrl)) {
							clearSelection();
						}

						bool was_already_selected = current_node->isSelected();
						if (!was_already_selected)
						{
							HistoryActionSelectNode* action_select = new HistoryActionSelectNode(current_node, false, true, &selected_nodes);
							current_node->setSelect(true);
							selected_nodes.push_back(current_node);
							addTrajectory(getTrajectoryMode(current_node));
							pushHistory(action_select);
						}

						updateSelection();
						
						// Always reset axis state when clicking an object, even if already selected
						if (was_already_selected) {
							axis->resetState();
						}
					}
					else
					{
						if (current_node->getType() == EDITOR_NODE_OBJECT)
						{
							
							ObjectNode* object_node = static_cast<ObjectNode*>(current_node);
							size_t id = object_node->getObject()->getID();

							if (is_pick_target)
							{
								bool is_list = current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_ID_LIST;
								edit_id_value = id;

								setTargetName(id, is_list);
							}

							if (is_pick_target_position)
							{
								is_update_look_at_vector = false;

								Ogre::Vector3 position = object_node->getPosition();
								look_at_x = position.x;
								look_at_y = position.y;
								look_at_z = position.z;

								updateLookAtPointVectorNode(position);
								is_update_look_at_vector = true;
							}
						}
					}
				}
				else if (!current_node) {
					//clearSelection();
				}
			}
		}
		else if (isPalettePreviewActive()) {
			mousePressedObjectsPalettePreview(arg, id);
		}
		else if (editor_mode == EDITOR_NODE_QUERY_NODE) {
			/*
			if (id == OIS::MB_Left) {
				float mouse_x=arg.state.X.abs/float(arg.state.width);
				float mouse_y=arg.state.Y.abs/float(arg.state.height);
				viewport->convertMouseToLocalScreen(mouse_x, mouse_y);

				// Raycast from camera to viewport
				Ogre::uint32 node_query_flags = EDITOR_NODE_QUERY_OBJECT | EDITOR_NODE_QUERY_PATH_NODE | EDITOR_NODE_QUERY_GHOST;
				Ogre::Entity *node_entity = viewport->raycastEntity(mouse_x, mouse_y, node_query_flags);

				if (node_entity) {
					Ogre::SceneNode *node=node_entity->getParentSceneNode();
					if (node) {
						Ogre::Any ptr_container=node->getUserObjectBindings().getUserAny(EDITOR_NODE_BINDING);
						if (!ptr_container.isEmpty()) {
							EditorNode *editor_node=Ogre::any_cast<EditorNode *>(ptr_container);

							Ogre::Vector3 raycast_point = editor_node->getPosition();
							closeVectorQueryMode();
							updateEditPropertyVectorGUI(LibGens::Vector3(raycast_point.x, raycast_point.y, raycast_point.z));
						}
					}
				}
			}

			if (id == OIS::MB_Right) {
				closeVectorQueryMode();
			}
			*/
		}

		viewport->mousePressed(arg, id);
	}
    return true;
}


bool EditorApplication::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id) {
	bool last_holding = axis->mouseReleased(arg, id);

	if (id == OIS::MB_Left) {
		if (last_holding) {
			if (dragging_mode != 1)
				makeHistorySelection(axis->getMode());
		}

		if (dragging_mode == 1)
		{
			if (cloning_nodes.size() && !show_multiset_dialog)
			{
				openMultiSetParamDlg();
				setVectorAndSpacing();

				// save temporary clones to delete them when cloning is done
				list<EditorNode*>::iterator it;
				for (it = selected_nodes.begin(); it != selected_nodes.end(); ++it)
				{
					temporary_nodes.push_back((*it));
					(*it)->setSelect(false);
				}
				selected_nodes.clear();

				for (it = cloning_nodes.begin(); it != cloning_nodes.end(); ++it)
				{
					(*it)->setSelect(true);
					selected_nodes.push_back(*it);
				}

				cloning_nodes.clear();
				updateSelection();
			}
		}
	}

	viewport->mouseReleased(arg, id);
    return true;
}

bool EditorApplication::frameRenderingQueued(const Ogre::FrameEvent& evt) {
    BaseApplication::frameRenderingQueued(evt);

	Ogre::Real timeSinceLastFrame = evt.timeSinceLastFrame;

	if (ignore_mouse_clicks_frames) {
		ignore_mouse_clicks_frames -= 1;
	}

	if (ignore_mouse_clicks_frames < 0) {
		ignore_mouse_clicks_frames = 0;
	}

	// Update Editor
	if (!inFocus()) {
		viewport->onFocusLoss();
	}
	viewport->update(timeSinceLastFrame);
	axis->update(viewport);

	// Update Objects
	updateObjectsPalettePreview();

	// Update Ghost
	checkGhost(timeSinceLastFrame);

	// Update Trajectory previews
	updateTrajectoryNodes(timeSinceLastFrame);

	if (terrain_streamer) {
		Ogre::Vector3 v = viewport->getCamera()->getPosition();
		terrain_streamer->getMutex().lock();
		terrain_streamer->setPosition(LibGens::Vector3(v.x, v.y, v.z));
		terrain_streamer->setCheck(true);
		terrain_streamer->getMutex().unlock();
	}

	// Update Terrain
	checkTerrainStreamer();
	checkTerrainVisibilityAndQuality(timeSinceLastFrame);

	// Update Animations
	object_node_manager->addTime(timeSinceLastFrame);
	animations_list->addTime(timeSinceLastFrame);
    return true;
}

void EditorApplication::loadGhostRecording() 
{
	char filename[MAX_PATH];
	ZeroMemory(filename, sizeof(filename));
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Ghost Recording(.gst.bin)\0*.gst.bin\0";
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Open Ghost Recording";
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING;

	if (!GetOpenFileName(&ofn))
		return;

	chdir(exe_path.c_str());
	LibGens::Ghost* gst = new LibGens::Ghost(std::string(filename));
	setGhost(gst);
}

void EditorApplication::saveGhostRecording()
{
	if (!ghost_data)
		return;

	char filename[MAX_PATH];
	ZeroMemory(filename, sizeof(filename));
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Ghost Recording(.gst.bin)\0*.gst.bin\0";
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Save Ghost Recording";
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING;

	if (!GetSaveFileName(&ofn))
		return;

	chdir(exe_path.c_str());
	ghost_data->save(std::string(filename));
}

void EditorApplication::saveGhostRecordingFbx()
{
	if (!ghost_data)
		return;

	char filename[MAX_PATH];
	ZeroMemory(filename, sizeof(filename));
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "FBX File(.fbx)\0*.fbx\0";
	ofn.nFilterIndex = 1;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Export Ghost Recording";
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING;

	if (!GetSaveFileName(&ofn))
		return;

	chdir(exe_path.c_str());
	LibGens::FBX* lFbx = ghost_data->buildFbx(fbx_manager, model_library->getModel("chr_Sonic_HD"), material_library);
	fbx_manager->exportFBX(lFbx, filename);

	delete lFbx;
}

void EditorApplication::launchGame()
{
	if (GetFileAttributes(configuration->game_path.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		char filename[MAX_PATH];
		ZeroMemory(filename, sizeof(filename));
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = "Windows Executable(.exe)\0*.exe\0";
		ofn.nFilterIndex = 1;
		ofn.nMaxFile = 1024;
		ofn.lpstrTitle = "Select Sonic Generations";
		ofn.lpstrFile = filename;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING;

		if (GetOpenFileName(&ofn))
		{
			chdir(exe_path.c_str());
			configuration->game_path = std::string(ofn.lpstrFile);
		}
	}

	string directory = configuration->game_path.substr(0, configuration->game_path.find_last_of('\\'));
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	CreateProcess(configuration->game_path.c_str(), NULL, NULL, NULL, FALSE, 0, FALSE, directory.c_str(), &si, &pi);
}

bool EditorApplication::connectGame() {
	return game_client->Connect();
}

DWORD EditorApplication::sendMessageGame(const PipeMessage& msg, size_t size) {
	return game_client->UploadMessage(msg, size);
}

void Game_ProcessMessage(PipeClient* client, PipeMessage* msg) {
	editor_application->processGameMessage(client, msg);
}

void EditorApplication::processGameMessage(PipeClient* client, PipeMessage* msg) {
	switch (msg->ID)
	{
	case SONICGLVL_MSG_SETRECORDING:
		isGhostRecording = ((MsgSetRecording*)msg)->Enable;
		break;

	case SONICGLVL_MSG_SAVERECORDING:
		isGhostRecording = false;
		MsgSaveRecording* m = (MsgSaveRecording*)msg;
		LibGens::Ghost* gst = new LibGens::Ghost(std::string(m->FilePath));
		setGhost(gst);
		break;
	}
}

void ColorListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_WORLD_GEOMETRY_2);
	scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_MAX);
}
 
void ColorListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
	scene_manager->clearSpecialCaseRenderQueues();
}

void DepthListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
	queue = scene_manager->getRenderQueue();
    queue->setRenderableListener(this); 

	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_WORLD_GEOMETRY_2);
	scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_MAX);
}
 
void DepthListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
	scene_manager->clearSpecialCaseRenderQueues();

	queue = scene_manager->getRenderQueue();
	queue->setRenderableListener(0); 
}

bool DepthListener::renderableQueued(Ogre::Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique** ppTech, Ogre::RenderQueue* pQueue)
{
	*ppTech = mDepthMaterial->getTechnique(0);
	return true;
};

ObjectNode* EditorApplication::getObjectNodeFromEditorNode(EditorNode* node)
{
	ObjectNode* object_node = nullptr;
	if (node->getType() == EDITOR_NODE_OBJECT)
	{
		object_node = static_cast<ObjectNode*>(node);
	}
	else if (node->getType() == EDITOR_NODE_OBJECT_MSP)
	{
		ObjectMultiSetNode* ms_node = static_cast<ObjectMultiSetNode*>(node);
		object_node = ms_node->getObjectNode();
	}

	return object_node;
}

TrajectoryMode EditorApplication::getTrajectoryMode(EditorNode* node)
{
	std::string object_name;
	ObjectNode* object_node = getObjectNodeFromEditorNode(node);
	if (object_node)
		object_name = object_node->getObject()->getName();

	TrajectoryMode mode = NONE;

	if ((object_name ==  "Spring") || (object_name == "AirSpring") || (object_name == "SpringFake") ||
		(object_name == "SpringClassic") || (object_name == "SpringClassicYellow"))
		mode = SPRING;
	else if (object_name == "WideSpring")
		mode = WIDE_SPRING;
	else if (object_name ==  "JumpPole")
		mode = JUMP_POLE;
	else if ((object_name == "JumpBoard") || (object_name == "JumpBoard3D") || (object_name == "AdlibTrickJump"))
		mode = JUMP_PANEL;
	else if ((object_name == "DashRing") || (object_name == "RainbowRing"))
		mode = DASH_RING;

	return mode;
}

void EditorApplication::addTrajectory(TrajectoryMode mode)
{
	if (mode == NONE)
		return;

	trajectory_preview_nodes.push_back(new TrajectoryNode(scene_manager, mode));
	
	// JumpBoards need two nodes. One for normal, and the other for boost
	if (mode == JUMP_PANEL)
		trajectory_preview_nodes.push_back(new TrajectoryNode(scene_manager, mode));
}

void EditorApplication::updateTrajectoryNodes(Ogre::Real timeSinceLastFrame)
{
	if (!selected_nodes.size())
		return;

	for (int count = 0; count < trajectory_preview_nodes.size(); ++count)
		trajectory_preview_nodes[count]->addTime(timeSinceLastFrame);

	int count = 0;
	list<EditorNode*>::iterator it = selected_nodes.begin();

	for (; it != selected_nodes.end(); ++it)
	{
		if (count < trajectory_preview_nodes.size())
		{
			EditorNode* node = *it;
			TrajectoryMode mode = getTrajectoryMode(node);
			switch (mode)
			{
			case SPRING:
			case WIDE_SPRING:
				trajectory_preview_nodes[count]->getTrajectorySpring(node);
				break;

			case JUMP_PANEL:
				trajectory_preview_nodes[count++]->getTrajectoryJumpBoard(node, false);
				trajectory_preview_nodes[count]->getTrajectoryJumpBoard(node, true);
				break;

			case DASH_RING:
				trajectory_preview_nodes[count]->getTrajectoryDashRing(node);
				break;

			default:
				break;
			}

			++count;
		}
		else
			break;
	}
}

void EditorApplication::removeAllTrajectoryNodes()
{
	for (vector<TrajectoryNode*>::iterator it = trajectory_preview_nodes.begin(); it != trajectory_preview_nodes.end(); ++it)
		delete* it;

	trajectory_preview_nodes.clear();
}

void EditorApplication::convertMaterialsToUnleashed() {
	std::string folderStr = SelectFolderWithIFileDialog(L"Select folder containing materials to convert (Unleashed)");
	if (folderStr.empty()) return;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((folderStr + "\\*.material").c_str(), &findFileData);
	int converted = 0;
	int failed = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string filePath = folderStr + "\\" + findFileData.cFileName;
			LibGens::Material mat(filePath);
			bool success = true;
			if (success) {
				mat.save(folderStr + "\\" + mat.getName() + ".material", LIBGENS_MATERIAL_ROOT_UNLEASHED);
				++converted;
			}
			else {
				++failed;
			}
		} while (FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}
	char msg[128];
	sprintf(msg, "Converted materials\nConverted : %d    Failed : %d", converted, failed);
	SHOW_MSG(msg);
}

void EditorApplication::convertMaterialsToGenerations() {
	std::string folderStr = SelectFolderWithIFileDialog(L"Select folder containing materials to convert (Generations)");
	if (folderStr.empty()) return;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((folderStr + "\\*.material").c_str(), &findFileData);
	int converted = 0;
	int failed = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string filePath = folderStr + "\\" + findFileData.cFileName;
			LibGens::Material mat(filePath);
			bool success = true;
			if (success) {
				mat.save(folderStr + "\\" + mat.getName() + ".material", LIBGENS_MATERIAL_ROOT_GENERATIONS);
				++converted;
			}
			else {
				++failed;
			}
		} while (FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}
	WIN32_FIND_DATA cleanupData;
	HANDLE hTexset = FindFirstFile((folderStr + "\\*.texset").c_str(), &cleanupData);
	if (hTexset != INVALID_HANDLE_VALUE) {
		do { DeleteFileA((folderStr + "\\" + cleanupData.cFileName).c_str()); } while (FindNextFile(hTexset, &cleanupData));
		FindClose(hTexset);
	}
	HANDLE hTexture = FindFirstFile((folderStr + "\\*.texture").c_str(), &cleanupData);
	if (hTexture != INVALID_HANDLE_VALUE) {
		do { DeleteFileA((folderStr + "\\" + cleanupData.cFileName).c_str()); } while (FindNextFile(hTexture, &cleanupData));
		FindClose(hTexture);
	}
	char msg[128];
	sprintf(msg, "Converted materials\nConverted : %d    Failed : %d", converted, failed);
	SHOW_MSG(msg);
}

void EditorApplication::convertMaterialsToLostWorld() {
	std::string folderStr = SelectFolderWithIFileDialog(L"Select folder containing materials to convert (Lost World)");
	if (folderStr.empty()) return;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((folderStr + "\\*.material").c_str(), &findFileData);
	int converted = 0;
	int failed = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string filePath = folderStr + "\\" + findFileData.cFileName;
			LibGens::Material mat(filePath);
			mat.save(folderStr + "\\" + mat.getName() + ".material", LIBGENS_FILE_HEADER_ROOT_TYPE_LOST_WORLD);
			++converted;
		} while (FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}
	WIN32_FIND_DATA cleanupData;
	HANDLE hTexset = FindFirstFile((folderStr + "\\*.texset").c_str(), &cleanupData);
	if (hTexset != INVALID_HANDLE_VALUE) {
		do { DeleteFileA((folderStr + "\\" + cleanupData.cFileName).c_str()); } while (FindNextFile(hTexset, &cleanupData));
		FindClose(hTexset);
	}
	HANDLE hTexture = FindFirstFile((folderStr + "\\*.texture").c_str(), &cleanupData);
	if (hTexture != INVALID_HANDLE_VALUE) {
		do { DeleteFileA((folderStr + "\\" + cleanupData.cFileName).c_str()); } while (FindNextFile(hTexture, &cleanupData));
		FindClose(hTexture);
	}
	char msg[128];
	sprintf(msg, "Converted materials\nConverted : %d    Failed : %d", converted, failed);
	SHOW_MSG(msg);
}

void EditorApplication::convertMaterialsToUnleashedShaders() {
	std::string folderStr = SelectFolderWithIFileDialog(L"Select folder containing materials to convert (Unleashed Shaders)");
	if (folderStr.empty()) return;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((folderStr + "\\*.material").c_str(), &findFileData);
	int converted = 0;
	int failed = 0;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string filePath = folderStr + "\\" + findFileData.cFileName;
			LibGens::Material mat(filePath);
			std::string shader = mat.getShader();   // TODO: Lost world shaders to Unleashed.
			if (shader == "BillboardParticle_da[sv]") shader = "BillboardParticle_da[sv]";
			if (shader == "BillboardParticle_da[v]") shader = "BillboardParticle_da[v]";
			if (shader == "BillboardParticle_d[fv]") shader = "Common_d";
			if (shader == "BillboardParticle_d[lsv]") shader = "BillboardParticle_d[lsv]";
			if (shader == "BillboardParticle_d[lv]") shader = "BillboardParticle_d[lv]";
			if (shader == "BillboardParticle_d[sv]") shader = "BillboardParticle_d[sv]";
			if (shader == "BillboardParticle_d[v]") shader = "BillboardParticle_d[v]";
			if (shader == "BillboardY_d") shader = "Common_d";
			if (shader == "BlbBlend_dd") shader = "Blend_dd";
			if (shader == "BlbCommon_dpne1") shader = "Common_d";
			if (shader == "BlbIndirect_dopn") shader = "Common_d";
			if (shader == "BlbLuminescence_d") shader = "Common_d";
			if (shader == "BlbLuminescence_dpne1E") shader = "Common_d";
			if (shader == "Blend_dbd") shader = "Blend_dd";
			if (shader == "Blend_dd") shader = "Blend_dd";
			if (shader == "Blend_dnbdn") shader = "Blend_dndn";
			if (shader == "Blend_dndn") shader = "Blend_dndn";
			if (shader == "Blend_dndn[w]") shader = "Blend_dndn[w]";
			if (shader == "Blend_dpbdp") shader = "Common_d";
			if (shader == "Blend_dpdp") shader = "Blend_dpdp";
			if (shader == "Blend_dpdpe") shader = "Common_dpe";
			if (shader == "Blend_dpdpe1") shader = "Common_dpe";
			if (shader == "Blend_dpdpe2") shader = "Common_dpe";
			if (shader == "Blend_dpdpn") shader = "Common_dpn";
			if (shader == "Blend_dpnd") shader = "Blend_dpndpn";
			if (shader == "Blend_dpndn") shader = "Blend_dpndn";
			if (shader == "Blend_dpndn[w]") shader = "Blend_dpndn[w]";
			if (shader == "Blend_dpndp") shader = "Blend_dpndpn";
			if (shader == "Blend_dpndpn") shader = "Blend_dpndpn";
			if (shader == "Blend_dsnbdse1") shader = "Common_dsne[b]";
			if (shader == "ChaosV_dsnne1") shader = "Common_dsne[b]";
			if (shader == "Chaos_da") shader = "Common_d[v]";
			if (shader == "Chaos_dsae1") shader = "Common_dsne[b]";
			if (shader == "Chaos_dsbnne1") shader = "Common_dsne[b]";
			if (shader == "Chaos_dsnne1") shader = "Common_dsne[b]";
			if (shader == "ChrEye_dpne") shader = "Common_dpne[b]";
			if (shader == "ChrEye_dpne1") shader = "Common_dpne[b]";
			if (shader == "ChrEye_dpne2") shader = "Common_d[b]";
			if (shader == "ChrEye_dpne[c]") shader = "Common_d[bcv]";
			if (shader == "ChrSkinHalf_dse11") shader = "Ring_dse[b]";
			if (shader == "ChrSkinIgnore_dsle11") shader = "Ring_dse[b]";
			if (shader == "ChrSkin_ds") shader = "Common_ds[b]";
			if (shader == "ChrSkin_dse") shader = "Ring_dse[b]";
			if (shader == "ChrSkin_dse1") shader = "Ring_dse";
			if (shader == "ChrSkin_dse2") shader = "Ring_dse";
			if (shader == "ChrSkin_dsf") shader = "SonicSkin_dspf[b]";
			if (shader == "ChrSkin_dsfe") shader = "SonicSkin_dspf[b]";
			if (shader == "ChrSkin_dsfe1") shader = "SonicSkin_dspf[b]";
			if (shader == "ChrSkin_dsfe2") shader = "SonicSkin_dspf[b]";
			if (shader == "ChrSkin_dsn") shader = "Common_dsne[b]";
			if (shader == "ChrSkin_dsne") shader = "Common_dsne[b]";
			if (shader == "ChrSkin_dsne1") shader = "Common_dsne[b]";
			if (shader == "ChrSkin_dsnf") shader = "SonicSkin_dspf[b]";
			if (shader == "ChrSkin_dsfe") shader = "Common_dsne[b]";
			if (shader == "ChrSkin_dsfe1") shader = "Common_dsne[b]";
			if (shader == "Cloak_do") shader = "Common_d";
			if (shader == "Cloak_doe1") shader = "Common_d";
			if (shader == "Cloth_dsnt") shader = "Cloth_dsnt";
			if (shader == "Cloth_dsnt[b]") shader = "Cloth_dsnt[b]";
			if (shader == "Cloud_nfe1") shader = "Common_d";
			if (shader == "Common_d") shader = "Common_d[v]";
			if (shader == "Common2_dpn") shader = "Common_dpn"; // FORCES shader
			if (shader == "Common2_dp") shader = "Common_dp"; // FORCES shader
			if (shader == "MCommon_dpn") shader = "Common_dpn"; // FORCES shader
			if (shader == "Blend2_dpndpn") shader = "Blend_dpndpn"; // FORCES shader
			if (shader == "IgnoreLight_E") shader = "IgnoreLight_d"; // FORCES shader // This will make emission ignorelights turn into a diffuse, you will have to manually edit the texture slot to be diffuse instead of "emissive"
			if (shader == "Common_da") shader = "Common_d[v]";
			if (shader == "CyberSpaceNoise_dpn") shader = "Common_dpn"; // Common Shader used in frontiers
			if (shader == "Emission_dpe") shader = "Luminescence_dpE"; // Common Shader used in frontiers
			if (shader == "Vegitation") shader = "Common_dpn"; // Common Shader used in frontiers foliage
			if (shader == "Common_dpna") shader = "Common_dpn"; // SXSG Shader
		    if (shader == "Common_de") shader = "Common_de";
			if (shader == "Common_de1") shader = "Common_de";
			if (shader == "Common_de2") shader = "Common_de";
			if (shader == "Common_de[bw]") shader = "Common_de[bw]";
			if (shader == "Common_de[b]") shader = "Common_de[b]";
			if (shader == "Common_de[c]") shader = "Common_de";
			if (shader == "Common_dn") shader = "Common_dn";
			if (shader == "Common_dne") shader = "Common_dne";
			if (shader == "Common_dne1") shader = "Common_dne";
			if (shader == "Common_dne1[v]") shader = "Common_dne";
			if (shader == "Common_dne2") shader = "Common_dne";
			if (shader == "Common_dne2[v]") shader = "Common_dne";
			if (shader == "Common_dne[bcv]") shader = "Common_dne";
			if (shader == "Common_dne[bc]") shader = "Common_dne[bc]";
			if (shader == "Common_dne[b]") shader = "Common_dne[b]";
			if (shader == "Common_dne[cv]") shader = "Common_dne[c]";
			if (shader == "Common_dne[c]") shader = "Common_dne[c]";
			if (shader == "Common_dn[bv]") shader = "Common_dn[bv]";
			if (shader == "Common_dn[bw]") shader = "Common_dn[bw]";
			if (shader == "Common_dn[b]") shader = "Common_dn[b]";
			if (shader == "Common_dp") shader = "Common_dp[v]";
			if (shader == "Common_dpe") shader = "Common_dpe";
			if (shader == "Common_dpe1") shader = "Common_dpe[v]";
			if (shader == "Common_dpe2") shader = "Common_dpe[v]";
			if (shader == "Common_dpe[bc]") shader = "Common_dpe[bc]";
			if (shader == "Common_dpe[b]") shader = "Common_dpe[b]";
			if (shader == "Common_dpe[c]") shader = "Common_dpe[c]";
			if (shader == "Common_dpe[uv]") shader = "Common_dpe[uv]";
			if (shader == "Common_dpe[v]") shader = "Common_dpe[uv]";
			if (shader == "Common_dpe[w]") shader = "Common_dpe[w]";
			if (shader == "Common_dpn") shader = "Common_dpn";
			if (shader == "Common_dpne") shader = "Common_dpne";
			if (shader == "Common_dpne1") shader = "Common_dpne[v]";
			if (shader == "Common_dpne2") shader = "Common_dpne[v]";
			if (shader == "Common_dpne[bcw]") shader = "Common_dpne[bcw]";
			if (shader == "Common_dpne[bc]") shader = "Common_dpne[bc]";
			if (shader == "Common_dpne[bw]") shader = "Common_dpne[bw]";
			if (shader == "Common_dpne[b]") shader = "Common_dpne[b]";
			if (shader == "Common_dpne[c]") shader = "Common_dpne[c]";
			if (shader == "Common_dpne[v]") shader = "Common_dpne[v]";
			if (shader == "Common_dpnne2") shader = "Common_dpne[v]";
			if (shader == "Common_dpnr") shader = "Common_dpnr";
			if (shader == "Common_dpn[bv]") shader = "Common_dpn[bv]";
			if (shader == "Common_dpn[bw]") shader = "Common_dpn[bw]";
			if (shader == "Common_dpn[b]") shader = "Common_dpn[b]";
			if (shader == "Common_dpn[E]") shader = "Common_dpne[v]";
			if (shader == "Common_dpn[uv]") shader = "Common_dpn[uv]";
			if (shader == "Common_dpn[u]") shader = "Common_dpn[u]";
			if (shader == "Common_dpn[v]") shader = "Common_dpn[v]";
			if (shader == "Common_dpn[w]") shader = "Common_dpn[w]";
			if (shader == "Common_dpr" ) shader = "Common_dpr";
			if (shader == "Common_dp[bu]") shader = "Common_dp[bu]";
			if (shader == "Common_dp[bw]") shader = "Common_dp[bw]";
			if (shader == "Common_dp[b]") shader = "Common_dp[b]";
			if (shader == "Common_dp[E]") shader = "Common_dpe";
			if (shader == "Common_dp[uv]") shader = "Common_dp[uv]";
			if (shader == "Common_dp[u]") shader = "Common_dp[u]";
			if (shader == "Common_dp[v]") shader = "Common_dp[v]";
			if (shader == "Common_dp[w]") shader = "Common_dp[w]";
			if (shader == "Common_dsae1") shader = "Common_d";
			if (shader == "Common_dse") shader = "Ring_dse";
			if (shader == "Common_dse1") shader = "Ring_dse";
			if (shader == "Common_dse2") shader = "Ring_dse";
			if (shader == "Common_dse[c]") shader = "Common_d";
			if (shader == "Common_dsn") shader = "Common_dn";
			if (shader == "Common_dsne") shader = "Common_dsne[b]";
			if (shader == "Common_dsne1") shader = "Common_d";
			if (shader == "Common_dsne[b]") shader = "Common_dsne[b]";
			if (shader == "Common_dsnne1") shader = "Common_d";
			if (shader == "Common_dsn[bw]") shader = "Common_dsn[bw]";
			if (shader == "Common_dsn[b]") shader = "Common_dsn[b]";
			if (shader == "Common_dsp") shader = "Ring_ds";
			if (shader == "Common_dspe") shader = "Ring_dse";
			if (shader == "Common_dspn") shader = "Common_dsne[b]";
			if (shader == "Common_dspne") shader = "Common_dsne[b]";
			if (shader == "Common_dspne2") shader = "Common_dsne[b]";
			if (shader == "Common_dspne[c]") shader = "Common_dsne[b]";
			if (shader == "Common_d[bu]") shader = "Common_d[bu]";
			if (shader == "Common_d[bv]") shader = "Common_d[bv]";
			if (shader == "Common_d[bw]") shader = "Common_d[bw]";
			if (shader == "Common_d[b]") shader = "Common_d[b]";
			if (shader == "Common_d[uv]") shader = "Common_d[uv]";
			if (shader == "Common_d[u]") shader = "Common_d[u]";
			if (shader == "Common_d[v]") shader = "Common_d[v]";
			if (shader == "Common_d[w]") shader = "Common_d[w]";
			if (shader == "csd") shader = "csd";
			if (shader == "csdNoTex") shader = "csdNoTex";
			if (shader == "DeformationParticle_d") shader = "DeformationParticle_d";
			if (shader == "Deformation_d") shader = "Common_d";
			if (shader == "DimIgnore_dE") shader = "Luminescence_dE";
			if (shader == "DimIgnore_dpE") shader = "Luminescence_dpeE";
			if (shader == "DimIgnore_e1") shader = "Common_de";
			if (shader == "Dim_dE") shader = "Luminescence_dE";
			if (shader == "Dim_dpE") shader = "Luminescence_dpE";
			if (shader == "Dim_dpnE") shader = "Luminescence_dpnE";
			if (shader == "DistortionOverlayChaos_dn") shader = "Common_dn[v]";
			if (shader == "DistortionOverlayChaos_dnn") shader = "Common_dn[v]";
			if (shader == "DistortionOverlay_dn") shader = "Common_dn[v]";
			if (shader == "DistortionOverlay_dnn") shader = "Common_dn[v]";
			if (shader == "Distortion_dsne1") shader = "Common_dsne[b]";
			if (shader == "Distortion_dsnne1") shader = "Common_dsne[b]";
			if (shader == "EnmCloud_nfe1") shader = "Common_d";
			if (shader == "EnmEmission_d") shader = "Luminescence_dE";
			if (shader == "EnmEmission_dl") shader = "Luminescence_dE";
			if (shader == "EnmEmission_dsl") shader = "Luminescence_dE";
			if (shader == "EnmEmission_dsle1") shader = "Luminescence_dE";
			if (shader == "EnmEmission_dsnl") shader = "Luminescence_dE";
			if (shader == "EnmEmission_dsnle1") shader = "Luminescence_dE";
			if (shader == "EnmGlass_dse1") shader = "Glass_de";
			if (shader == "EnmGlass_dsle1") shader = "Glass_de";
			if (shader == "EnmGlass_dsne1") shader = "Common_dsne[b]";
			if (shader == "EnmGlass_dsnle1") shader = "Common_dsne[b]";
			if (shader == "EnmGlass_e1") shader = "Glass_de";
			if (shader == "EnmMetal_dsnfe1") shader = "SonicSkin_dspnf[b]";
			if (shader == "EnmIgnore_dl") shader = "IgnoreLight_d";
			if (shader == "EnmIgnore_dle1") shader = "IgnoreLight_d";
			if (shader == "EnmIgnore_dsle1") shader = "Ring_dse";
			if (shader == "FadeOutNormal_dn") shader = "FadeOutNormal_dn";
			if (shader == "FakeGlass_dse1") shader = "Glass_de";
			if (shader == "FakeGlass_dsle1") shader = "Glass_de";
			if (shader == "FakeGlass_dsne1") shader = "Glass_de";
			if (shader == "FakeGlass_dsnle1") shader = "Glass_de";
			if (shader == "FakeGlass_e1") shader = "Glass_de";
			if (shader == "FallOffV_dn") shader = "FallOffV_dn";
			if (shader == "FallOffV_dnn") shader = "FallOffV_dn";
			if (shader == "FallOffV_dnn[bu]") shader = "FallOffV_dnn[bu]";
			if (shader == "FallOffV_dp") shader = "FallOffV_dp[b]";
			if (shader == "FallOffV_dp[b]") shader = "FallOffV_dp[b]";
			if (shader == "FallOff_df") shader = "FallOff_df[bw]";
			if (shader == "FallOff_df[bw]") shader = "FallOff_df[bw]";
			if (shader == "FallOff_dpf[v]") shader = "FallOff_dpf[v]";
			if (shader == "FallOff_dpnf") shader = "FallOff_dpnf[bw]";
			if (shader == "FallOff_dpnf[bw]") shader = "FallOff_dpnf[bw]";
			if (shader == "FontDirect_d") shader = "Common_d";
			if (shader == "Font_d") shader = "Common_d";
			if (shader == "GlassRefraction_de1") shader = "Glass_de";
			if (shader == "GlassRefraction_de2") shader = "Glass_de";
			if (shader == "Glass_d") shader = "Glass_de";
			if (shader == "Glass_de") shader = "Glass_de";
			if (shader == "Glass_de2") shader = "Glass_de[c]";
			if (shader == "Glass_de[bw]") shader = "Glass_de[bw]";
			if (shader == "Glass_de[b]") shader = "Glass_de[b]";
			if (shader == "Glass_de[c]") shader = "Glass_de";
			if (shader == "Glass_dpe") shader = "Glass_dpe";
			if (shader == "Glass_dpe2") shader = "Glass_dpe[c]";
			if (shader == "Glass_dpe[bw]") shader = "Glass_dpe[bw]";
			if (shader == "Glass_dpe[b]") shader = "Glass_dpe[b]";
			if (shader == "Glass_dpe[c]") shader = "Glass_dpe[c]";
			if (shader == "Glass_dpne") shader = "Glass_dpne";
			if (shader == "Glass_dspe") shader = "Glass_dspe";
			if (shader == "Glass_dspe2") shader = "Glass_dspe[c]";
			if (shader == "Glass_dspe[c]") shader = "Glass_dspe[c]";
			if (shader == "Glass_dspne") shader = "Glass_dspne[bw]";
			if (shader == "Glass_dspne[bw]") shader = "Glass_dspne[bw]";
			if (shader == "GrassInstance_L1") shader = "GrassInstance_L1";
			if (shader == "GrassInstance_L2") shader = "GrassInstance_L2";
			if (shader == "Ice_der[bv]") shader = "Ice_der[bv]";
			if (shader == "Ice_der[b]") shader = "Ice_der[b]";
			if (shader == "Ice_der[v]") shader = "Ice_der[v]";
			if (shader == "Ice_dnenr[cuv]") shader = "Ice_dnenr[cuv]";
			if (shader == "Ice_dnenr[cu]") shader = "Ice_dnenr[cu]";
			if (shader == "Ice_dnenr[uv]") shader = "Ice_dnenr[uv]";
			if (shader == "Ice_dnenr[u]") shader = "Ice_dnenr[u]";
			if (shader == "Ice_dner[bv]") shader = "Ice_dner[bv]";
			if (shader == "Ice_dner[v]") shader = "Ice_dner[v]";
			if (shader == "Ice_dpnenr") shader = "Ice_dpnenr";
			if (shader == "Ice_dpnenr[bc]") shader = "Ice_dpnenr[bc]";
			if (shader == "Ice_dpnenr[bw]") shader = "Ice_dpnenr[bw]";
			if (shader == "Ice_dpnenr[b]") shader = "Ice_dpnenr[b]";
			if (shader == "Ice_dpnenr[v]") shader = "Ice_dpnenr[v]";
			if (shader == "Ice_dpnr") shader = "Ice_dpnr";
			if (shader == "IgnoreLightTwice_d") shader = "IgnoreLight_d[v]";
			if (shader == "IgnoreLightV_d[s]") shader = "IgnoreLight_d[v]";
			if (shader == "IgnoreLight_d") shader = "IgnoreLight_d[v]";
			if (shader == "IgnoreLight_da") shader = "IgnoreLight_d[v]";
			if (shader == "IgnoreLight_dE") shader = "Luminescence_dE[v]";
			if (shader == "IgnoreLight_d[buv]") shader = "IgnoreLight_d[buv]";
			if (shader == "IgnoreLight_d[bu]") shader = "IgnoreLight_d[bu]";
			if (shader == "IgnoreLight_d[bv]") shader = "IgnoreLight_d[bv]";
			if (shader == "IgnoreLight_d[b]") shader = "IgnoreLight_d[b]";
			if (shader == "IgnoreLight_d[uv]") shader = "IgnoreLight_d[uv]";
			if (shader == "IgnoreLight_d[u]") shader = "IgnoreLight_d[u]";
			if (shader == "IgnoreLight_d[v]") shader = "IgnoreLight_d[v]";
			if (shader == "IndirectVnoGIs_doapn") shader = "Common_dpne";
			if (shader == "IndirectV_doap") shader = "Common_dpne";
			if (shader == "IndirectV_doapn") shader = "Common_dpne";
			if (shader == "Indirect_dop") shader = "Common_dp[v]";
			if (shader == "Indirect_dopn") shader = "Common_dpne[v]";
			if (shader == "Lava_dnE[u]") shader = "Lava_dnE";
			if (shader == "LightSpeedDashReady") shader = "LightSpeedDashReady";
			if (shader == "LuminescenceV_d") shader = "Luminescence_d[bv]";
			if (shader == "LuminescenceV_dn") shader = "Luminescence_d";
			if (shader == "LuminescenceV_dp") shader = "Luminescence_dpE";
			if (shader == "LuminescenceV_dpdpv") shader = "Luminescence_dpE";
			if (shader == "LuminescenceV_dpe1") shader = "Luminescence_dpE";
			if (shader == "LuminescenceV_dpe2") shader = "Luminescence_dpE";
			if (shader == "LuminescenceV_dpn") shader = "Luminescence_dpnE";
			if (shader == "LuminescenceV_dpndp") shader = "Luminescence_d";
			if (shader == "LuminescenceV_dpnv") shader = "Luminescence_d";
			if (shader == "Luminescence_d") shader = "Luminescence_d";
			if (shader == "Luminescence_dE") shader = "Luminescence_dE[bv]";
			if (shader == "Luminescence_dE[bv]") shader = "Luminescence_dE[bv]";
			if (shader == "Luminescence_dE[b]") shader = "Luminescence_dE[bv]";
			if (shader == "Luminescence_dE[v]") shader = "Luminescence_dE[bv]";
			if (shader == "Luminescence_dnE") shader = "Luminescence_dnE";
			if (shader == "Luminescence_dne1E") shader = "Luminescence_dnE";
			if (shader == "Luminescence_dne2E") shader = "Luminescence_dnE";
			if (shader == "Luminescence_dnE[uv]") shader = "Luminescence_dnE";
			if (shader == "Luminescence_dnE[u]") shader = "Luminescence_dnE[uv]";
			if (shader == "Luminescence_dnE[v]") shader = "Luminescence_dE[bv]";
			if (shader == "Luminescence_dpE") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpe1E") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpe2E") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpE[v]") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpnE") shader = "Luminescence_dpnE";
			if (shader == "Luminescence_dpne1E") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpne2E") shader = "Luminescence_dpE";
			if (shader == "Luminescence_dpnE[v]") shader = "Luminescence_dpE";
			if (shader == "Luminescence_d[buv]") shader = "Luminescence_d";
			if (shader == "Luminescence_d[bu]") shader = "Luminescence_d[b]";
			if (shader == "Luminescence_d[bv]") shader = "Luminescence_d";
			if (shader == "Luminescence_d[b]") shader = "Luminescence_d";
			if (shader == "Luminescence_d[v]") shader = "Luminescence_d";
			if (shader == "MakeShadowMap") shader = "MakeShadowMap";
			if (shader == "MakeShadowMapImage") shader = "MakeShadowMapImage";
			if (shader == "MakeShadowMapTransparent") shader = "MakeShadowMapTransparent";
			if (shader == "MeshParticleLightingShadow_d") shader = "";
			if (shader == "MeshParticleRef_d[v]") shader = "MeshParticleRef_d[v]";
			if (shader == "MeshParticle_d") shader = "MeshParticle_d";
			if (shader == "MeshParticle_dE[uv]") shader = "MeshParticle_d[uv]";
			if (shader == "MeshParticle_dE[v]") shader = "MeshParticle_d[v]";
			if (shader == "MeshParticle_d[uv]") shader = "MeshParticle_d[uv]";
			if (shader == "MeshParticle_d[u]") shader = "MeshParticle_d[u]";
			if (shader == "MeshParticle_d[v]") shader = "MeshParticle_d[v]";
			if (shader == "Mirror2_d") shader = "Glass_de";
			if (shader == "Mirror_d") shader = "Glass_de";
			if (shader == "Myst_d[bu]") shader = "Common_d[bu]";
			if (shader == "Myst_d[u]") shader = "Common_d[u]";
			if (shader == "Parallax_dpnh") shader = "Parallax_dpnh";
			if (shader == "RenderBuffer") shader = "RenderBuffer";
			if (shader == "Ring_dse") shader = "Ring_dse";
			if (shader == "Ring_dse[b]") shader = "Ring_dse[b]";
			if (shader == "SkyIndirect_dao[uv]") shader = "";
			if (shader == "SkyIndirect_do[uv]") shader = "";
			if (shader == "SkyInFocus_d") shader = "Sky_d";
			if (shader == "SkyInFocus_d[uv]") shader = "Sky_d[u]";
			if (shader == "SkyInFocus_d[u]") shader = "Sky_d[u]";
			if (shader == "SkyInFocus_d[v]") shader = "Sky_d[v]";
			if (shader == "Sky_d") shader = "Sky_d";
			if (shader == "Sky_daE[uv]") shader = "Sky_d[v]";
			if (shader == "Sky_da[uv]") shader = "Sky_d[v]";
			if (shader == "Sky_dE") shader = "Sky_d";
			if (shader == "Sky_dE[uv]") shader = "Sky_d[u]";
			if (shader == "Sky_dE[u]") shader = "Sky_d[u]";
			if (shader == "Sky_dE[v]") shader = "Sky_d[v]";
			if (shader == "Sky_d[uv]") shader = "Sky_d[u]";
			if (shader == "Sky_d[u]") shader = "Sky_d[u]";
			if (shader == "Sky_d[v]") shader = "Sky_d[v]";
			if (shader == "SysDiffuse") shader = "Common_d";
			if (shader == "SysError") shader = "Common_d";
			if (shader == "SysNormal") shader = "Common_d";
			if (shader == "test") shader = "test";
			if (shader == "TimeEaterDistortion_dsne1") shader = "Common_dsne[b]";
			if (shader == "TimeEaterEmission_dl") shader = "Luminescence_dE";
			if (shader == "TimeEaterGlass_dsne1") shader = "Common_dsne[b]";
			if (shader == "TimeEaterIndirect_dol") shader = "Common_d";
			if (shader == "TimeEaterMetal_dsnfe1") shader = "SonicSkin_dspf[b]";
			if (shader == "TimeEater_dbnn") shader = "Common_d";
			if (shader == "TimeEater_dsbnne1") shader = "Common_d";
			if (shader == "TransThin_dnt") shader = "TransThin_dnt";
			if (shader == "TransThin_dnt[b]") shader = "TransThin_dnt[b]";
			if (shader == "TransThin_dpnt") shader = "TransThin_dpnt";
			if (shader == "TransThin_dpnt[b]") shader = "TransThin_dpnt[b]";
			if (shader == "TransThin_dpt") shader = "TransThin_dpt";
			if (shader == "TransThin_dpt[b]") shader = "TransThin_dpt[b]";
			if (shader == "TransThin_dt") shader = "TransThin_dt";
			if (shader == "TransThin_dt[b]") shader = "TransThin_dt[b]";
			if (shader == "Water_Add") shader = "Water_Add";
			if (shader == "Water_Add[u]") shader = "Water_Add[u]";
			if (shader == "Water_Add_Ref") shader = "Water_Add_Ref[u]";
			if (shader == "Water_Add_Ref[u]") shader = "Water_Add_Ref[u]";
			if (shader == "Water_Add_SoftEdge") shader = "Water_Add_SoftEdge";
			if (shader == "Water_Add_SoftEdge[u]") shader = "Water_Add_SoftEdge[u]";
			if (shader == "Water_Add_SoftEdge_Ref") shader = "";
			if (shader == "Water_Add_SoftEdge_Ref[u]") shader = "";
			if (shader == "Water_Mul") shader = "Water_Mul";
			if (shader == "Water_Mul[u]") shader = "Water_Mul[u]";
			if (shader == "Water_Mul_Cube") shader = "Water_Mul_Cube[u]";
			if (shader == "Water_Mul_Cube[u]") shader = "Water_Mul_Cube[u]";
			if (shader == "Water_Mul_Offset") shader = "Water_Mul";
			if (shader == "Water_Mul_Offset_Lite") shader = "Water_Mul";
			if (shader == "Water_Mul_Ref") shader = "Water_Mul_Ref[u]";
			if (shader == "Water_Mul_Ref[u]") shader = "Water_Mul_Ref[u]";
			if (shader == "Water_Mul_SoftEdge") shader = "Water_Mul_SoftEdge";
			if (shader == "Water_Mul_SoftEdge[u]") shader = "Water_Mul_SoftEdge[u]";
			if (shader == "Water_Mul_SoftEdge_Ref") shader = "Water_Mul_SoftEdge_Ref";
			if (shader == "Water_Mul_SoftEdge_Ref[u]") shader = "Water_Mul_SoftEdge_Ref[u]";
			if (shader == "Water_Opacity") shader = "Water_Opacity";
			if (shader == "Water_Opacity[u]") shader = "Water_Opacity[u]";
			if (shader == "Water_Opacity_Cube") shader = "Water_Opacity_Cube[u]";
			if (shader == "Water_Opacity_Cube[u]") shader = "Water_Opacity_Cube[u]";
			if (shader == "Water_Opacity_Ref") shader = "Water_Opacity_Ref[u]";
			if (shader == "Water_Opacity_Ref[u]") shader = "Water_Opacity_Ref[u]";
			if (shader == "Water_Opacity_SoftEdge") shader = "Water_Add_SoftEdge";
			mat.setShader(shader);
			bool success = true;
			if (success) {
				mat.save(folderStr + "\\" + mat.getName() + ".material", LIBGENS_MATERIAL_ROOT_UNLEASHED);
				++converted;
			}
			else {
				++failed;
			}
		} while (FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}
	char msg[128];
	sprintf(msg, "Converted materials\nConverted : %d    Failed : %d", converted, failed);
	SHOW_MSG(msg);
}

std::string EditorApplication::SelectFolderWithIFileDialog(const wchar_t* title) {
	std::string result;
	IFileDialog* pFileDialog = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr)) {
		DWORD dwOptions;
		pFileDialog->GetOptions(&dwOptions);
		pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
		if (title) pFileDialog->SetTitle(title);
		hr = pFileDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			IShellItem* pItem = nullptr;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				PWSTR pszFilePath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
				if (SUCCEEDED(hr)) {
					char pathA[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, pszFilePath, -1, pathA, MAX_PATH, NULL, NULL);
					result = pathA;
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	return result;
}
void EditorApplication::openTerrainInfoDialog() {
	updateTerrainInfoDialog();
}

void EditorApplication::updateTerrainInfoDialog() {
	TerrainNode* terrainNode = nullptr;
	for (auto* node : selected_nodes) {
		if (node->getType() == EDITOR_NODE_TERRAIN) {
			terrainNode = static_cast<TerrainNode*>(node);
			break;
		}
	}

	if (!terrainNode || !current_level) return;

	LibGens::TerrainInstance* instance = terrainNode->getTerrainInstance();
	if (!instance) return;

	const std::string instance_name = instance->getName();

	LibGens::Terrain* terrain = current_level->getTerrain();
	if (!terrain) return;

	std::string group_name;
	int subset_id = 0;
	bool found = false;

	auto groups = terrain->getGroups();
	for (auto* group : groups) {
		auto instances = group->getInstances();
		for (auto* inst : instances) {
			if (inst == instance) {
				group_name = group->getName();
				subset_id = group->getSubsetID();
				found = true;
				break;
			}
		}
		if (found) break;
	}

	std::string msg = "Instance Name: " + instance_name + "\n";
	msg += "Group: " + group_name + "\n";
	msg += "Subset ID: " + std::to_string(subset_id) + "\n";
	msg += "Model: " + instance->getModelName();

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Terrain Info", msg.c_str(), sdl_window);
};

void EditorApplication::renderImGui() {
	renderMainMenuBar();

	if (show_left_panel) {
		renderLeftPanel();
	}

	if (show_bottom_panel) {
		renderBottomPanel();
	}

	if (show_properties_editor) {
		renderPropertyEditor();
	}

	if (show_material_editor) {
		renderMaterialEditor();
	}

	if (show_physics_editor) {
		renderPhysicsEditor();
	}

	if (show_find_dialog) {
		renderFindDialog();
	}

	if (show_look_at_dialog) {
		renderLookAtDialog();
	}

	if (show_multiset_dialog) {
		renderMultiSetDialog();
	}

	if (show_terrain_info) {
		renderTerrainInfoDialog();
	}

	if (show_quick_overview) {
		renderQuickOverviewDialog();
	}
}

void EditorApplication::renderMaterialEditor() {
	ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(800, 700), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("Material Editor", &show_material_editor, ImGuiWindowFlags_None);

	// Mode and info on same line
	const char* mode_labels[] = { "Model", "Material", "Terrain" };
	int mode = (int)material_editor_mode;
	ImGui::SetNextItemWidth(120);
	if (ImGui::Combo("Mode", &mode, mode_labels, IM_ARRAYSIZE(mode_labels))) {
		material_editor_mode = (size_t)mode;
		if (mode == SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN) materialEditorTerrainMode();
		else materialEditorModelMode();	
	}
	ImGui::SameLine();
	ImGui::Text("Model: %s", material_editor_model ? material_editor_model->getName().c_str() : "None");
	ImGui::Text("Skeleton: %s  Animation: %s", 
		material_editor_skeleton_name.size()? material_editor_skeleton_name.c_str():"None",
		material_editor_animation_name.size()? material_editor_animation_name.c_str():"None");

	if (ImGui::Button("Load Model")) loadMaterialEditorModelGUI();
	ImGui::SameLine();
	if (ImGui::Button("Load Skeleton")) loadMaterialEditorSkeletonGUI();
	ImGui::SameLine();
	if (ImGui::Button("Load Animation")) loadMaterialEditorAnimationGUI();
	ImGui::SameLine();
	if (ImGui::Button("Save")) saveMaterialEditorMaterial();
	ImGui::SameLine();
	if (ImGui::Button("Save All")) saveAllMaterialEditorMaterials();

	ImGui::Separator();
	
	// Materials and Shader side by side
	ImGui::BeginChild("MaterialList", ImVec2(220, 150), true);
	ImGui::Text("Materials");
	ImGui::Separator();
	for (int i=0;i<(int)material_editor_materials.size();++i) {
		bool selected = (i == material_editor_list_selection);
		if (ImGui::Selectable(material_editor_materials[i]->getName().c_str(), selected)) {
			updateMaterialEditorIndex(i);
		}
		if (selected) ImGui::SetItemDefaultFocus();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::Text("Shader");
	static char shader_input[512] = "";
	static char shader_filter[512] = "";
	static int last_material_index = -1;
	static bool combo_just_opened = false;
	
	if (material_editor_material) {
		if (material_editor_list_selection != last_material_index) {
			last_material_index = material_editor_list_selection;
			strncpy(shader_input, material_editor_material->getShader().c_str(), sizeof(shader_input));
			shader_input[sizeof(shader_input) - 1] = '\0';
		}
		
		ImGui::SetNextItemWidth(250);
		if (ImGui::BeginCombo("##ShaderCombo", shader_input)) {
			if (combo_just_opened) {
				shader_filter[0] = '\0';
				combo_just_opened = false;
				ImGui::SetKeyboardFocusHere();
			}
			
			// Filter input at the top
			ImGui::InputText("##ShaderFilter", shader_filter, sizeof(shader_filter));
			ImGui::Separator();
			
			// Create filtered list
			string filter_lower = string(shader_filter);
			std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
			
			for (size_t i=0;i<material_editor_shader_names.size();++i) {
				string shader_lower = material_editor_shader_names[i];
				std::transform(shader_lower.begin(), shader_lower.end(), shader_lower.begin(), ::tolower);
				
				// Show if filter is empty or matches
				if (filter_lower.empty() || shader_lower.find(filter_lower) != string::npos) {
					bool is_selected = (material_editor_material->getShader()==material_editor_shader_names[i]);
					if (ImGui::Selectable(material_editor_shader_names[i].c_str(), is_selected)) {
						strncpy(shader_input, material_editor_shader_names[i].c_str(), sizeof(shader_input));
						shader_input[sizeof(shader_input) - 1] = '\0';
						updateEditShaderMaterialEditor(material_editor_shader_names[i]);
						if (material_editor_defaults_on_shader_change) {
							loadMaterialDefaultParams();
						}
						updateMaterialEditorInfo();
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
			}
			ImGui::EndCombo();
		} else {
			combo_just_opened = true;
		}
	}
	
	ImGui::Checkbox("Defaults on Shader Change", &material_editor_defaults_on_shader_change);
	ImGui::EndGroup();

	if (material_editor_material) {
		bool no_cull = material_editor_material->hasNoCulling();
		if (ImGui::Checkbox("No Culling", &no_cull)) {
			material_editor_material->setNoCulling(no_cull);
		}
		ImGui::SameLine();
		bool additive = material_editor_material->hasColorBlend();
		if (ImGui::Checkbox("Additive", &additive)) {
			material_editor_material->setColorBlend(additive);
		}
	}

	ImGui::Text("Texture Units");
	ImGui::BeginChild("TextureList", ImVec2(250, 120), true);
	if (material_editor_material) {
		for (int i=0;i<material_editor_material->getTextureUnitsSize();++i) {
			LibGens::Texture* tex = material_editor_material->getTextureByIndex(i);
			string label = tex->getName()+" ("+tex->getUnit()+")";
			bool selected = (i==texture_list_selection);
			if (ImGui::Selectable(label.c_str(), selected)) {
				updateMaterialEditorTextureIndex(i);
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
	}
	ImGui::EndChild();
	if (ImGui::Button("Add Texture")) addMaterialEditorTextureGUI();
	ImGui::SameLine();
	if (ImGui::Button("Remove Texture")) removeMaterialEditorTexture();
	ImGui::SameLine();
	if (ImGui::Button("Defaults")) loadMaterialDefaultParams();

	if (material_editor_texture) {
		ImGui::InputText("Texture Name", texture_filename_buf, sizeof(texture_filename_buf));
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			updateEditTextureMaterialEditor(texture_filename_buf, true);
		}
		ImGui::InputText("Unit", texture_unit_name_buf, sizeof(texture_unit_name_buf));
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			updateEditTextureUnitMaterialEditor(texture_unit_name_buf);
		}
		if (!material_editor_slot_names.empty()) {
			if (texture_slot_index < 0) texture_slot_index = 0;
			if (texture_slot_index >= (int)material_editor_slot_names.size()) texture_slot_index = (int)material_editor_slot_names.size()-1;
			const char* current_slot = material_editor_slot_names[texture_slot_index].c_str();
			if (ImGui::BeginCombo("Slot", current_slot)) {
				for (int i=0;i<(int)material_editor_slot_names.size();++i) {
					bool sel = (i==texture_slot_index);
					if (ImGui::Selectable(material_editor_slot_names[i].c_str(), sel)) {
						texture_slot_index = i;
						updateEditTextureUnitMaterialEditor(material_editor_slot_names[i]);
					}
					if (sel) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		if (ImGui::Button("Choose Texture")) pickMaterialEditorTextureGUI();
	}

	ImGui::Separator();
	ImGui::Text("Parameters");
	ImGui::BeginChild("ParamGrid", ImVec2(400, 0), true);
	if (material_editor_material) {
		vector<LibGens::Parameter*> params = material_editor_material->getParameters();
		for (int i=0;i<(int)params.size();i++) {
			ImGui::PushID(i);
			ImGui::Text("%s", params[i]->getName().c_str());
			float rgba[4];
			LibGens::Color c = params[i]->getColor();
			rgba[0]=c.r; rgba[1]=c.g; rgba[2]=c.b; rgba[3]=c.a;
			
			bool changed = false;
			ImGui::Text("R:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(60);
			changed |= ImGui::InputFloat("##R", &rgba[0], 0.0f, 0.0f, "%.3f");
			ImGui::SameLine();
			
			ImGui::Text("G:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(60);
			changed |= ImGui::InputFloat("##G", &rgba[1], 0.0f, 0.0f, "%.3f");
			ImGui::SameLine();
			
			ImGui::Text("B:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(60);
			changed |= ImGui::InputFloat("##B", &rgba[2], 0.0f, 0.0f, "%.3f");
			ImGui::SameLine();
			
			ImGui::Text("A:"); ImGui::SameLine();
			ImGui::SetNextItemWidth(60);
			changed |= ImGui::InputFloat("##A", &rgba[3], 0.0f, 0.0f, "%.3f");
			
			if (changed) {
				updateEditParameterMaterialEditor(i, LibGens::Color(rgba[0], rgba[1], rgba[2], rgba[3]));
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();



	ImGui::End();
}