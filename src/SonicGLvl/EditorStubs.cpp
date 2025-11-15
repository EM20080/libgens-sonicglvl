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

// This is where my Stubs and unfinished ImGui conversions will go, if i do end up finishing it i will keep it here

#include "EditorApplication.h"

void EditorApplication::updateObjectsPropertiesGUI(void) {
	createObjectsPropertiesGUI();
}

void EditorApplication::updateEditPropertyVectorGUI(int index, bool is_list) {
	if (is_list && index >= 0 && index < (int)temp_property_vector_list.size()) {
		edit_vector_x = temp_property_vector_list[index].x;
		edit_vector_y = temp_property_vector_list[index].y;
		edit_vector_z = temp_property_vector_list[index].z;
	}
}

void EditorApplication::updateEditPropertyVectorListGUI(std::vector<LibGens::Vector3> v) {
	temp_property_vector_list = v;
}

bool EditorApplication::isVectorListSelectionValid(void) {
	return (current_vector_list_selection >= 0 &&
		current_vector_list_selection < (int)temp_property_vector_list.size());
}

void EditorApplication::createObjectsPropertiesGUI(void) {
	current_object_list_properties.clear();
	current_properties_names.clear();
	current_properties_values.clear();
	current_properties_types.clear();
	current_property_index = -1;

	for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
		ObjectNode* object_node = getObjectNodeFromEditorNode(*it);
		if (object_node) {
			LibGens::Object* object = object_node->getObject();
			if (object) {
				current_object_list_properties.push_back(object);
			}
		}
	}

	if (!current_object_list_properties.empty()) {
		LibGens::Object* first_object = current_object_list_properties.front();
		list<LibGens::ObjectElement*> elements = first_object->getElements();

		for (list<LibGens::ObjectElement*>::iterator it = elements.begin(); it != elements.end(); it++) {
			current_properties_names.push_back((*it)->getName());
			current_properties_types.push_back((*it)->getType());
			current_properties_values.push_back("");
		}
	}
}

void EditorApplication::closeVectorQueryMode(void) {
	for (size_t i = 0; i < property_vector_nodes.size(); i++) {
		delete property_vector_nodes[i];
	}
	property_vector_nodes.clear();
}

void EditorApplication::openQueryTargetMode(bool is_list) {
	is_pick_target = true;
	is_pick_target_position = is_list;
}

void EditorApplication::setTargetName(unsigned int id, bool is_list) {
	if (current_property_index >= 0 && current_property_index < (int)current_properties_names.size()) {
		if (is_list) {
			temp_property_id_list.push_back(id);
		}
		else {
			for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
				it != current_object_list_properties.end(); it++) {
				LibGens::ObjectElementTarget* element =
					(LibGens::ObjectElementTarget*)(*it)->getElement(current_properties_names[current_property_index]);
				if (element) {
					element->value = id;
				}
			}
		}
	}
}


void EditorApplication::renderPropertyEditor(void) {
	static bool temp_bool = false;
	static int temp_int = 0;
	static float temp_float = 0.0f;
	static char temp_string[512] = "";
	static float temp_vector[3] = { 0, 0, 0 };
	static bool initialized = false;
	
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Edit Property", &show_properties_editor, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (!current_object_list_properties.empty() && current_property_index >= 0) {
			string property_name = current_properties_names[current_property_index];
			LibGens::ObjectElementType property_type = current_properties_types[current_property_index];

			ImGui::Text("Editing: %s", property_name.c_str());
			ImGui::Separator();
			ImGui::Spacing();

			LibGens::Object* first_obj = current_object_list_properties.front();
			LibGens::ObjectElement* element = first_obj->getElement(property_name);

			if (element) {
				if (property_type == LibGens::OBJECT_ELEMENT_STRING && property_name == "Type" && object_production) {
					LibGens::ObjectElementString* str_elem = (LibGens::ObjectElementString*)element;
					if (!initialized) {
						strncpy(temp_string, str_elem->value.c_str(), sizeof(temp_string));
						temp_string[sizeof(temp_string) - 1] = '\0';
						initialized = true;
					}

					ImGui::Text("Current: %s", str_elem->value.c_str());
					ImGui::Spacing();
					ImGui::Text("New Value:");
					if (ImGui::BeginCombo("##ObjectPhysicsCombo", temp_string)) {
						ImGui::SetKeyboardFocusHere();
						ImGui::InputText("##ObjectPhysicsFilter", temp_string, sizeof(temp_string));
						object_production->readySortedEntries();
						string entry_name;
						size_t filter_len = strlen(temp_string);
						while (object_production->getNextEntryName(entry_name)) {
							if (filter_len == 0 || strncmp(temp_string, entry_name.c_str(), filter_len) == 0 || 
								entry_name.find(temp_string) != string::npos) {
								bool is_selected = (strcmp(temp_string, entry_name.c_str()) == 0);
								if (ImGui::Selectable(entry_name.c_str(), is_selected)) {
									strncpy(temp_string, entry_name.c_str(), sizeof(temp_string));
									temp_string[sizeof(temp_string) - 1] = '\0';
									ImGui::CloseCurrentPopup();
								}
								if (is_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
						}
						ImGui::EndCombo();
					}
				}
				else {
					switch (property_type) {
					case LibGens::OBJECT_ELEMENT_BOOL: {
						LibGens::ObjectElementBool* bool_elem = (LibGens::ObjectElementBool*)element;
						if (!initialized) {
							temp_bool = bool_elem->value;
							backup_property_bool = bool_elem->value;
							initialized = true;
						}
						ImGui::Text("Current: %s", bool_elem->value ? "true" : "false");
						ImGui::Spacing();
						ImGui::Checkbox("New Value", &temp_bool);
						break;
					}
					case LibGens::OBJECT_ELEMENT_INTEGER: {
						LibGens::ObjectElementInteger* int_elem = (LibGens::ObjectElementInteger*)element;
						if (!initialized) {
							temp_int = int_elem->value;
							backup_property_int = int_elem->value;
							initialized = true;
						}
						ImGui::Text("Current: %d", int_elem->value);
						ImGui::Spacing();
						ImGui::InputInt("New Number", &temp_int, 1, 10);
						break;
					}
					case LibGens::OBJECT_ELEMENT_FLOAT: {
						LibGens::ObjectElementFloat* float_elem = (LibGens::ObjectElementFloat*)element;
						if (!initialized) {
							temp_float = float_elem->value;
							backup_property_float = float_elem->value;
							initialized = true;
						}
						ImGui::Text("Current: %.3f", float_elem->value);
						ImGui::Spacing();
						ImGui::InputFloat("New Number", &temp_float, 0.1f, 1.0f, "%.3f");
						break;
					}
					case LibGens::OBJECT_ELEMENT_STRING: {
						LibGens::ObjectElementString* str_elem = (LibGens::ObjectElementString*)element;
						if (!initialized) {
							strncpy(temp_string, str_elem->value.c_str(), sizeof(temp_string));
							temp_string[sizeof(temp_string) - 1] = '\0';
							backup_property_string = str_elem->value;
							initialized = true;
						}
						ImGui::Text("Current: %s", str_elem->value.c_str());
						ImGui::Spacing();
						ImGui::InputText("New Text", temp_string, sizeof(temp_string));
						break;
					}
					case LibGens::OBJECT_ELEMENT_VECTOR: {
						LibGens::ObjectElementVector* vec_elem = (LibGens::ObjectElementVector*)element;
						if (!initialized) {
							temp_vector[0] = vec_elem->value.x;
							temp_vector[1] = vec_elem->value.y;
							temp_vector[2] = vec_elem->value.z;
							backup_property_vector = vec_elem->value;
							initialized = true;
						}
						ImGui::Text("Current: (%.2f, %.2f, %.2f)", vec_elem->value.x, vec_elem->value.y, vec_elem->value.z);
						ImGui::Spacing();
						ImGui::InputFloat3("New Value", temp_vector, "%.2f");
						break;
					}
					}
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				if (ImGui::Button("Save", ImVec2(100, 0))) {
					switch (property_type) {
					case LibGens::OBJECT_ELEMENT_BOOL:
						updateEditPropertyBool(temp_bool);
						break;
					case LibGens::OBJECT_ELEMENT_INTEGER:
						updateEditPropertyInteger(temp_int);
						break;
					case LibGens::OBJECT_ELEMENT_FLOAT:
						updateEditPropertyFloat(temp_float);
						break;
					case LibGens::OBJECT_ELEMENT_STRING:
						updateEditPropertyString(string(temp_string));
						break;
					case LibGens::OBJECT_ELEMENT_VECTOR:
						updateEditPropertyVector(LibGens::Vector3(temp_vector[0], temp_vector[1], temp_vector[2]));
						break;
					}
					initialized = false;
					show_properties_editor = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(100, 0))) {
					// Just close without applying changes
					initialized = false;
					show_properties_editor = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Close", ImVec2(100, 0))) {
					initialized = false;
					show_properties_editor = false;
				}
			}
		}
	} else {
		initialized = false;
	}
	ImGui::End();
}

void EditorApplication::renderTerrainInfoDialog(void) {
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Terrain Information", &show_terrain_info, ImGuiWindowFlags_NoResize)) {
		if (current_level) {
			ImGui::Text("Terrain Instances: %d", (int)terrain_nodes_list.size());
			ImGui::Separator();

			int index = 0;
			for (list<TerrainNode*>::iterator it = terrain_nodes_list.begin();
				it != terrain_nodes_list.end(); it++) {
				LibGens::TerrainInstance* instance = (*it)->getTerrainInstance();
				if (instance) {
					ImGui::Text("Terrain %d: %s", index++, instance->getName().c_str());
				}
			}
		}
	}
	ImGui::End();
}


void EditorApplication::renderImGuiContent(void) {
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

	if (show_material_editor && material_editor_preview_window && !material_editor_preview_window->isClosed()) {
		material_editor_preview_window->update();
	}
}

// Initialization - call base class implementations
void EditorApplication::initializeImGui(void) {
	BaseApplication::initializeImGui();
}

void EditorApplication::shutdownImGui(void) {
	BaseApplication::shutdownImGui();
}

Ogre::SceneManager* EditorApplication::getSceneManager(void) {
	return scene_manager;
}

GhostNode* EditorApplication::getGhostNode(void) {
	return ghost_node;
}

// getCurrentLevel already defined

LibGens::HavokEnviroment* EditorApplication::getHavokEnviroment(void) {
	return havok_enviroment;
}

EditorAnimationsList* EditorApplication::getAnimationsList(void) {
	return animations_list;
}

LibGens::ShaderLibrary* EditorApplication::getShaderLibrary(void) {
	if (checked_shader_library) {
		if (current_level && current_level->getGameMode() == LIBGENS_LEVEL_GAME_UNLEASHED) {
			return unleashed_shader_library;
		}
		return generations_shader_library;
	}
	return nullptr;
}

LibGens::UVAnimationLibrary* EditorApplication::getUVAnimationLibrary(void) {
	return uv_animation_library;
}

void EditorApplication::updateEditPropertyVectorList(std::vector<LibGens::Vector3> v) {
	temp_property_vector_list = v;
	updateEditPropertyVectorListGUI(v);
}

EditorLevel* EditorApplication::getCurrentLevel(void) {
	return current_level;
}

void EditorApplication::closeEditPropertyGUI(void) {
	show_properties_editor = false;
	closeVectorQueryMode();
}

bool EditorApplication::checkGameConnection(void) {
	// TODO: Dont really feel like having it connect to Generations if no one is going to use it, i might remove it.
	return false;
}

void EditorApplication::updateEditPropertyBool(bool v) {
	if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();

	for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
		it != current_object_list_properties.end(); it++) {
		LibGens::ObjectElementBool* element =
			(LibGens::ObjectElementBool*)(*it)->getElement(current_properties_names[current_property_index]);
		if (element) {
			element->value = v;
			object_node_manager->reloadObjectNode(*it);
		}
	}
}

void EditorApplication::updateEditPropertyInteger(unsigned int v) {
	if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();

	for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
		it != current_object_list_properties.end(); it++) {
		LibGens::ObjectElementInteger* element =
			(LibGens::ObjectElementInteger*)(*it)->getElement(current_properties_names[current_property_index]);
		if (element) {
			element->value = v;
			object_node_manager->reloadObjectNode(*it);
		}
	}
}

void EditorApplication::updateEditPropertyFloat(float v) {
	if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();

	for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
		it != current_object_list_properties.end(); it++) {
		LibGens::ObjectElementFloat* element =
			(LibGens::ObjectElementFloat*)(*it)->getElement(current_properties_names[current_property_index]);
		if (element) {
			element->value = v;
			object_node_manager->reloadObjectNode(*it);
		}
	}
}

void EditorApplication::updateEditPropertyString(string v) {
	if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();

	for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
		it != current_object_list_properties.end(); it++) {
		LibGens::ObjectElementString* element =
			(LibGens::ObjectElementString*)(*it)->getElement(current_properties_names[current_property_index]);
		if (element) {
			element->value = v;
			object_node_manager->reloadObjectNode(*it);
		}
	}
}

void EditorApplication::updateEditPropertyVector(LibGens::Vector3 v) {
	if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();

	for (list<LibGens::Object*>::iterator it = current_object_list_properties.begin();
		it != current_object_list_properties.end(); it++) {
		LibGens::ObjectElementVector* element =
			(LibGens::ObjectElementVector*)(*it)->getElement(current_properties_names[current_property_index]);
		if (element) {
			element->value = v;
			object_node_manager->reloadObjectNode(*it);
		}
	}
}

void EditorApplication::addVectorToList(LibGens::Vector3 v) {
	temp_property_vector_list.push_back(v);
	updateEditPropertyVectorListGUI(temp_property_vector_list);
}
