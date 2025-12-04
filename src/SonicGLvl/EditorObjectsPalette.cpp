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

#include <d3d9.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#include "../LibGens-externals/stb/stb_image.h"
#include "EditorApplication.h"
#include "EditorNodeHistory.h"
#include "ObjectNodeHistory.h"
#include "ObjectLibrary.h"
#include "ObjectSet.h"

namespace Ogre {
	class D3D9RenderSystem {
	public:
		static IDirect3DDevice9 *getActiveD3D9Device();
	};
}

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
			clearSelection();
			updateSelection();

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

ImTextureID EditorApplication::loadIconTexture(string icon_name) {
	if (icon_textures.find(icon_name) != icon_textures.end()) {
		return icon_textures[icon_name];
	}
	icon_textures[icon_name] = 0;
	IDirect3DDevice9* device = Ogre::D3D9RenderSystem::getActiveD3D9Device();
	if (!device) return 0;
	string icon_path = SONICGLVL_ICONS_PATH + icon_name;
	int width, height, channels;
	unsigned char* data = stbi_load(icon_path.c_str(), &width, &height, &channels, 4);
	if (!data) return 0;
	IDirect3DTexture9* texture = NULL;
	HRESULT hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
	if (SUCCEEDED(hr) && texture) {
		D3DLOCKED_RECT rect;
		if (SUCCEEDED(texture->LockRect(0, &rect, NULL, D3DLOCK_DISCARD))) {
			unsigned char* dest = (unsigned char*)rect.pBits;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					int i = (y * width + x) * 4;
					int j = y * rect.Pitch + x * 4;
					dest[j + 0] = data[i + 2];
					dest[j + 1] = data[i + 1];
					dest[j + 2] = data[i + 0];
					dest[j + 3] = data[i + 3];
				}
			}
			texture->UnlockRect(0);
		}
		stbi_image_free(data);
		icon_textures[icon_name] = (ImTextureID)texture;
		return (ImTextureID)texture;
	}
	stbi_image_free(data);
	return 0;
}

ImTextureID EditorApplication::getIconTexture(string object_name) {
	string icon_name = object_name + ".png";
	if (icon_database && icon_database->hasIcon(object_name)) {
		icon_name = icon_database->getIcon(object_name);
	}
	return loadIconTexture(icon_name);
}

void EditorApplication::renderLeftPanel() {
	float menu_bar_height = ImGui::GetFrameHeight();
	float panel_height = (float)screen_height - menu_bar_height;
	if (show_bottom_panel) {
		panel_height -= 99.0f;
	}
	
	ImGui::SetNextWindowPos(ImVec2(0, menu_bar_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(SONICGLVL_GUI_LEFT_WIDTH + 1, panel_height), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(1.0f);
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
							string cat_name = categories[i]->getName();
							if (cat_name.empty()) cat_name = "(unnamed)";
							if (ImGui::Selectable(cat_name.c_str(), is_selected)) {
								search_buffer[0] = '\0';
								updateObjectsPaletteGUI((int)i);
							}
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
                    
					ImGui::BeginChild("##PaletteList", ImVec2(-FLT_MIN, 300), true);
					
					if (!show_object_icons) {
						if (!current_category_search.empty() && !palette_search_results.empty()) {
							for (size_t i = 0; i < palette_search_results.size(); i++) {
								bool is_selected = (current_palette_selection == palette_search_results[i]);
								string obj_name = palette_search_results[i]->getName();
								if (obj_name.empty()) obj_name = "##obj" + to_string(i);
								if (ImGui::Selectable(obj_name.c_str(), is_selected)) {
									current_palette_selection = palette_search_results[i];
									updateObjectsPalettePreview();
								}
							}
						} else {
							LibGens::ObjectCategory* object_category = library->getCategoryByIndex(current_category_index);
							if (object_category) {
							vector<LibGens::Object*> objects = object_category->getTemplates();
							for (size_t i = 0; i < objects.size(); i++) {
								bool is_selected = (current_palette_selection == objects[i]);
								string obj_name = objects[i]->getName();
								if (obj_name.empty()) obj_name = "##obj" + to_string(i);
								if (ImGui::Selectable(obj_name.c_str(), is_selected)) {
										updateObjectsPaletteSelection((int)i);
										updateObjectsPalettePreview();
									}
								}
							}
						}
					} else {
						// Grid view with icons
					float iconSize = 64.0f;
					float cellSize = iconSize + 20.0f;
					float panelWidth = ImGui::GetContentRegionAvail().x;
					int columns = (int)(panelWidth / cellSize);
					if (columns < 1) columns = 1;
					if (!current_category_search.empty() && !palette_search_results.empty()) {
						for (size_t i = 0; i < palette_search_results.size(); i++) {
							ImGui::PushID((int)i);
							bool is_selected = (current_palette_selection == palette_search_results[i]);
							ImTextureID icon_tex = getIconTexture(palette_search_results[i]->getName());
							ImVec2 cursorPos = ImGui::GetCursorScreenPos();
							ImVec4 bgColor = is_selected ? ImVec4(0.3f, 0.5f, 0.8f, 0.3f) : ImVec4(0.2f, 0.2f, 0.2f, 0.2f);
							ImVec4 borderColor = is_selected ? ImVec4(0.4f, 0.7f, 1.0f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
							ImDrawList* draw_list = ImGui::GetWindowDrawList();
							draw_list->AddRectFilled(cursorPos, ImVec2(cursorPos.x + cellSize - 4, cursorPos.y + cellSize + 20), ImGui::ColorConvertFloat4ToU32(bgColor), 4.0f);
							draw_list->AddRect(cursorPos, ImVec2(cursorPos.x + cellSize - 4, cursorPos.y + cellSize + 20), ImGui::ColorConvertFloat4ToU32(borderColor), 4.0f, 0, 1.5f);
							ImGui::Dummy(ImVec2(cellSize - 4, cellSize + 20));
							if (ImGui::IsItemClicked()) {
								current_palette_selection = palette_search_results[i];
								updateObjectsPalettePreview();
							}
					ImVec2 imagePos = ImVec2(cursorPos.x + (cellSize - 4 - iconSize) * 0.5f, cursorPos.y + 8);
					if (icon_tex && show_object_icons) {
						draw_list->AddImage(icon_tex, imagePos, ImVec2(imagePos.x + iconSize, imagePos.y + iconSize));
					}
					float textYPos = show_object_icons ? (cursorPos.y + iconSize + 10) : (cursorPos.y + (cellSize + 20 - ImGui::GetTextLineHeight()) * 0.5f);
					ImVec2 textPos = ImVec2(cursorPos.x + 4, textYPos);
					ImVec2 textSize = ImGui::CalcTextSize(palette_search_results[i]->getName().c_str(), NULL, false, cellSize - 8);
					draw_list->AddText(NULL, 0, textPos, ImGui::ColorConvertFloat4ToU32(ImVec4(1,1,1,1)), palette_search_results[i]->getName().c_str(), NULL, cellSize - 8);
					if ((i + 1) % columns != 0 && i < palette_search_results.size() - 1) {
						ImGui::SameLine();
					}
							ImGui::PopID();
						}
					} else {
						LibGens::ObjectCategory* object_category = library->getCategoryByIndex(current_category_index);
						if (object_category) {
							vector<LibGens::Object*> objects = object_category->getTemplates();
							for (size_t i = 0; i < objects.size(); i++) {
								ImGui::PushID((int)i);
								bool is_selected = (current_palette_selection == objects[i]);
								ImTextureID icon_tex = getIconTexture(objects[i]->getName());
								ImVec2 cursorPos = ImGui::GetCursorScreenPos();
								ImVec4 bgColor = is_selected ? ImVec4(0.3f, 0.5f, 0.8f, 0.3f) : ImVec4(0.2f, 0.2f, 0.2f, 0.2f);
								ImVec4 borderColor = is_selected ? ImVec4(0.4f, 0.7f, 1.0f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
								ImDrawList* draw_list = ImGui::GetWindowDrawList();
								draw_list->AddRectFilled(cursorPos, ImVec2(cursorPos.x + cellSize - 4, cursorPos.y + cellSize + 20), ImGui::ColorConvertFloat4ToU32(bgColor), 4.0f);
								draw_list->AddRect(cursorPos, ImVec2(cursorPos.x + cellSize - 4, cursorPos.y + cellSize + 20), ImGui::ColorConvertFloat4ToU32(borderColor), 4.0f, 0, 1.5f);
							ImGui::Dummy(ImVec2(cellSize - 4, cellSize + 20));
							if (ImGui::IsItemClicked()) {
								updateObjectsPaletteSelection((int)i);
								updateObjectsPalettePreview();
							}
						ImVec2 imagePos = ImVec2(cursorPos.x + (cellSize - 4 - iconSize) * 0.5f, cursorPos.y + 8);
						if (icon_tex && show_object_icons) {
							draw_list->AddImage(icon_tex, imagePos, ImVec2(imagePos.x + iconSize, imagePos.y + iconSize));
						}
					float textYPos = show_object_icons ? (cursorPos.y + iconSize + 10) : (cursorPos.y + (cellSize + 20 - ImGui::GetTextLineHeight()) * 0.5f);
					ImVec2 textPos = ImVec2(cursorPos.x + 4, textYPos);
					ImVec2 textSize = ImGui::CalcTextSize(objects[i]->getName().c_str(), NULL, false, cellSize - 8);
					draw_list->AddText(NULL, 0, textPos, ImGui::ColorConvertFloat4ToU32(ImVec4(1,1,1,1)), objects[i]->getName().c_str(), NULL, cellSize - 8);
					if ((i + 1) % columns != 0 && i < objects.size() - 1) {
						ImGui::SameLine();
					}
							ImGui::PopID();
						}
					}
				}
					}
				}
				ImGui::EndChild();
			}
		}
		if (current_layout == 1 && ImGui::CollapsingHeader("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (!current_object_list_properties.empty()) {
				LibGens::Object* obj = current_object_list_properties.front();
				ImGui::Text("Name: %s", obj->getName().c_str());
				ImGui::Text("SetObjectID: %u", obj->getID());
				LibGens::ObjectSet* parent_set = obj->getParentSet();
				ImGui::Text("Layer: %s", parent_set ? parent_set->getName().c_str() : "None");
				ImGui::Separator();
			}
			
			float item_height = ImGui::GetTextLineHeightWithSpacing();
			float list_height = std::min((float)current_properties_names.size() * item_height + 4.0f, 200.0f);
			if (ImGui::BeginListBox("##PropertiesList", ImVec2(-FLT_MIN, list_height))) {
				for (size_t i = 0; i < current_properties_names.size(); i++) {
					string property_name = current_properties_names[i];
					string display_name = property_name.empty() ? "(unnamed)" : property_name;
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
							
							string selectable_label = property_name.empty() ? ("##prop" + to_string(i)) : property_name;
							if (ImGui::Selectable(selectable_label.c_str(), is_selected, 0, ImVec2(ImGui::GetContentRegionAvail().x - 30, 0))) {
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
							string label = display_name.empty() ? ("##prop" + to_string(i)) : display_name;
							if (ImGui::Selectable(label.c_str(), is_selected)) {
								current_property_index = (int)i;
							}
						}
					} else {
						string label = display_name.empty() ? ("##prop" + to_string(i)) : display_name;
						if (ImGui::Selectable(label.c_str(), is_selected)) {
							current_property_index = (int)i;
						}
						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
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
			}
		}
		
		if (current_layout == 2 && ImGui::CollapsingHeader("Working Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Working Layer:");
			
			if (current_level && current_level->getLevel()) {
				list<LibGens::ObjectSet*> sets = current_level->getLevel()->getSets();
				LibGens::ObjectSet* current_set_ptr = current_set;
				string current_set_name = current_set_ptr ? current_set_ptr->getName() : "None";
				
				if (ImGui::BeginCombo("##WorkingLayer", current_set_name.c_str())) {
					for (auto set : sets) {
						bool is_selected = (current_set_ptr == set);
						if (ImGui::Selectable(set->getName().c_str(), is_selected)) {
							current_set = set;
						}
						if (is_selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
			
			ImGui::Separator();
			ImGui::Text("Name");
			ImGui::SameLine(200);
			ImGui::Text("Objects");
			
			float list_height = std::min(200.0f, panel_height - ImGui::GetCursorPosY() - 50.0f);
			if (ImGui::BeginListBox("##LayerList", ImVec2(-FLT_MIN, list_height))) {
				if (current_level && current_level->getLevel()) {
					list<LibGens::ObjectSet*> sets = current_level->getLevel()->getSets();
					int idx = 0;
					for (auto set : sets) {
						bool visible = true;
						if (set_visibility.count(set)) {
							visible = set_visibility[set];
						}
						
						ImGui::PushID(idx);
						if (ImGui::Checkbox("##vis", &visible)) {
							set_visibility[set] = visible;
							object_node_manager->updateSetVisibility(set, visible);
						}
						ImGui::PopID();
						
						ImGui::SameLine();
						string display_str = set->getName();
						ImGui::Text("%s", display_str.c_str());
						
						ImGui::SameLine(200);
						ImGui::Text("%d", (int)set->getObjects().size());
						idx++;
					}
				}
				ImGui::EndListBox();
			}
		}
		
		ImGui::TextWrapped("Help: Select objects from palette and place in level. Use properties panel to edit.");
	}
	ImGui::PopStyleVar(2);
	ImGui::End();
}
