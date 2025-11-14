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
#include "ObjectLibrary.h"
#include "ObjectSet.h"

void EditorApplication::updateObjectCategoriesGUI() {
}


void EditorApplication::updateObjectsPaletteGUI(int index) {
	if (!library) return;

	current_category_index = index;
	current_category_search = "";
	palette_search_results.clear();
}

void EditorApplication::searchObjectsPalette(string search_name) {
	if (!library) return;
	if (current_category_search == search_name) return;

	current_category_search = search_name;
	if (search_name.empty())
	{
		updateObjectsPaletteGUI(current_category_index);
		return;
	}

	palette_search_results.clear();

	for (auto object_category : library->getCategories())
	{
		for (auto object : object_category->getTemplates())
		{
			string object_name = object->getName();
			string search_lower = search_name;
			std::transform(object_name.begin(), object_name.end(), object_name.begin(), [](unsigned char c) { return std::tolower(c); });
			std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), [](unsigned char c) { return std::tolower(c); });

			if (object_name.find(search_lower) != string::npos)
			{
				palette_search_results.push_back(object);
			}
		}
	}

	sort(palette_search_results.begin(), palette_search_results.end(), 
		[](LibGens::Object* a, LibGens::Object* b) { return a->getName() < b->getName(); });
}


void EditorApplication::updateHelpWithObjectGUI(LibGens::Object *object) {
}

void EditorApplication::updateObjectsPaletteSelection(int index) {
	if (!library) return;

	if (index < 0) {
		current_palette_selection = NULL;
		return;
	}

	if (!current_category_search.empty()) {
		if (index >= 0 && index < (int)palette_search_results.size()) {
			current_palette_selection = palette_search_results[index];
		}
		return;
	}

	LibGens::ObjectCategory *object_category=library->getCategoryByIndex(current_category_index);
	if (object_category) {
		LibGens::Object *target_selection=object_category->getTemplateByIndex(index);
		current_palette_selection = target_selection;
	}
}

void EditorApplication::updateObjectsPalettePreview() {
	if (current_palette_selection != last_palette_selection) {
		closeVectorQueryMode();
		closeEditPropertyGUI();
		clearObjectsPalettePreview();

		if (current_palette_selection) {
			current_palette_selection->setPosition(LibGens::Vector3(LIBGENS_AABB_MAX_START, LIBGENS_AABB_MAX_START, LIBGENS_AABB_MAX_START));

			palette_cloning_mode = true;

			ObjectNode *palette_node=new ObjectNode(current_palette_selection, scene_manager, model_library, material_library, object_production, object_node_manager->getSlotIDName());
			current_palette_nodes.push_back(palette_node);
		}
		last_palette_selection = current_palette_selection;

		updateHelpWithObjectGUI(current_palette_selection);
	}
}

void EditorApplication::overrideObjectsPalettePreview(list<LibGens::Object *> override_objects) {
	closeVectorQueryMode();
	closeEditPropertyGUI();
	clearObjectsPalettePreview();

	current_palette_selection = NULL;
	last_palette_selection = NULL;

	palette_cloning_mode = false;

	for (list<LibGens::Object *>::iterator it=override_objects.begin(); it!=override_objects.end(); it++) {
		ObjectNode *palette_node=new ObjectNode((*it), scene_manager, model_library, material_library, object_production, object_node_manager->getSlotIDName());
		current_palette_nodes.push_back(palette_node);
	}
}


void EditorApplication::mouseMovedObjectsPalettePreview(const OIS::MouseEvent &arg) {
	float mouse_x=arg.state.X.abs/float(arg.state.width);
	float mouse_y=arg.state.Y.abs/float(arg.state.height);
	viewport->convertMouseToLocalScreen(mouse_x, mouse_y);

	// Raycast from camera to viewport
	Ogre::Vector3 raycast_point(0.0f);
	viewport->raycastPlacement(mouse_x, mouse_y, 15.0f, &raycast_point, EDITOR_NODE_QUERY_TERRAIN | EDITOR_NODE_QUERY_HAVOK);

	if (placement_grid_snap > 0.0f) {
		float half_placement_grid_snap = placement_grid_snap/2.0f;

		float grid_offset = fmod(raycast_point.x, placement_grid_snap);
		if (grid_offset < half_placement_grid_snap) raycast_point.x -= grid_offset;
		else raycast_point.x += placement_grid_snap - grid_offset;

		grid_offset = fmod(raycast_point.y, placement_grid_snap);
		if (grid_offset < half_placement_grid_snap) raycast_point.y -= grid_offset;
		else raycast_point.y += placement_grid_snap - grid_offset;

		grid_offset = fmod(raycast_point.z, placement_grid_snap);
		if (grid_offset < half_placement_grid_snap) raycast_point.z -= grid_offset;
		else raycast_point.z += placement_grid_snap - grid_offset;
	}

	Ogre::Vector3 center=Ogre::Vector3::ZERO;
	for (list<ObjectNode *>::iterator it=current_palette_nodes.begin(); it!=current_palette_nodes.end(); it++) {
		center += (*it)->getPosition();
	}
	center /= current_palette_nodes.size();

	Ogre::Vector3 translate = raycast_point - center;
	for (list<ObjectNode *>::iterator it=current_palette_nodes.begin(); it!=current_palette_nodes.end(); it++) {
		(*it)->translate(translate);
	}
}


void EditorApplication::mousePressedObjectsPalettePreview(const OIS::MouseEvent &arg, OIS::MouseButtonID id) {
	if (id == OIS::MB_Left) {
		mouseMovedObjectsPalettePreview(arg);
		clearSelection();

		HistoryActionWrapper *wrapper = new HistoryActionWrapper();
		for (list<ObjectNode *>::iterator it=current_palette_nodes.begin(); it!=current_palette_nodes.end(); it++) {
			LibGens::Object *object_from_preview = (*it)->getObject();

			if (object_from_preview) {
				LibGens::Object *new_object = new LibGens::Object(object_from_preview);

				if (current_level) {
					if (current_level->getLevel()) {
						new_object->setID(current_level->getLevel()->newObjectID());
					}
				}
			
				if (current_set) {
					current_set->addObject(new_object);

					if (!current_level) {
						new_object->setID(current_set->newObjectID());
					}
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
		pushHistory(wrapper);

		updateSelection();

		if (!keyboard->isModifierDown(OIS::Keyboard::Ctrl) && !keyboard->isModifierDown(OIS::Keyboard::Shift)) {
			clearObjectsPalettePreviewGUI();
		}
	}

	if (id == OIS::MB_Right) {
		clearObjectsPalettePreviewGUI();
	}
}


void EditorApplication::clearObjectsPalettePreview() {
	for (list<ObjectNode *>::iterator it=current_palette_nodes.begin(); it!=current_palette_nodes.end(); it++) {
		if (!palette_cloning_mode) {
			LibGens::Object *object=(*it)->getObject();
			if (object) {
				delete object;
			}
		}

		delete (*it);
	}
	current_palette_nodes.clear();
}

void EditorApplication::clearObjectsPalettePreviewGUI() {
	clearObjectsPalettePreview();
	current_palette_selection = NULL;
	current_object_list_properties.clear();
	updateSelection();
}

bool EditorApplication::isPalettePreviewActive() {
	return (current_palette_nodes.size() > 0);
}

bool EditorApplication::isRegularMode() {
	bool regular_mode = true;
	if (isPalettePreviewActive())              regular_mode = false;
	if (editor_mode == EDITOR_NODE_QUERY_NODE) regular_mode = false;
	return regular_mode;
}

void EditorApplication::renderLeftPanel() {
	float menu_bar_height = ImGui::GetFrameHeight();
	float panel_height = (float)screen_height - menu_bar_height;
	if (show_bottom_panel) {
		panel_height -= 99.0f;
	}
	
	ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(SONICGLVL_GUI_LEFT_WIDTH + 1, panel_height), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.95f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	if (ImGui::Begin("Objects & Properties", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
		if (ImGui::CollapsingHeader("Object Palette", ImGuiTreeNodeFlags_DefaultOpen)) {
			static char search_buffer[256] = "";
			if (ImGui::InputTextWithHint("##Search", "Search objects...", search_buffer, 256)) {
				searchObjectsPalette(search_buffer);
			}
            
			if (library) {
				vector<LibGens::ObjectCategory*> categories = library->getCategories();
				if (!categories.empty()) {
					const char* current_category_name = current_category_search.empty() ? 
						categories[current_category_index]->getName().c_str() : "Search Results";
                    
					if (ImGui::BeginCombo("Category", current_category_name)) {
						for (size_t i = 0; i < categories.size(); i++) {
							bool is_selected = (current_category_index == i && current_category_search.empty());
							if (ImGui::Selectable(categories[i]->getName().c_str(), is_selected)) {
								search_buffer[0] = '\0';
								updateObjectsPaletteGUI((int)i);
							}
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
                    
					if (ImGui::BeginListBox("##PaletteList", ImVec2(-FLT_MIN, 200))) {
						if (!current_category_search.empty() && !palette_search_results.empty()) {
							for (size_t i = 0; i < palette_search_results.size(); i++) {
								bool is_selected = (current_palette_selection == palette_search_results[i]);
								if (ImGui::Selectable(palette_search_results[i]->getName().c_str(), is_selected)) {
									current_palette_selection = palette_search_results[i];
									updateObjectsPalettePreview();
								}
								if (is_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
						} else {
							LibGens::ObjectCategory* object_category = library->getCategoryByIndex(current_category_index);
							if (object_category) {
								vector<LibGens::Object*> objects = object_category->getTemplates();
								for (size_t i = 0; i < objects.size(); i++) {
									bool is_selected = (current_palette_selection == objects[i]);
									if (ImGui::Selectable(objects[i]->getName().c_str(), is_selected)) {
										updateObjectsPaletteSelection((int)i);
										updateObjectsPalettePreview();
									}
									if (is_selected) {
										ImGui::SetItemDefaultFocus();
									}
								}
							}
						}
						ImGui::EndListBox();
					}
				}
			}
            
			ImGui::Checkbox("Cloning Mode", &palette_cloning_mode); // This is gonna be where if you place an object, put params, then place the same object again it will copy parameters off the other.
		}
        
		if (ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			float item_height = ImGui::GetTextLineHeightWithSpacing();
			float list_height = std::min((float)current_properties_names.size() * item_height + 4.0f, 200.0f);
			if (ImGui::BeginListBox("##PropertiesList", ImVec2(-FLT_MIN, list_height))) {
				for (size_t i = 0; i < current_properties_names.size(); i++) {
					string property_name = current_properties_names[i];
					string display_name = property_name;
					LibGens::ObjectElementType type = LibGens::OBJECT_ELEMENT_BOOL;
					
					if (i < current_properties_types.size()) {
						type = current_properties_types[i];
						if (!current_object_list_properties.empty() && type != LibGens::OBJECT_ELEMENT_BOOL) {
							LibGens::Object* first_obj = current_object_list_properties.front();
							LibGens::ObjectElement* element = first_obj->getElement(property_name);
							if (element) {
								switch(type) {
									case LibGens::OBJECT_ELEMENT_INTEGER: {
										LibGens::ObjectElementInteger* int_elem = (LibGens::ObjectElementInteger*)element;
										display_name += ": " + ToString(int_elem->value);
										break;
									}
									case LibGens::OBJECT_ELEMENT_FLOAT: {
										LibGens::ObjectElementFloat* float_elem = (LibGens::ObjectElementFloat*)element;
										char buf[32];
										snprintf(buf, sizeof(buf), ": %.2f", float_elem->value);
										display_name += buf;
										break;
									}
									case LibGens::OBJECT_ELEMENT_STRING: {
										LibGens::ObjectElementString* str_elem = (LibGens::ObjectElementString*)element;
										display_name += ": " + str_elem->value;
										break;
									}
									case LibGens::OBJECT_ELEMENT_VECTOR: {
										LibGens::ObjectElementVector* vec_elem = (LibGens::ObjectElementVector*)element;
										char buf[64];
										snprintf(buf, sizeof(buf), ": (%.1f, %.1f, %.1f)", vec_elem->value.x, vec_elem->value.y, vec_elem->value.z);
										display_name += buf;
										break;
									}
									default: break;
								}
							}
						}
					}
					
					bool is_selected = (current_property_index == (int)i);
					
					if (type == LibGens::OBJECT_ELEMENT_BOOL && !current_object_list_properties.empty()) {
						LibGens::Object* first_obj = current_object_list_properties.front();
						LibGens::ObjectElement* element = first_obj->getElement(property_name);
						if (element) {
							LibGens::ObjectElementBool* bool_elem = (LibGens::ObjectElementBool*)element;
							bool value = bool_elem->value;
							
							if (ImGui::Selectable(property_name.c_str(), is_selected, 0, ImVec2(ImGui::GetContentRegionAvail().x - 30, 0))) {
								current_property_index = (int)i;
							}
							
							ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);
							ImGui::PushID(i);
							ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
							if (ImGui::Checkbox("##boolval", &value)) {
								if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
								current_property_index = (int)i;
								updateEditPropertyBool(value);
							}
							ImGui::PopStyleColor(4);
							ImGui::PopID();
						} else {
							if (ImGui::Selectable(display_name.c_str(), is_selected)) {
								current_property_index = (int)i;
							}
						}
					} else {
						if (ImGui::Selectable(display_name.c_str(), is_selected)) {
							current_property_index = (int)i;
						}
						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) { // im not done yet.
							show_properties_editor = true;
							if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
						}
					}
					
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
            
			if (!current_properties_names.empty() && current_property_index >= 0 && current_property_index < (int)current_properties_names.size()) {
				if (ImGui::Button("Edit Property", ImVec2(-FLT_MIN, 0))) {
					show_properties_editor = true;
					if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
				}
				
				if (ImGui::BeginPopupContextWindow("PropertyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
					ImGui::Text("Property: %s", current_properties_names[current_property_index].c_str());
					ImGui::Separator();
					if (ImGui::MenuItem("Edit...")) {
						show_properties_editor = true;
						if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
					}
					if (current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_VECTOR_LIST) {
						if (ImGui::MenuItem("Add Vector")) {
							addVectorToList();
						}
					}
					if (current_properties_types[current_property_index] == LibGens::OBJECT_ELEMENT_ID_LIST) {
						if (ImGui::MenuItem("Pick Target...")) {
							openQueryTargetMode(true);
						}
					}
					ImGui::EndPopup();
				}
			}
		}
		ImGui::TextWrapped("Help: Select objects from palette and place in level. Use properties panel to edit.");
	}
	ImGui::PopStyleVar(2);
	ImGui::End();
}
