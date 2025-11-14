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
#include "ObjectSet.h"

#define NEW_SET_OPTION "New..."
#define DELETE_SET_OPTION "Delete..."

void EditorApplication::updateBottomSelectionGUI() {
		bool objects_selected = selected_nodes.size() > 0;

	if (objects_selected) {
		Ogre::Vector3 axis_position = axis->getPosition();
		Ogre::Quaternion axis_rotation = axis->getRotation();

		Ogre::Radian yRad, pRad, rRad;
		Ogre::Matrix3 mat;
		axis_rotation.ToRotationMatrix(mat);
		mat.ToEulerAnglesYXZ(yRad, pRad, rRad);
		
		bottom_pos_x = (float)axis_position.x;
		bottom_pos_y = (float)axis_position.y;
		bottom_pos_z = (float)axis_position.z;
		
		bottom_rot_x = pRad.valueDegrees();
		bottom_rot_y = yRad.valueDegrees();
		bottom_rot_z = rRad.valueDegrees();
	}
}


void EditorApplication::updateBottomSelectionPosition(float value_x, float value_y, float value_z) {
	axis->setPositionAndTranslate(Ogre::Vector3(value_x, value_y, value_z));
	translateSelection(axis->getTranslate());
}

void EditorApplication::updateBottomSelectionRotation(float value_x, float value_y, float value_z) {
	Ogre::Radian yRad = Ogre::Degree(value_y);
	Ogre::Radian pRad = Ogre::Degree(value_x);
	Ogre::Radian rRad = Ogre::Degree(value_z);

	Ogre::Matrix3 mat;
	mat.FromEulerAnglesYXZ(yRad, pRad, rRad);

	Ogre::Quaternion rotation(mat);

	if (!rotation.isNaN() && (rotation.Norm() > 0)) {
		axis->setRotationAndTranslate(rotation);
		setSelectionRotation(rotation);
	}
}

void EditorApplication::updateSetsGUI() {
	set_indices.clear();

	int index = 0;
	if (current_level && current_level->getLevel()) {
		list<LibGens::ObjectSet *> sets = current_level->getLevel()->getSets();
		for (list<LibGens::ObjectSet *>::iterator it = sets.begin(); it != sets.end(); it++) {
			set_indices[*it] = index++;
			set_visibility[*it] = true;
		}
	}
}

void EditorApplication::updateSelectedSetGUI() {
}

void EditorApplication::renderBottomPanel() {
	float panel_x = 0.0f;
	float panel_height = 100.0f;
	float panel_y = (float)screen_height - panel_height;
	
	ImGui::SetNextWindowPos(ImVec2(panel_x, panel_y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)screen_width - panel_x, panel_height), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	
	ImGui::Begin("BottomPanel", nullptr, 
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoScrollbar);
	
	ImGui::PopStyleVar(3);
	
	bool objects_selected = selected_nodes.size() > 0;
	
	// Object Set at top left
	if (current_level && current_level->getLevel()) {
		list<LibGens::ObjectSet*> sets = current_level->getLevel()->getSets();
		if (!sets.empty()) {
		string current_set_name_str = (current_set && current_set->getName().size() > 0) ? current_set->getName() : "None";
		const char* current_set_name = current_set_name_str.c_str();
		ImGui::Text("Object Set:");
		ImGui::SameLine();
		ImGui::PushItemWidth(150);
			if (ImGui::BeginCombo("##ObjectSet", current_set_name)) {
				for (list<LibGens::ObjectSet*>::iterator it = sets.begin(); it != sets.end(); it++) {
					bool is_selected = (current_set == *it);
					if (ImGui::Selectable((*it)->getName().c_str(), is_selected)) {
						changeCurrentSet((*it)->getName());
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				if (ImGui::Selectable(NEW_SET_OPTION)) {
					newCurrentSet();
				}
				if (ImGui::Selectable(DELETE_SET_OPTION)) {
					deleteCurrentSet();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			bool current_set_visible = (current_set && set_visibility[current_set]);
			if (ImGui::Checkbox("Visible", &current_set_visible)) {
				updateCurrentSetVisible(current_set_visible);
			}
		}
	}
	
	if (objects_selected) {
		ImGui::SameLine(400);
		ImGui::Text("Current Selection's Transform");
	} else {
		ImGui::SameLine(400);
		ImGui::TextDisabled("No Selection");
	}
	
	if (objects_selected) {
		updateBottomSelectionGUI();
		
		ImGui::SetCursorPosX(400);
		ImGui::PushItemWidth(100);
		ImGui::Text("Rotation:"); ImGui::SameLine();
		ImGui::Text("X:"); ImGui::SameLine();
		ImGui::InputFloat("##RotX", &bottom_rot_x, 0.5f, 5.0f, "%.1f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionRotation(bottom_rot_x, bottom_rot_y, bottom_rot_z);
		}
		ImGui::SameLine();
		ImGui::Text("Y:"); ImGui::SameLine();
		ImGui::InputFloat("##RotY", &bottom_rot_y, 0.5f, 5.0f, "%.1f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionRotation(bottom_rot_x, bottom_rot_y, bottom_rot_z);
		}
		ImGui::SameLine();
		ImGui::Text("Z:"); ImGui::SameLine();
		ImGui::InputFloat("##RotZ", &bottom_rot_z, 0.5f, 5.0f, "%.1f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionRotation(bottom_rot_x, bottom_rot_y, bottom_rot_z);
		}
		
		ImGui::SetCursorPosX(400);
		ImGui::Text("Position:"); ImGui::SameLine();
		ImGui::Text("X:"); ImGui::SameLine();
		ImGui::InputFloat("##PosX", &bottom_pos_x, 0.1f, 1.0f, "%.2f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionPosition(bottom_pos_x, bottom_pos_y, bottom_pos_z);
		}
		ImGui::SameLine();
		ImGui::Text("Y:"); ImGui::SameLine();
		ImGui::InputFloat("##PosY", &bottom_pos_y, 0.1f, 1.0f, "%.2f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionPosition(bottom_pos_x, bottom_pos_y, bottom_pos_z);
		}
		ImGui::SameLine();
		ImGui::Text("Z:"); ImGui::SameLine();
		ImGui::InputFloat("##PosZ", &bottom_pos_z, 0.1f, 1.0f, "%.2f");
		if (ImGui::IsItemDeactivatedAfterEdit() && !axis->isHolding()) {
			updateBottomSelectionPosition(bottom_pos_x, bottom_pos_y, bottom_pos_z);
		}
		ImGui::PopItemWidth();
	} else {
		ImGui::SetCursorPosX(400);
		ImGui::Text("No selection");
	}
	
	if (getGhostNode()) {
		ImGui::SameLine();
		if (ImGui::Button("Play")) {
			getGhostNode()->setPlay(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Pause")) {
			getGhostNode()->setPlay(false);
		}
		ImGui::SameLine();
		if (ImGui::Button("<<")) {
			getGhostNode()->setPlay(true);
			getGhostNode()->addTime(-0.01f);
			getGhostNode()->setPlay(false);
		}
		ImGui::SameLine();
		if (ImGui::Button(">>")) {
			getGhostNode()->setPlay(true);
			getGhostNode()->addTime(0.01f);
			getGhostNode()->setPlay(false);
		}
		ImGui::SameLine();
		float ghost_time = getGhostNode()->getTime() * 1000.0f;
		float ghost_duration = getGhostNode()->getDuration() * 1000.0f;
		ImGui::PushItemWidth(200);
		if (ImGui::SliderFloat("##GhostSeek", &ghost_time, 0.0f, ghost_duration, "%.0f ms")) {
			getGhostNode()->setPlay(true);
			getGhostNode()->setTime(ghost_time / 1000.0f);
			getGhostNode()->setPlay(false);
		}
		ImGui::PopItemWidth();
	}
	
	ImGui::End();
}