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
#include "imgui.h"

void EditorApplication::updateTransformGUI() {
	bool objects_selected = selected_nodes.size() > 0;

	if (objects_selected) {
		Ogre::Vector3 axis_position = axis->getPosition();
		Ogre::Quaternion axis_rotation = axis->getRotation();

		Ogre::Radian yRad, pRad, rRad;
		Ogre::Matrix3 mat;
		axis_rotation.ToRotationMatrix(mat);
		mat.ToEulerAnglesYXZ(yRad, pRad, rRad);
		Ogre::Real yDeg = yRad.valueDegrees();
		Ogre::Real pDeg = pRad.valueDegrees();
		Ogre::Real rDeg = rRad.valueDegrees();

		// Update internal state for ImGui rendering
		transform_pos[0] = (float)axis_position.x;
		transform_pos[1] = (float)axis_position.y;
		transform_pos[2] = (float)axis_position.z;
		transform_rot[0] = (float)pDeg;
		transform_rot[1] = (float)yDeg;
		transform_rot[2] = (float)rDeg;
	}
}
void EditorApplication::updateSelectionPosition(float value_x, float value_y, float value_z, bool push_history) {
	rememberSelection(false);
	{
		axis->setPositionAndTranslate(Ogre::Vector3(value_x, value_y, value_z));
		translateSelection(axis->getTranslate());
	}
	if (push_history) makeHistorySelection(false);
}

void EditorApplication::updateSelectionRotation(float value_x, float value_y, float value_z, bool push_history) {
	rememberSelection(true);
	{
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
	if (push_history) makeHistorySelection(true);
}

bool EditorApplication::isUpdatePosRot()
{
	return is_update_pos_rot;
}

void EditorApplication::updateHelpWithObjectGUI(LibGens::Object* object) {
	if (object) {
		help_object_name = object->getName();
		help_object_description = object->queryExtraName(OBJECT_NODE_EXTRA_DESCRIPTION);
	}
	else {
		help_object_name = "";
		help_object_description = "";
	}
}

void EditorApplication::renderRightPanel() {
	ImGui::Begin("Transform & Properties", nullptr, ImGuiWindowFlags_NoCollapse);

	bool objects_selected = selected_nodes.size() > 0;

	// Transform section
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::BeginDisabled(!objects_selected);
		
		if (objects_selected) {
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
		if (ImGui::DragFloat3("##pos", transform_pos, 0.01f, 0.0f, 0.0f, "%.3f")) {
			if (!axis->isHolding() && objects_selected) {
				updateSelectionPosition(transform_pos[0], transform_pos[1], transform_pos[2]);
			}
		}

		ImGui::Text("Rotation");
		if (ImGui::DragFloat3("##rot", transform_rot, 1.0f, 0.0f, 0.0f, "%.1f")) {
			if (!axis->isHolding() && objects_selected) {
				updateSelectionRotation(transform_rot[0], transform_rot[1], transform_rot[2]);
			}
		}

		ImGui::EndDisabled();
	}

	// Help section
	if (ImGui::CollapsingHeader("Help", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (!help_object_name.empty()) {
			ImGui::TextWrapped("%s", help_object_name.c_str());
			ImGui::Separator();
			ImGui::TextWrapped("%s", help_object_description.c_str());
		} else {
			ImGui::TextDisabled("No object selected");
		}
	}

	ImGui::End();
}