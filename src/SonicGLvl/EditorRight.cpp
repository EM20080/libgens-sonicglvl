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
#include "ObjectNodeHistory.h"
#include "ObjectSet.h"

void EditorApplication::renderRightPanel() {
	float menu_bar_height = ImGui::GetFrameHeight();
	float panel_height = (float)screen_height - menu_bar_height;
	if (show_bottom_panel) {
		panel_height -= 99.0f;
	}
	
	float right_panel_width = 280.0f;
	float right_panel_x = (float)screen_width - right_panel_width;
	
	ImGui::SetNextWindowPos(ImVec2(right_panel_x, menu_bar_height), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(right_panel_width, panel_height), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	
	if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
		bool objects_selected = selected_nodes.size() > 0;

		// ID section (removed State field as it's not used)
		ImGui::Text("ID");
		ImGui::SameLine(80);
		ImGui::PushItemWidth(-1);
		if (objects_selected && !current_object_list_properties.empty()) {
			LibGens::Object* obj = current_object_list_properties.front();
			char id_str[32];
			snprintf(id_str, sizeof(id_str), "%u", obj->getID());
			ImGui::InputText("##id", id_str, sizeof(id_str), ImGuiInputTextFlags_ReadOnly);
		} else {
			ImGui::InputText("##id", (char*)"", 0, ImGuiInputTextFlags_ReadOnly);
		}
		ImGui::PopItemWidth();
		
		ImGui::Separator();
		
		if (objects_selected && axis) {
			// Update transform values from axis
			Ogre::Vector3 axis_position = axis->getPosition();
			Ogre::Quaternion axis_rotation = axis->getRotation();

			Ogre::Radian yRad, pRad, rRad;
			Ogre::Matrix3 mat;
			axis_rotation.ToRotationMatrix(mat);
			mat.ToEulerAnglesYXZ(yRad, pRad, rRad);

			transform_pos[0] = (float)axis_position.x;
			transform_pos[1] = (float)axis_position.y;
			transform_pos[2] = (float)axis_position.z;
			transform_rot[0] = (float)pRad.valueDegrees();
			transform_rot[1] = (float)yRad.valueDegrees();
			transform_rot[2] = (float)rRad.valueDegrees();
		}

		ImGui::Text("Position");
		ImGui::PushItemWidth(-1);
		ImGui::BeginDisabled(!objects_selected);
		if (ImGui::DragFloat3("##pos", transform_pos, 0.01f, 0.0f, 0.0f, "%.3f")) {
			if (objects_selected && axis && !axis->isHolding()) {
				rememberSelection(false);
				axis->setPositionAndTranslate(Ogre::Vector3(transform_pos[0], transform_pos[1], transform_pos[2]));
				translateSelection(axis->getTranslate());
				makeHistorySelection(false);
			}
		}

		ImGui::Text("Rotation");
		if (ImGui::DragFloat3("##rot", transform_rot, 1.0f, 0.0f, 0.0f, "%.1f")) {
			if (objects_selected && axis && !axis->isHolding()) {
				rememberSelection(true);
				Ogre::Radian yRad = Ogre::Degree(transform_rot[1]);
				Ogre::Radian pRad = Ogre::Degree(transform_rot[0]);
				Ogre::Radian rRad = Ogre::Degree(transform_rot[2]);
				Ogre::Matrix3 mat;
				mat.FromEulerAnglesYXZ(yRad, pRad, rRad);
				Ogre::Quaternion rotation(mat);
				if (!rotation.isNaN() && (rotation.Norm() > 0)) {
					axis->setRotationAndTranslate(rotation);
					setSelectionRotation(rotation);
				}
				makeHistorySelection(true);
			}
		}
		ImGui::EndDisabled();
		ImGui::PopItemWidth();
		
		ImGui::Separator();
		
		ImGui::Text("Properties");
		ImGui::SameLine(ImGui::GetWindowWidth() - 80);
		ImGui::BeginDisabled(!objects_selected || current_property_index < 0 || current_property_index >= (int)current_properties_names.size());
		if (ImGui::Button("Edit", ImVec2(70, 0))) {
			show_properties_editor = true;
			if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
		}
		ImGui::EndDisabled();
		
		float properties_start_y = ImGui::GetCursorPosY();
		float list_height = panel_height - properties_start_y - 10.0f;
		
		if (objects_selected && !current_properties_names.empty()) {
			ImGui::Columns(2, "PropertiesColumns", true);
			ImGui::SetColumnWidth(0, 120);
			ImGui::Text("Name");
			ImGui::NextColumn();
			ImGui::Text("Value");
			ImGui::NextColumn();
			ImGui::Separator();
			ImGui::Columns(1);
			
			if (ImGui::BeginChild("PropertiesList", ImVec2(0, list_height), true)) {
				ImGui::Columns(2, "PropertiesData", true);
				ImGui::SetColumnWidth(0, 120);
				
				for (size_t i = 0; i < current_properties_names.size(); i++) {
					string property_name = current_properties_names[i];
					string value_str = "";
					LibGens::ObjectElementType type = LibGens::OBJECT_ELEMENT_BOOL;
					bool is_bool = false;
					bool bool_value = false;
					
					if (i < current_properties_types.size()) {
						type = current_properties_types[i];
						if (!current_object_list_properties.empty()) {
							LibGens::Object* first_obj = current_object_list_properties.front();
							LibGens::ObjectElement* element = first_obj->getElement(property_name);
							if (element) {
								switch(type) {
									case LibGens::OBJECT_ELEMENT_STRING: {
										LibGens::ObjectElementString* str_elem = (LibGens::ObjectElementString*)element;
										value_str = str_elem->value;
										break;
									}
									case LibGens::OBJECT_ELEMENT_INTEGER: {
										LibGens::ObjectElementInteger* int_elem = (LibGens::ObjectElementInteger*)element;
										value_str = ToString(int_elem->value);
										break;
									}
									case LibGens::OBJECT_ELEMENT_FLOAT: {
										LibGens::ObjectElementFloat* float_elem = (LibGens::ObjectElementFloat*)element;
										char buf[32];
										snprintf(buf, sizeof(buf), "%.3f", float_elem->value);
										value_str = buf;
										break;
									}
									case LibGens::OBJECT_ELEMENT_BOOL: {
										LibGens::ObjectElementBool* bool_elem = (LibGens::ObjectElementBool*)element;
										is_bool = true;
										bool_value = bool_elem->value;
										value_str = bool_elem->value ? "true" : "false";
										break;
									}
									default:
										value_str = "";
										break;
								}
							}
						}
					}
					
					bool is_selected = (current_property_index == (int)i);
					if (ImGui::Selectable(property_name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
						current_property_index = (int)i;
						if (ImGui::IsMouseDoubleClicked(0)) {
							show_properties_editor = true;
							if (!history_edit_property_wrapper) history_edit_property_wrapper = new HistoryActionWrapper();
						}
					}
					ImGui::NextColumn();
					
					if (is_bool) {
						bool temp_bool = bool_value;
						char checkbox_id[64];
						snprintf(checkbox_id, sizeof(checkbox_id), "##bool_%zu", i);
						if (ImGui::Checkbox(checkbox_id, &temp_bool)) {
							if (temp_bool != bool_value) {
								current_property_index = (int)i;
								updateEditPropertyBool(temp_bool);
							}
						}
					} else {
						ImGui::Text("%s", value_str.c_str());
					}
					ImGui::NextColumn();
				}
				
				ImGui::Columns(1);
				ImGui::EndChild();
			}
		} else {
			if (ImGui::BeginChild("PropertiesList", ImVec2(0, list_height), true)) {
				ImGui::TextDisabled("No object selected");
				ImGui::EndChild();
			}
		}
	}
	
	ImGui::PopStyleVar(2);
	ImGui::End();
}