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

//==================================================================================================================================================
// Very Heavily Modified from the Ogre3D Wiki: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Ogre+Wiki+Tutorial+Framework&structure=Development
//==================================================================================================================================================

#include "BaseApplication.h"
#include "EditorViewport.h"
#include "ObjectNode.h"
#include "TerrainNode.h"
#include "HavokNode.h"
#include "GhostNode.h"
#include "PathNode.h"
#include "VectorNode.h"
#include "CameraManager.h"
#include "EditorAxis.h"
#include "EditorTerrain.h"
#include "EditorLevel.h"
#include "EditorConfiguration.h"
#include "EditorAnimations.h"
#include "EditorGlobalIllumination.h"
#include "History.h"
#include "EditorMaterial.h"
#include "FBXManager.h"
#include "Shader.h"
#include "UVAnimationLibrary.h"
#include "Havok.h"
#include "HavokPropertyDatabase.h"
#include "Level.h"
#include "Object.h"
#include "ObjectCategory.h"
#include "ObjectSet.h"
#include "PipeClient.h"
#include "TrajectoryNode.h"
#include <SDL.h>
#include <imgui.h>

#ifndef EDITOR_APPLICATION_H_INCLUDED
#define EDITOR_APPLICATION_H_INCLUDED

#define SONICGLVL_CAMERA_NAME                       "EditorCamera"
#define SONICGLVL_CAMERA_PREVIEW_NAME               "PreviewCamera"
#define SONICGLVL_CACHE_PATH                        "../cache/"
#define SONICGLVL_CACHE_DATA_PATH                   "data"
#define SONICGLVL_CACHE_GI_TEMP_PATH                "gi_temp"
#define SONICGLVL_CACHE_TERRAIN_PATH                "terrain"
#define SONICGLVL_CACHE_RESOURCES_PATH              "resources"
#define SONICGLVL_CACHE_SLOT_RESOURCES_PATH         "slot_resources"
#define SONICGLVL_LOW_END_TECHNIQUE                 "LowEnd"
#define SONICGLVL_LEVEL_DATABASE_PATH               "../database/LevelDatabase.xml"
#define SONICGLVL_GHOST_DATABASE_PATH               "../database/GhostDatabase.xml"
#define SONICGLVL_GENERATIONS_OBJECTS_DATABASE_PATH "../database/GenerationsObjectsDatabase.xml"
#define SONICGLVL_UNLEASHED_OBJECTS_DATABASE_PATH   "../database/UnleashedObjectsDatabase.xml"
#define SONICGLVL_HAVOK_PROPERTY_DATABASE_PATH      "../database/HavokPropertyDatabase.xml"
#define SONICGLVL_LIBRARY_PATH                      "../database/objects/"
#define SONICGLVL_RESOURCES_PATH                    "../database/resources/"
#define SONICGLVL_RESOURCES_UNLEASHED_PATH          "../database/objects/Resources_Unleashed/"
#define SONICGLVL_RESOURCES_GENERATIONS_PATH        "../database/objects/Resources_Generations/"
#define SONICGLVL_SHADERS_PATH                      "../database/shaders/"

#define SONICGLVL_FBX_SCENE_NAME               "FBXTerrainImport"
#define SONICGLVL_UNASSIGNED_OBJECT_CATEGORY   "Unassigned"

#define SONICGLVL_CACHE_HASH_FILE              "Hashes.xml"
#define SONICGLVL_CONFIGURATION_FILE           "Configuration.xml"

#define SONICGLVL_SHADER_LIBRARY               editor_application->getShaderLibrary()
#define SONICGLVL_UV_ANIMATION_LIBRARY         editor_application->getUVAnimationLibrary()
#define SONICGLVL_HAVOK_ENVIROMENT             editor_application->getHavokEnviroment()

#define SONICGLVL_HAVOK_PRECISION_FPS          30.0f

#define SONICGLVL_GUI_LEFT_WIDTH               280
#define SONICGLVL_GUI_BOTTOM_HEIGHT            83

#define SONICGLVL_MATERIAL_EDITOR_MODE_MODEL     0
#define SONICGLVL_MATERIAL_EDITOR_MODE_MATERIAL  1
#define SONICGLVL_MATERIAL_EDITOR_MODE_TERRAIN   2

#define SONICGLVL_MULTISETPARAM_MODE_CLONE		 0
#define SONICGLVL_MULTISETPARAM_MODE_MSP		 1
#define SONICGLVL_MULTISETPARAM_MODE_MSP_ADD	 2

extern int global_cursor_state;
extern class EditorApplication *editor_application;

extern Ogre::SceneNode *camera_marker_node;
extern Ogre::SceneNode *camera_marker_tangent;

class ColorListener : public Ogre::RenderTargetListener {
	protected:
		Ogre::SceneManager *scene_manager;
	public:
		ColorListener(Ogre::SceneManager *scene_manager_param) {
			scene_manager = scene_manager_param;
		}

		void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
		void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
};

class DepthListener : public Ogre::RenderTargetListener, public Ogre::RenderQueue::RenderableListener {
	protected:
		Ogre::SceneManager *scene_manager;
		Ogre::RenderQueue* queue;
		Ogre::MaterialPtr mDepthMaterial;
	public:
		DepthListener(Ogre::SceneManager *scene_manager_param) {
			scene_manager = scene_manager_param;

			mDepthMaterial = Ogre::MaterialManager::getSingleton().getByName("DepthMap");
			mDepthMaterial->load();

			Ogre::GpuProgramParametersSharedPtr vp_parameters = mDepthMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters();
			vp_parameters->setTransposeMatrices(true);
		}

		void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
		void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
		bool renderableQueued(Ogre::Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique** ppTech, Ogre::RenderQueue* pQueue);

		Ogre::Pass *getDepthPass() {
			return mDepthMaterial->getTechnique(0)->getPass(0);
		}
};


class EditorApplication : public BaseApplication {
	protected:
		// General Editor Variables
		Ogre::uint32 editor_mode;
		bool world_transform;
		bool local_rotation;
		size_t dragging_mode;
		size_t cloning_mode;
		Ogre::AnimationState *animation_state;
		EditorLevelDatabase *level_database;
		EditorLevel *current_level;
		string current_level_filename;
		string exe_path;
		map<LibGens::ObjectSet *, int> set_indices;
		map<LibGens::ObjectSet *, bool> set_visibility;

		int current_vector_list_selection;
		int last_vector_list_selection;
		int current_id_list_selection;
		int last_id_list_selection;
		bool is_update_vector_list;
		bool is_pick_target;
		bool is_pick_target_position;
		bool is_update_pos_rot;
		bool is_update_look_at_vector;

		// Finder
		list<ObjectNode*>::iterator find_position;
		
		// Ogre
		Ogre::Light *global_directional_light;
		Ogre::SceneManager *axis_scene_manager;

		Ogre::RenderTexture *color_texture;
		Ogre::RenderTexture *depth_texture;

		ColorListener *color_listener;
		DepthListener *depth_listener;
		
		float farPlaneChange;

		// Viewport
		EditorViewport *viewport;
		EditorAxis *axis;

		// Node
		History *history;
		list<EditorNode *> selected_nodes;
		list<EditorNode *> previous_selected_nodes;
		EditorNode *current_node;

		// FBX
		LibGens::FBXManager *fbx_manager;

		// Model
		LibGens::ModelLibrary *model_library;

		// Material
		LibGens::MaterialLibrary *material_library;
		
		// Game-specific resource libraries
		LibGens::MaterialLibrary *unleashed_material_library;
		LibGens::ModelLibrary *unleashed_model_library;
		LibGens::MaterialLibrary *generations_material_library;
		LibGens::ModelLibrary *generations_model_library;

		LibGens::ShaderLibrary *generations_shader_library;
		LibGens::ShaderLibrary *unleashed_shader_library;
		bool checked_shader_library;

		EditorAnimationsList *animations_list;
		LibGens::UVAnimationLibrary *uv_animation_library;

		// Terrain
		TerrainStreamer *terrain_streamer;
		list<TerrainNode *> terrain_nodes_list;
		float terrain_update_counter;

		// GI
		GlobalIlluminationListener *global_illumination_listener;

		// Object
		ObjectNodeManager *object_node_manager;
		LibGens::ObjectLibrary *generations_library;
		LibGens::ObjectLibrary *unleashed_library;
		LibGens::ObjectLibrary *library;
		LibGens::ObjectProduction *object_production;

		// Ghost
		GhostNode *ghost_node;
		unordered_map<string, string> ghost_animation_mappings;
		CameraManager *camera_manager;

		// Animation
		
		// Havok
		LibGens::HavokEnviroment *havok_enviroment;
		LibGens::HavokPropertyDatabase *havok_property_database;
		list<HavokNode *> havok_nodes_list;
		
		// Configuration
		EditorConfiguration *configuration;
		
		// ImGui State
		bool show_left_panel;
		bool show_bottom_panel;
		bool show_properties_editor;
		bool show_material_editor;
		bool show_physics_editor;
		bool show_find_dialog;
		bool show_look_at_dialog;
		bool show_multiset_dialog;
		bool show_terrain_info;
		bool show_quick_overview;
		
		ImVec2 left_panel_size;
		ImVec2 bottom_panel_size;
		
		char find_object_name[256];
		char find_property_name[256];
		char find_property_value[256];
		bool find_match_exactly;
		bool find_with_filter;
		bool find_select_all;
		
		SDL_Cursor* cursor_arrow;
		SDL_Cursor* cursor_hand;
		SDL_Cursor* cursor_sizeall;
		SDL_Cursor* cursor_cross;
		
		float edit_vector_x, edit_vector_y, edit_vector_z;
		size_t edit_id_value;
		float look_at_x, look_at_y, look_at_z;
		int look_at_axis;
		float bottom_pos_x, bottom_pos_y, bottom_pos_z;
		float bottom_rot_x, bottom_rot_y, bottom_rot_z;
		float multiset_vec_x, multiset_vec_y, multiset_vec_z;
		float multiset_spacing;
		int multiset_count;
		
		unsigned int node_visibility_flags;
		bool game_shaders_enabled;
		bool framebuffer_enabled;
		bool uv_animations_enabled;
		bool skybox_enabled;
		
		// Object Palette
		int current_category_index;
		string current_category_search;
		LibGens::Object *last_palette_selection;
		LibGens::Object *current_palette_selection;
		list<ObjectNode *> current_palette_nodes;
		LibGens::ObjectSet *current_set;
		bool palette_cloning_mode;
		vector<LibGens::Object*> palette_search_results;

		// Object Properties
		list<LibGens::Object *> current_object_list_properties;
		vector<string> current_properties_names;
		vector<string> current_properties_values;
		vector<LibGens::ObjectElementType> current_properties_types;
		vector<LibGens::Vector3> temp_property_vector_list;
		vector<unsigned int> temp_property_id_list;
		int current_property_index;
		LibGens::Object *current_single_property_object;
       
		// Cancel Button for Properties Editor
		string backup_property_string;
		int backup_property_int;
		float backup_property_float;
		bool backup_property_bool;
		LibGens::Vector3 backup_property_vector;
		string properties_group_text;
		string help_property_name;
		string help_property_description;
		string target_object_name;

		// ImGui transform panel state
		float transform_pos[3] = {0, 0, 0};
		float transform_rot[3] = {0, 0, 0};
		string help_object_name;
		string help_object_description;

		HistoryActionWrapper *history_edit_property_wrapper;
		int ignore_mouse_clicks_frames;

		vector<TrajectoryNode*> trajectory_preview_nodes;
		vector<VectorNode *> property_vector_nodes;
		History *property_vector_history;
		History* look_at_vector_history;


		// Material Editor
		size_t material_editor_mode;
		string material_editor_mesh_group;
		int material_editor_list_selection;
		int last_material_editor_list_selection;
		int texture_list_selection;
		int last_texture_list_selection;
		LibGens::Model *material_editor_model;
		string material_editor_model_filename;
		string material_editor_library_folder;
		vector<LibGens::Material *> material_editor_materials;
		// ImGui state for Material Editor
		bool material_editor_defaults_on_shader_change;
		vector<string> material_editor_shader_names;
		vector<string> material_editor_slot_names;
		char material_name_buf[128];
		char texture_filename_buf[256];
		char texture_unit_name_buf[128];
		int texture_slot_index;
		char material_param_name_buf[10][64];
		float material_param_rgba[10][4];
		Ogre::RenderWindow *material_editor_preview_window;
		Ogre::Viewport *material_editor_preview_viewport;
		Ogre::Camera *material_editor_preview_camera;
		Ogre::SceneManager *material_editor_preview_scene_manager;
		Ogre::SceneManager *material_editor_preview_bogus_scene_manager;
		LibGens::MaterialLibrary *material_editor_material_library;
		LibGens::Material *material_editor_material;
		LibGens::Texture *material_editor_texture;
		string material_editor_skeleton_name;
		string material_editor_animation_name;
		Ogre::AnimationState *material_editor_animation_state;
		Ogre::SceneNode *material_editor_scene_node;

		OIS::InputManager* material_editor_input_manager;
		OIS::Mouse*    material_editor_mouse;
		OIS::Keyboard* material_editor_keyboard;

		MaterialEditorPreviewListener *material_editor_preview_listener;
		EditorViewport *material_editor_viewport;

		LibGens::ShaderLibrary* material_editor_shader_library;
		bool material_editor_unleashed;

		// Object Movement
		float placement_grid_snap;

		// Cloning
		list<EditorNode*> cloning_nodes;
		list<EditorNode*> temporary_nodes;

		// Game
		PipeClient* game_client;

		// Ghost
		LibGens::Ghost* ghost_data;
		bool isGhostRecording;

		// LookAt feature
		VectorNode* vector_node;

	public:
		EditorApplication(void);
		virtual ~EditorApplication(void);

		void createTerrainStreamer();
		void *updateTerrainStreamer();

		bool keyPressed(const OIS::KeyEvent &arg);
		bool keyReleased(const OIS::KeyEvent &arg);
		bool mouseMoved(const OIS::MouseEvent &arg);
		bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		bool frameRenderingQueued(const Ogre::FrameEvent& evt);
		void windowResized(Ogre::RenderWindow* rw);
		void createScene(void);

		// Editor Node Methods		
		void updateSelection();
		void deleteSelection();
		void clearSelection();
		void cloneSelection();
		void temporaryCloneSelection();
		void showSelectionNames();
		void selectAll();
		void translateSelection(Ogre::Vector3 v);
		void rotateSelection(Ogre::Quaternion q);
		void setSelectionRotation(Ogre::Quaternion q);
		void rememberSelection(bool mode);
		void makeHistorySelection(bool mode);
		void toggleWorldTransform();
		void togglePlacementSnap();
		void toggleLocalRotation();
		void toggleRotationSnap();
		void lookAt(EditorNode*, int, Ogre::Vector3);
		void lookAtPoint(int, Ogre::Vector3);
		void lookAtEachOther(int);
		void snapToClosestPath();
		void updateNodeVisibility();
		void toggleNodeVisibility(unsigned int flag);
		void updateVisibilityGUI();
		void rememberCloningNodes();
		list<EditorNode*> getSelectedNodes();
		ObjectNode* getObjectNodeFromEditorNode(EditorNode* node);
		TrajectoryMode getTrajectoryMode(EditorNode* node);
		void addTrajectory(TrajectoryMode mode);
		void removeAllTrajectoryNodes();
		bool isUpdatePosRot();

		void openFindGUI();
		void closeFindGUI();
		void findNext(string obj_name, string param, string value);
		void findAll(string obj_name, string param, string value);

		void copySelection();
		void pasteSelection();

		void undoHistory();
		void redoHistory();
		void pushHistory(HistoryAction *action);

		// Update methods
		void checkGhost(Ogre::Real timeSinceLastFrame);
		void checkTerrainStreamer();
		void checkTerrainVisibilityAndQuality(Ogre::Real timeSinceLastFrame);
		void updateTrajectoryNodes(Ogre::Real timeSinceLastFrame);

		void ignoreMouseClicks(int frames) {
			ignore_mouse_clicks_frames = frames;
		}

		bool ignoringMouseClicks() {
			return (ignore_mouse_clicks_frames > 0);
		}

		// GUI Methods
		void focus();
		bool inFocus();
		void updateCursor();
		void initializeImGui();
		void shutdownImGui();
		void renderImGui();
		void renderImGuiContent();
		void renderLeftPanel();
		void renderBottomPanel();
		void renderPropertyEditor();
		void renderMaterialEditor();
		void renderRightPanel();
		void handleImGuiEvent(SDL_Event* event);
		void renderMainMenuBar();
		void renderQuickOverviewDialog();
		void renderFindDialog();
		void renderLookAtDialog();
		void renderMultiSetDialog();
		void renderTerrainInfoDialog();
		void handleSDLEvent(const SDL_Event& event);
		
		void setEditorMode(Ogre::uint32 v) {
			editor_mode = v;
		}

		Ogre::uint32 getEditorMode() {
			return editor_mode;
		}
		
		void updateObjectCategoriesGUI();
		void updateObjectsPaletteGUI(int index = 0);
		void searchObjectsPalette(string search_text = "");
		void updateHelpWithObjectGUI(LibGens::Object* object);
		void updateObjectsPaletteSelection(int index);
		void updateObjectsPalettePreview();
		void overrideObjectsPalettePreview(list<LibGens::Object*> override_objects);
		void mouseMovedObjectsPalettePreview(const OIS::MouseEvent &arg);
		void mousePressedObjectsPalettePreview(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		
		void openLevelGUI();
		void openLostWorldLevelGUI();
		void saveLevelDataGUI();
		void saveLevelResourcesGUI();
		void saveLevelTerrainGUI();
		void exportSceneFBXGUI();
		void exportSceneFBX(string filename);
		void importLevelTerrainFBXGUI();
		void loadAllTerrain();
		void openTerrainInfoDialog();
		void updateTerrainInfoDialog();
		void closeTerrainInfoDialog();
		
		void convertMaterialsToUnleashed();
		void convertMaterialsToUnleashedShaders();
		void convertMaterialsToGenerations();
		void convertMaterialsToLostWorld();
		void openMaterialEditorGUI();
		
		// Material Editor Methods
		void closePreviewMaterialEditorGUI();
		void createPreviewMaterialEditorGUI();
		void enableMaterialEditorGUI(bool enable);
		void enableMaterialEditorListGUI();
		void updateMaterialEditorTextureList();
		void updateMaterialTextureInfo();
		void updateMaterialEditorInfo();
		void loadMaterialDefaultParams();
		void removeMaterialEditorTexture();
		void materialEditorTerrainMode();
		void materialEditorModelMode();
		void clearMaterialEditorGUI();
		void cleanMaterialEditorModelGUI();
		void clearSelectionMaterialEditorGUI();
		void clearTextureInfo();
		void rebuildMaterialPreviewNodes();
		void rebuildListMaterialEditorGUI();
		void saveMaterialEditorModelGUI();
		void saveMaterialEditorMaterial();
		void saveAllMaterialEditorMaterials();
		void loadMaterialEditorModelGUI();
		void copyMaterialEditorTexture(const string& file) const;
		void pickMaterialEditorTextureGUI();
		void addMaterialEditorTextureGUI();
		void loadMaterialEditorSkeletonGUI();
		void loadMaterialEditorAnimationGUI();
		void updateMaterialEditorIndex(int selection_index);
		void updateMaterialEditorTextureIndex(int selection_index);
		void updateEditParameterMaterialEditor(size_t i, LibGens::Color parameter_color);
		void updateEditShaderMaterialEditor(string shader_name);
		void updateEditTextureMaterialEditor(string texture_name, bool update_ui = false);
		void updateEditTextureUnitMaterialEditor(string unit_name);

		void reloadTemplatesDatabase();
		void saveTemplatesDatabase();
		
		bool checkGameConnection();
		bool connectGame();
		void launchGame();
		void sendMessageGame(int msg);
		
		void setupGhost();
		void loadGhostRecording();
		void saveGhostRecording();
		void saveGhostRecordingFbx();
		void loadGhostAnimations();
		
		// Physics
		void openPhysicsEditorGUI();
		void addPhysicsEditorEntryGUI(LibGens::LevelCollisionEntry *entry);
		void importPhysicsEditorGUI();
		void detectAndTagHavokPhysics(LibGens::HavokPhysicsCache *physics_cache);
		void clearPhysicsEditorGUI();
		void renderPhysicsEditor();
		
		// Path 
		void loadLevelPaths();
		
		// Sets GUI
		void updateBottomSelectionGUI();
		void updateMenu();
		void updateSetsGUI();
		void updateSelectedSetGUI();
		void newCurrentSet();
		void deleteCurrentSet();
		void updateCurrentSetVisible(bool v);
		void changeCurrentSet(string change_set);
		void renameCurrentSet(string rename_set);
		
		// Object property 
		void closeVectorQueryMode();
		void closeEditPropertyGUI();
		void clearObjectsPalettePreview();
		void clearObjectsPalettePreviewGUI();
		void updateObjectPropertyIndex(int index);
		void editObjectPropertyIndex(int index);
		void createObjectsPropertiesGUI();
		void updateObjectsPropertiesGUI();
		void updateObjectsPropertiesValuesGUI(LibGens::Object *object);
		void updateHelpWithPropertyGUI(LibGens::ObjectElement *element);
		void updateEditPropertyBool(bool v);
		void updateEditPropertyInteger(unsigned int v);
		void updateEditPropertyFloat(float v);
		void updateEditPropertyString(string v);
		void updateEditPropertyID(size_t v);
		void updateEditPropertyIDList(vector<size_t> v);
		void updateEditPropertyVector(LibGens::Vector3 v);
		void updateEditPropertyVectorFocus(int index = 0);
		void updateEditPropertyVectorGUI(int index = 0, bool is_list = false);
		void updateEditPropertyVectorMode(bool mode_state, bool is_list = false, int index = 0);
		void updateEditPropertyVectorList(vector<LibGens::Vector3> v);
		void updateEditPropertyVectorListGUI(vector<LibGens::Vector3> v);
		void selectNode(EditorNode* node);
		void openQueryTargetMode(bool mode);
		void setTargetName(size_t id, bool is_list = false);
		void addVectorToList(LibGens::Vector3 = LibGens::Vector3(0, 0, 0));
		void updateVectorListSelection(int index);
		void removeVectorFromList(int index);
		void moveVector(int index, bool up);
		bool isVectorListSelectionValid();
		bool isUpdateVectorList();
		void addIDToList(size_t id);
		void updateIDListSelection(int index);
		void removeIDFromList(int index);
		void moveID(int index, bool up);
		bool isIDListSelectionValid();
		vector<size_t>& getCurrentPropertyIDList();
		vector<LibGens::Vector3>& getCurrentPropertyVectorList();
		vector<VectorNode*>& getPropertyVectorNodes();
		void closeTargetQueryMode();
		void verifySonicSpawnChange();
		void confirmEditProperty();
		void revertEditProperty();
		
		void openLookAtPointGUI();
		void closeLookAtPointGUI();
		void updateLookAtPointVectorNode(Ogre::Vector3);
		void focusLookAtPointVector();
		void queryLookAtObject(bool);
		void updateLookAtVectorMode(bool);
		void updateLookAtVectorGUI();
		bool isUpdateLookAtVector();
		
		void openMultiSetParamDlg();
		void closeMultiSetParamDlg();
		void clearMultiSetParamDlg();
		void createMultiSetParamObjects();
		void getVectorFromObject();
		void setCloningMode(size_t mode);
		void setVectorAndSpacing();
		void deleteTemporaryNodes();

		bool isPalettePreviewActive();
		bool isRegularMode();

		// Accessor methods
		LibGens::ShaderLibrary* getShaderLibrary();
		LibGens::UVAnimationLibrary* getUVAnimationLibrary();
		LibGens::HavokEnviroment* getHavokEnviroment();
		EditorLevel* getCurrentLevel();
		EditorAnimationsList* getAnimationsList();
		ObjectNodeManager* getObjectNodeManager();
		Ogre::SceneManager* getSceneManager();
		void checkShaderLibrary(size_t game_mode);
		GhostNode* getGhostNode();
		
		void setGhost(LibGens::Ghost* ghost_p) {
			if (!ghost_p)
				return;
			
			if (ghost_data != ghost_p && ghost_data)
				delete ghost_data;

			ghost_data = ghost_p;
			setupGhost();
			ghost_node->setGhost(ghost_p);
		}

		EditorAxis *getEditorAxis() {
			return axis;
		}

		LibGens::ObjectSet *getCurrentSet() {
			return current_set;
		}

		EditorConfiguration *getConfiguration() {
			return configuration;
		}

		void updateBottomSelectionPosition(float value_x, float value_y, float value_z);
		void updateBottomSelectionRotation(float value_x, float value_y, float value_z);

		// LibGens Methods
		void openLevel(string filename);
		void openLostWorldLevel(string filename);

		void createDirectionalLight(LibGens::Light *direct_light);
		void createSkybox(string skybox_name);

		void saveLevelData(string filename);
		void saveLevelResources();
		void saveLevelTerrain();
		void cleanLevelTerrain();
		void importLevelTerrainFBX(string filename);
		void generateTerrainGroups();

		void createLevel(string name);
		void createLibrary();
		void createCategory(LibGens::ObjectCategory *category, string folder);
		void createNodesFromSet(LibGens::ObjectSet *set);
		void createNodesFromTerrain(LibGens::Terrain *terrain, LibGens::GITextureGroupInfo *gi_group_info);
		void createNodesFromTerrainGroup(LibGens::TerrainGroup *terrain_group);
		void createNodesFromHavokEnviroment(LibGens::HavokEnviroment *havok_enviroment);

		void processGameMessage(PipeClient* client, PipeMessage* msg);
		DWORD sendMessageGame(const PipeMessage& msg, size_t size);
		
		static std::string SelectFolderWithIFileDialog(const wchar_t* title = L"Select Folder");
};

#endif