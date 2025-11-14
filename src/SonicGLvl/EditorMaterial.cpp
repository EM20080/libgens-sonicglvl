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
#include "AR.h"
#include "Texture.h"
#include "Parameter.h"
#include "Material.h"

static bool hasScene = false;
// fixes an issue where closing the preview window for models doesn't open back when you close a material, and if you tried doing it via 
// code, ogre brings already existing window errors. 
void EditorApplication::closePreviewMaterialEditorGUI() {
	if (!hasScene) return;
	if (material_editor_preview_window && material_editor_preview_listener) {
		Ogre::WindowEventUtilities::removeWindowEventListener(material_editor_preview_window, material_editor_preview_listener);
		root->removeFrameListener(material_editor_preview_listener);
	}
	if (material_editor_preview_window) {
		material_editor_preview_window->destroy();
		material_editor_preview_window = NULL;
	}
	Ogre::RenderTarget* existingRT = root->getRenderTarget("Preview Window");
	if (existingRT) {
		root->getRenderSystem()->destroyRenderTarget("Preview Window");
	}
	if (material_editor_viewport) {
		delete material_editor_viewport;
		material_editor_viewport = NULL;
	}
	if (material_editor_preview_scene_manager) {
		root->destroySceneManager(material_editor_preview_scene_manager);
		material_editor_preview_scene_manager = NULL;
	}
	if (material_editor_preview_bogus_scene_manager) {
		root->destroySceneManager(material_editor_preview_bogus_scene_manager);
		material_editor_preview_bogus_scene_manager = NULL;
	}
	if (material_editor_preview_listener) {
		delete material_editor_preview_listener;
		material_editor_preview_listener = NULL;
	}
	material_editor_scene_node = NULL;
	material_editor_animation_state = NULL;
	hasScene = false;
}

void EditorApplication::createPreviewMaterialEditorGUI() {
	Ogre::RenderTarget* existingRT = root->getRenderTarget("Preview Window");
	if (existingRT && !hasScene) {
		root->getRenderSystem()->destroyRenderTarget("Preview Window");
	}
	if (hasScene && (!material_editor_preview_window || material_editor_preview_window->isClosed())) {
		closePreviewMaterialEditorGUI();
	}
	if (!hasScene) {
		material_editor_preview_scene_manager = root->createSceneManager("DefaultSceneManager");
		material_editor_preview_scene_manager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
		material_editor_preview_bogus_scene_manager = root->createSceneManager("DefaultSceneManager");
		Ogre::Light* dir_light = material_editor_preview_scene_manager->createLight("Preview Directional Light");
		dir_light->setSpecularColour(Ogre::ColourValue::White);
		dir_light->setDiffuseColour(Ogre::ColourValue(1.0, 1.0, 1.0));
		dir_light->setType(Ogre::Light::LT_DIRECTIONAL);
		dir_light->setDirection(Ogre::Vector3(1, 1, 1).normalisedCopy());
		Ogre::NameValuePairList misc;
		misc["FSAA"] = Ogre::StringConverter::toString(8);
		misc["vsync"] = Ogre::StringConverter::toString(true);
		material_editor_preview_window = root->createRenderWindow("Preview Window", 640, 480, false, &misc);
		material_editor_preview_window->setDeactivateOnFocusChange(false);
		material_editor_preview_window->setAutoUpdated(true);
		material_editor_viewport = new EditorViewport(material_editor_preview_scene_manager, material_editor_preview_bogus_scene_manager, material_editor_preview_window, SONICGLVL_CAMERA_PREVIEW_NAME);
		material_editor_viewport->setPanningMultiplier(3);
		material_editor_viewport->setZoomingMultiplier(0.04);
		material_editor_viewport->setNearClipDistance(0.001f);
		material_editor_viewport->setFarClipDistance(100.0f);
		material_editor_preview_listener = new MaterialEditorPreviewListener();
		material_editor_preview_listener->setEditorViewport(material_editor_viewport);
		material_editor_preview_listener->setEditorWindow(material_editor_preview_window);
		OIS::ParamList pl;
		size_t windowHnd = 0; std::ostringstream windowHndStr; material_editor_preview_window->getCustomAttribute("WINDOW", &windowHnd); windowHndStr << windowHnd; pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str())); pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND"))); pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE"))); pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND"))); pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
		material_editor_input_manager = OIS::InputManager::createInputSystem(pl);
		material_editor_keyboard = static_cast<OIS::Keyboard*>(material_editor_input_manager->createInputObject(OIS::OISKeyboard, true));
		material_editor_mouse = static_cast<OIS::Mouse*>(material_editor_input_manager->createInputObject(OIS::OISMouse, true));
		material_editor_mouse->setEventCallback(material_editor_preview_listener);
		material_editor_keyboard->setEventCallback(material_editor_preview_listener);
		Ogre::WindowEventUtilities::addWindowEventListener(material_editor_preview_window, material_editor_preview_listener);
		root->addFrameListener(material_editor_preview_listener);
		material_editor_preview_listener->setMouse(material_editor_mouse);
		material_editor_preview_listener->setKeyboard(material_editor_keyboard);
		unsigned int width, height, depth; int left, top; material_editor_preview_window->getMetrics(width, height, depth, left, top); const OIS::MouseState& ms = material_editor_mouse->getMouseState(); ms.width = width; ms.height = height; 
	}
	rebuildMaterialPreviewNodes();
	material_editor_model->buildAABB();
	LibGens::AABB model_aabb = material_editor_model->getAABB();
	LibGens::Vector3 aabb_center = model_aabb.center();
	Ogre::Vector3 camera_center = Ogre::Vector3(aabb_center.x, aabb_center.y, aabb_center.z);
	float size_max = model_aabb.sizeMax();
	Ogre::Camera* camera = material_editor_viewport->getCamera();
	camera->setPosition(camera_center);
	camera->setDirection(Ogre::Vector3(0, 0, -1).normalisedCopy());
	camera->moveRelative(Ogre::Vector3::UNIT_Z * size_max * 1.5f);
	hasScene = true;
}


void EditorApplication::openMaterialEditorGUI() {
	show_material_editor = true;
	material_editor_model = NULL;
	material_editor_model_filename.clear();
	material_editor_material = NULL;
	material_editor_skeleton_name.clear();
	material_editor_animation_name.clear();
	material_editor_animation_state = NULL;
	material_editor_scene_node = NULL;
	material_editor_mesh_group = PREVIEW_MESH_GROUP;
	material_editor_mode = SONICGLVL_MATERIAL_EDITOR_MODE_MODEL;
	material_editor_materials.clear();
	material_editor_defaults_on_shader_change = false;
	clearSelectionMaterialEditorGUI();
}

void EditorApplication::enableMaterialEditorGUI(bool enable) {
}


void EditorApplication::enableMaterialEditorListGUI() {
}

void EditorApplication::updateMaterialEditorTextureList() {

}

void EditorApplication::updateMaterialTextureInfo() {
	if (!material_editor_texture) return;
	strncpy(texture_filename_buf, material_editor_texture->getName().c_str(), sizeof(texture_filename_buf));
	texture_filename_buf[sizeof(texture_filename_buf)-1] = '\0';
	strncpy(texture_unit_name_buf, material_editor_texture->getTexset().c_str(), sizeof(texture_unit_name_buf));
	texture_unit_name_buf[sizeof(texture_unit_name_buf)-1] = '\0';
	// Update slot index to match unit name if possible
	texture_slot_index = 0;
	for (size_t i=0;i<material_editor_slot_names.size();++i) {
		if (material_editor_slot_names[i] == material_editor_texture->getUnit()) {
			texture_slot_index = (int)i;
			break;
		}
	}
}

void EditorApplication::updateMaterialEditorInfo() {
	if (!material_editor_material) return;
	strncpy(material_name_buf, material_editor_material->getName().c_str(), sizeof(material_name_buf));
	material_name_buf[sizeof(material_name_buf)-1] = '\0';
	vector<LibGens::Parameter*> parameters = material_editor_material->getParameters();
	for (size_t i=0;i<10;i++) {
		if (i < parameters.size()) {
			strncpy(material_param_name_buf[i], parameters[i]->getName().c_str(), sizeof(material_param_name_buf[i]));
			material_param_name_buf[i][sizeof(material_param_name_buf[i])-1] = '\0';
			LibGens::Color c = parameters[i]->getColor();
			material_param_rgba[i][0]=c.r;
			material_param_rgba[i][1]=c.g;
			material_param_rgba[i][2]=c.b;
			material_param_rgba[i][3]=c.a;
		} else {
			material_param_name_buf[i][0]='\0';
			material_param_rgba[i][0]=material_param_rgba[i][1]=material_param_rgba[i][2]=material_param_rgba[i][3]=0.0f;
		}
	}
	// Build texture unit slot names from shader library
	material_editor_slot_names.clear();
	if (material_editor_shader_library) {
		string shader_name = material_editor_material->getShader();
		LibGens::Shader* vs = NULL; LibGens::Shader* ps = NULL;
		material_editor_shader_library->getMaterialShaders(shader_name, vs, ps, false, !material_editor_material->hasExtraGI(), false);
		if (ps) {
			vector<string> names = ps->getShaderParameterFilenames();
			for (size_t i = 0; i < names.size(); i++) {
				LibGens::ShaderParams* params = material_editor_shader_library->getPixelShaderParams(names[i]);
				if (params->getName() == "global") continue;
				vector<LibGens::ShaderParam*> paramList = params->getParameterList(3);
				for (size_t j = 0; j < paramList.size(); j++) {
					material_editor_slot_names.push_back(paramList[j]->getName());
				}
			}
		}
	}
}

void EditorApplication::loadMaterialDefaultParams() {
	if (!material_editor_material || !material_editor_shader_library) {
		return;
	}

	LibGens::Material* material = material_editor_material;
	material->removeAllParameters();
	string shader_name = material->getShader();
	LibGens::Shader* vertex_shader = NULL;
	LibGens::Shader* pixel_shader = NULL;
	material_editor_shader_library->getMaterialShaders(shader_name, vertex_shader, pixel_shader, false, !material->hasExtraGI(), false);

	if (pixel_shader) {
		vector<string> names = pixel_shader->getShaderParameterFilenames();
		for (size_t i = 0; i < names.size(); i++) {
			LibGens::ShaderParams* params = material_editor_shader_library->getPixelShaderParams(names[i]);
			vector<LibGens::ShaderParam*> paramList = params->getParameterList(0);
			for (size_t i2 = 0; i2 < paramList.size(); i2++) {
				if (paramList[i2]->getName().rfind("g_", 0) == -1 && paramList[i2]->getName().rfind("mrg", 0) == -1)
				{
					material->addParameter(new LibGens::Parameter(paramList[i2]->getName(), LibGens::Color(1, 1, 1, 1)));
				}
			}
		}
		updateMaterialEditorInfo();
		updateEditShaderMaterialEditor(shader_name);
	}
}

void EditorApplication::removeMaterialEditorTexture() {
	if (!material_editor_texture)
		return;

	material_editor_material->removeTextureUnitByIndex(texture_list_selection);
	updateMaterialEditorTextureList();

	Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

	if (ogre_material) {
		updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
	}
}

void EditorApplication::materialEditorTerrainMode() {
	material_editor_mode = SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN;
	material_editor_mesh_group = GENERAL_MESH_GROUP;
	if (material_editor_model) {
		cleanMaterialEditorModelGUI();
	}

	clearSelectionMaterialEditorGUI();
	enableMaterialEditorListGUI();
	material_editor_materials.clear();

	material_editor_material_library = current_level->getTerrain()->getMaterialLibrary();
	material_editor_library_folder = current_level->getTerrain()->getResourcesFolder();

	for (LibGens::Material* mat : material_editor_material_library->getMaterials()) {
		material_editor_materials.push_back(mat);
	}

	std::sort(material_editor_materials.begin(), material_editor_materials.end(),
		[](LibGens::Material* lhs, LibGens::Material* rhs) {
			return lhs->getName().compare(rhs->getName()) < 0;
		});

	rebuildListMaterialEditorGUI();
}

void EditorApplication::materialEditorModelMode() {
	material_editor_mode = SONICGLVL_MATERIAL_EDITOR_MODE_MODEL;
	material_editor_mesh_group = PREVIEW_MESH_GROUP;
	material_editor_materials.clear();
	clearSelectionMaterialEditorGUI();
	rebuildListMaterialEditorGUI();
}

void EditorApplication::clearMaterialEditorGUI() {
	cleanMaterialEditorModelGUI();
	closePreviewMaterialEditorGUI();
	show_material_editor = false;
}

void EditorApplication::cleanMaterialEditorModelGUI() {
	if (material_editor_model) {
		delete material_editor_model;
	}

	material_editor_model = NULL;
}

void EditorApplication::clearSelectionMaterialEditorGUI() {
	material_editor_list_selection = -1;
	last_material_editor_list_selection = -1;
	texture_list_selection = -1;
	last_texture_list_selection = -1;
	enableMaterialEditorGUI(false);
}

void EditorApplication::clearTextureInfo() {
	material_editor_texture = NULL;
	texture_filename_buf[0] = '\0';
	texture_unit_name_buf[0] = '\0';
}


void EditorApplication::rebuildMaterialPreviewNodes() {
	if (material_editor_scene_node) {
		destroySceneNode(material_editor_scene_node);
	}

	if (material_editor_model) {
		cleanModelResource(material_editor_model, PREVIEW_MESH_GROUP);
	}

	// Create the nodes
	material_editor_scene_node = material_editor_preview_scene_manager->getRootSceneNode()->createChildSceneNode();


	if (material_editor_skeleton_name.size() && material_editor_animation_name.size()) {
		prepareSkeletonAndAnimation(material_editor_skeleton_name, material_editor_animation_name);
	}

	LibGens::ShaderLibrary* shader_library = NULL;

	if (material_editor_unleashed) {
		shader_library = unleashed_shader_library;
	}
	else {
		shader_library = generations_shader_library;
	}

	buildModel(material_editor_scene_node, material_editor_model, material_editor_model->getName(), material_editor_skeleton_name, material_editor_preview_scene_manager, material_editor_material_library, 0, PREVIEW_MESH_GROUP, false, shader_library);


	material_editor_animation_state = NULL;
	if (material_editor_animation_name.size()) {
		unsigned short attached_objects = material_editor_scene_node->numAttachedObjects();
		for (unsigned short i = 0; i < attached_objects; i++) {
			Ogre::Entity* entity = static_cast<Ogre::Entity*>(material_editor_scene_node->getAttachedObject(i));

			if (entity->hasAnimationState(material_editor_animation_name)) {
				material_editor_animation_state = entity->getAnimationState(material_editor_animation_name);
				material_editor_animation_state->setLoop(true);
				material_editor_animation_state->setEnabled(true);
				break;
			}
		}
	}

	material_editor_preview_listener->setAnimationState(material_editor_animation_state);
}


void EditorApplication::rebuildListMaterialEditorGUI() {
	// Populate the shader list for ImGui combo
	material_editor_shader_names.clear();
	bool shader_library_unleashed = false;

	if (material_editor_mode == SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN && current_level != NULL) {
		shader_library_unleashed = (current_level->getGameMode() == LIBGENS_LEVEL_GAME_UNLEASHED);
	}
	else {
		shader_library_unleashed = material_editor_unleashed;
	}

	checkShaderLibrary(shader_library_unleashed ? LIBGENS_LEVEL_GAME_UNLEASHED : LIBGENS_LEVEL_GAME_GENERATIONS);

	material_editor_shader_library = shader_library_unleashed ? unleashed_shader_library : generations_shader_library;

	if (material_editor_shader_library) {
		for (size_t i = 0; i < material_editor_shader_library->getFileCount(); i++) {
			LibGens::ArFile* ar_file = material_editor_shader_library->getFileByIndex(i);
			if (ar_file->getName().find(".shader-list") != string::npos) {
				material_editor_shader_names.push_back(LibGens::File::nameFromFilenameNoExtension(ar_file->getName()));
			}
		}
	}
}


void EditorApplication::saveMaterialEditorModelGUI() {
	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Generations Model File(.model)\0*.model\0Unleashed Model File(.model)\0*.model\0";
	ofn.nFilterIndex = material_editor_unleashed ? 2 : 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose where you would like save the model";
	ofn.lpstrFile = (LPSTR)material_editor_model_filename.c_str();
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST |
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
		OFN_ENABLESIZING;

	if (GetSaveFileName(&ofn)) {
		string folder = LibGens::File::folderFromFilename(ofn.lpstrFile);
		material_editor_model->save(ToString(ofn.lpstrFile));

		for (LibGens::Material* mat : material_editor_materials) {
			mat->save(folder + "\\" + mat->getName() + ".material", ofn.nFilterIndex == 2 ? LIBGENS_MATERIAL_ROOT_UNLEASHED : LIBGENS_MATERIAL_ROOT_GENERATIONS);
		}
	}

	free(filename);
}

void EditorApplication::saveMaterialEditorMaterial() {
	if (!material_editor_material)
		return;

	bool save_unleashed = false;

	if (material_editor_mode == SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN && current_level != NULL) {
		save_unleashed = (current_level->getGameMode() == LIBGENS_LEVEL_GAME_UNLEASHED);
	}
	else {
		save_unleashed = material_editor_unleashed;
	}

	material_editor_material->save(material_editor_library_folder + "\\" + material_editor_material->getName() + ".material", save_unleashed ? LIBGENS_MATERIAL_ROOT_UNLEASHED : LIBGENS_MATERIAL_ROOT_GENERATIONS);
}

// Self Explanatory, its the Save All Button. 
void EditorApplication::saveAllMaterialEditorMaterials() {
	if (material_editor_materials.empty()) return;
	bool save_unleashed = false;
	if (material_editor_mode == SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN && current_level != NULL) {
		save_unleashed = (current_level->getGameMode() == LIBGENS_LEVEL_GAME_UNLEASHED);
	} else {
		save_unleashed = material_editor_unleashed;
	}
	for (LibGens::Material* mat : material_editor_materials) {
		if (mat) {
			mat->save(material_editor_library_folder + "\\" + mat->getName() + ".material", save_unleashed ? LIBGENS_MATERIAL_ROOT_UNLEASHED : LIBGENS_MATERIAL_ROOT_GENERATIONS);
		}
	}
}

void EditorApplication::loadMaterialEditorModelGUI() {
	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Model File(.model)\0*.model\0Terrain Model File(.terrain-model)\0*.terrain-model\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose the Model File you want to edit";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_LONGNAMES | OFN_EXPLORER |
		OFN_HIDEREADONLY | OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());

		if (material_editor_model) {
			cleanMaterialEditorModelGUI();
		}

		// Load Model into memory and fetch the material names
		material_editor_model_filename = ToString(filename);
		material_editor_model = new LibGens::Model(material_editor_model_filename);
		list<string> material_names = material_editor_model->getMaterialNames();

		// Build material library
		material_editor_library_folder = LibGens::File::folderFromFilename(material_editor_model_filename);
		material_editor_material_library = new LibGens::MaterialLibrary(material_editor_library_folder);

		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(material_editor_library_folder, "FileSystem");

		// Enable the List UI
		enableMaterialEditorListGUI();

		// Clear Material List and insert the ones from the model
		clearSelectionMaterialEditorGUI();
		material_editor_materials.clear();
		material_editor_unleashed = true;
		for (list<string>::iterator it = material_names.begin(); it != material_names.end(); it++) {
			LibGens::Material* mat = material_editor_material_library->getMaterial(*it);

			if (mat) {
				if (mat->getRootNodeType() != LIBGENS_MATERIAL_ROOT_UNLEASHED) {
					material_editor_unleashed = false;
				}
				material_editor_materials.push_back(mat);
			}
		}

		std::sort(material_editor_materials.begin(), material_editor_materials.end(),
			[](LibGens::Material* lhs, LibGens::Material* rhs) {
				return lhs->getName().compare(rhs->getName()) < 0;
			});

		rebuildListMaterialEditorGUI();
		createPreviewMaterialEditorGUI();
	}

	chdir(exe_path.c_str());
	free(filename);
}

void EditorApplication::copyMaterialEditorTexture(const string& file) const
{
	string destination_folder;

	if (material_editor_mode == SONICGLVL_MATERIAL_EDITOR_MODE_MODEL)
		destination_folder = LibGens::File::folderFromFilename(material_editor_model_filename);

	else if (material_editor_mode == SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN && editor_application->getCurrentLevel())
		destination_folder = editor_application->getCurrentLevel()->getResourcesFolder();

	if (!destination_folder.empty())
		CopyFile(file.c_str(), (destination_folder + "\\" + LibGens::File::nameFromFilename(file)).c_str(), false);
}

void EditorApplication::pickMaterialEditorTextureGUI() {
	if (!material_editor_texture)
		return;

	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "DirectX Texture File(.dds)\0*.dds\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose the Texture File you want to use";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_LONGNAMES | OFN_EXPLORER |
		OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());
		string file = ToString(ofn.lpstrFile);
		copyMaterialEditorTexture(file);
		updateEditTextureMaterialEditor(LibGens::File::nameFromFilenameNoExtension(file), true);
	}
	chdir(exe_path.c_str());
	free(filename);
}

void EditorApplication::addMaterialEditorTextureGUI() {
	if (!material_editor_material)
		return;

	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "DirectX Texture File(.dds)\0*.dds\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose the Texture File you want to use";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_LONGNAMES | OFN_EXPLORER |
		OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());
		string file = ToString(ofn.lpstrFile);
		string internal_name = LibGens::File::nameFromFilenameNoExtension(file);
		char buffer[8];
		sprintf(buffer, "-%04d", material_editor_material->getTextureUnitsSize());
		string unitName = material_editor_material->getName() + ToString(buffer);
		string unit = "diffuse";
		if (internal_name.size() > 3) {
			string fileUnit = internal_name.substr(internal_name.size() - 3, 3);
			if (fileUnit == "spc" || fileUnit == "pec")
				unit = "specular";
			else if (fileUnit == "pow")
				unit = "gloss";
			else if (fileUnit == "nrm")
				unit = "normal";
			else if (fileUnit == "fal")
				unit = "displacement";
			else if (fileUnit == "env" || fileUnit == "ref")
				unit = "reflection";
		}
		LibGens::Texture* tex = new LibGens::Texture(unitName, unit, internal_name);
		material_editor_material->addTextureUnit(tex);

		copyMaterialEditorTexture(file);

		Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

		if (ogre_material) {
			updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
		}

		updateMaterialEditorTextureList();
	}
	chdir(exe_path.c_str());
	free(filename);
}

void EditorApplication::loadMaterialEditorSkeletonGUI() {
	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Havok Skeleton File(.skl.hkx)\0*.skl.hkx\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose the Skeleton File you want to use";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_LONGNAMES | OFN_EXPLORER |
		OFN_HIDEREADONLY | OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());

		material_editor_skeleton_name = LibGens::File::nameFromFilenameNoExtension(ToString(filename));
		string havok_library_folder = LibGens::File::folderFromFilename(ToString(filename));
		material_editor_animation_name = "";

		havok_enviroment->addFolder(havok_library_folder);
	}

	chdir(exe_path.c_str());
	free(filename);
}



void EditorApplication::loadMaterialEditorAnimationGUI() {
	char* filename = (char*)malloc(1024);
	strcpy(filename, "");

	OPENFILENAME    ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Havok Animation File(.anm.hkx)\0*.anm.hkx\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = "Choose the Animation File you want to use";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
		OFN_LONGNAMES | OFN_EXPLORER |
		OFN_HIDEREADONLY | OFN_ENABLESIZING;

	if (GetOpenFileName(&ofn)) {
		chdir(exe_path.c_str());

		material_editor_animation_name = LibGens::File::nameFromFilenameNoExtension(ToString(filename));
		string havok_library_folder = LibGens::File::folderFromFilename(ToString(filename));

		havok_enviroment->addFolder(havok_library_folder);

		rebuildMaterialPreviewNodes();
	}

	chdir(exe_path.c_str());
	free(filename);
}


void EditorApplication::updateMaterialEditorIndex(int selection_index) {
	material_editor_list_selection = selection_index;
	if (material_editor_list_selection != last_material_editor_list_selection) {
		last_material_editor_list_selection = material_editor_list_selection;
		if (material_editor_list_selection != -1 && material_editor_list_selection < (int)material_editor_materials.size()) {
			material_editor_material = material_editor_materials[material_editor_list_selection];
			updateMaterialEditorInfo();
			enableMaterialEditorGUI(true);
		} else {
			enableMaterialEditorGUI(false);
			material_editor_material = NULL;
		}
	}
}


void EditorApplication::updateMaterialEditorTextureIndex(int selection_index) {
	texture_list_selection = selection_index;
	if (texture_list_selection != last_texture_list_selection) {
		last_texture_list_selection = texture_list_selection;
		if (texture_list_selection != -1 && material_editor_material) {
			material_editor_texture = material_editor_material->getTextureByIndex(texture_list_selection);
			updateMaterialTextureInfo();
		} else {
			clearTextureInfo();
		}
	}
}

void EditorApplication::updateEditParameterMaterialEditor(size_t i, LibGens::Color parameter_color) {
	if (!material_editor_material) return;

	LibGens::Parameter* parameter = material_editor_material->getParameterByIndex(i);
	if (parameter) {
		parameter->color = parameter_color;
	}
	else return;

	Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

	if (ogre_material) {
		updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
	}
}


void EditorApplication::updateEditShaderMaterialEditor(string shader_name) {
	if (!material_editor_material) return;

	material_editor_material->setShader(shader_name);
	Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

	if (ogre_material) {
		updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
	}
}

void EditorApplication::updateEditTextureMaterialEditor(string texture_name, bool update_ui) {
	if (!material_editor_texture)
		return;

	material_editor_texture->setName(texture_name);
	Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

	if (ogre_material) {
		updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
	}

	if (update_ui) {
		// ImGui buffers will be updated by updateMaterialTextureInfo() iirc..
		updateMaterialTextureInfo();
	}
}

void EditorApplication::updateEditTextureUnitMaterialEditor(string unit_name) {
	if (!material_editor_texture)
		return;

	material_editor_texture->setUnit(unit_name);

	Ogre::Material* ogre_material = Ogre::MaterialManager::getSingleton().getByName(material_editor_material->getExtra(), material_editor_mesh_group).getPointer();

	if (ogre_material) {
		updateMaterialShaderParameters(ogre_material, material_editor_material, !material_editor_material->hasExtraGI(), NULL, material_editor_shader_library);
	}
}
// Legacy Win32 callback implementations removed; ImGui-based rendering will handle
// all material editor interactions.