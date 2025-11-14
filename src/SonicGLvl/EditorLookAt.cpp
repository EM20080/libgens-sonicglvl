#include "EditorApplication.h"
#include "EditorNodeHistory.h"
#include "ObjectNodeHistory.h"

void EditorApplication::lookAt(EditorNode* node, int axis, Ogre::Vector3 direction)
{
	Ogre::Quaternion node_rotation = node->getRotation();
	Ogre::Radian y_rad, p_rad, r_rad;
	Ogre::Matrix3 rot_matrix;
	node_rotation.ToRotationMatrix(rot_matrix);
	rot_matrix.ToEulerAnglesYXZ(y_rad, p_rad, r_rad);

	Ogre::Real yaw_rad = y_rad.valueRadians();
	Ogre::Real pitch_rad = p_rad.valueRadians();
	Ogre::Real roll_rad = r_rad.valueRadians();

	yaw_rad = atan2(direction.x, direction.z);
	pitch_rad = asin(-direction.y);

	switch (axis)
	{
	case LIBGENS_MATH_AXIS_X:
		yaw_rad += ((90 * LIBGENS_MATH_PI) / 180);
		break;

	case LIBGENS_MATH_AXIS_Y:
		pitch_rad += ((90 * LIBGENS_MATH_PI) / 180);
		break;

	default:
		break;
	}

	y_rad = yaw_rad;
	p_rad = pitch_rad;
	rot_matrix.FromEulerAnglesYXZ(y_rad, p_rad, r_rad);
	node_rotation = Ogre::Quaternion(rot_matrix);

	node->rememberRotation();
	node->setRotation(node_rotation);
}

void EditorApplication::lookAtEachOther(int axis)
{
	if (selected_nodes.size() != 2)
		return;

	list<EditorNode*>::iterator it = selected_nodes.begin();
	EditorNode* node1 = *it++;
	EditorNode* node2 = *it;

	if (node1->getType() == EDITOR_NODE_OBJECT || node1->getType() == EDITOR_NODE_OBJECT_MSP &&
		node2->getType() == EDITOR_NODE_OBJECT || node2->getType() == EDITOR_NODE_OBJECT_MSP)
	{
		HistoryActionWrapper* wrapper = new HistoryActionWrapper();
		HistoryActionWrapper* sub_wrapper = new HistoryActionWrapper();

		for (it = selected_nodes.begin(); it != selected_nodes.end(); ++it)
		{
			EditorNode* node = *it;
			EditorNode* other_node;
			if (it == selected_nodes.begin())
			{
				++it;
				other_node = *it;
				--it;
			}
			else
			{
				--it;
				other_node = *it;
				++it;
			}

			Ogre::Vector3 direction = other_node->getPosition() - node->getPosition();
			direction.normalise();

			lookAt(node, axis, direction);
			HistoryActionRotateNode* action_rot = new HistoryActionRotateNode(node, node->getLastRotation(), node->getRotation());
			sub_wrapper->push(action_rot);
		}

		wrapper->push(sub_wrapper);
		pushHistory(wrapper);
	}
}

void EditorApplication::lookAtPoint(int axis, Ogre::Vector3 v)
{
	if (!selected_nodes.size())
		return;

	HistoryActionWrapper* wrapper = new HistoryActionWrapper();
	HistoryActionWrapper* sub_wrapper = new HistoryActionWrapper();

	for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); ++it)
	{
		EditorNode* node = *it;
		if (node->getType() == EDITOR_NODE_OBJECT || node->getType() == EDITOR_NODE_OBJECT_MSP)
		{
			Ogre::Vector3 direction = v - node->getPosition();
			direction.normalise();

			lookAt(node, axis, direction);
			HistoryActionRotateNode* action_rot = new HistoryActionRotateNode(node, node->getLastRotation(), node->getRotation());
			sub_wrapper->push(action_rot);
		}
	}

	wrapper->push(sub_wrapper);
	pushHistory(wrapper);
}

void EditorApplication::openLookAtPointGUI()
{
	if (!show_look_at_dialog)
	{
		if (!selected_nodes.size()) return;
		list<EditorNode*>::iterator it = selected_nodes.begin();

		Ogre::Vector3 position = (*it)->getPosition();
		vector_node = new VectorNode(scene_manager);
		vector_node->setPosition(position);

		look_at_x = position.x;
		look_at_y = position.y;
		look_at_z = position.z;
		look_at_axis = LIBGENS_MATH_AXIS_Z;

		is_update_look_at_vector = true;
		show_look_at_dialog = true;
	}
}

void EditorApplication::closeLookAtPointGUI()
{
	if (show_look_at_dialog)
	{
		queryLookAtObject(false);
		show_look_at_dialog = false;
	}
	delete vector_node;
}

void EditorApplication::updateLookAtVectorMode(bool mode_state) {
	is_update_look_at_vector = mode_state;

	if (mode_state) {
		previous_selected_nodes = selected_nodes;
		for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
			(*it)->setSelect(false);
		}
		selected_nodes.clear();

		setEditorMode(EDITOR_NODE_QUERY_VECTOR);

		look_at_vector_history->clear();

		selected_nodes.push_back(vector_node);
		vector_node->setSelect(true);

		updateSelection();
		focusLookAtPointVector();
	}
	else {
		for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
			(*it)->setSelect(false);
		}

		selected_nodes = previous_selected_nodes;
		for (list<EditorNode*>::iterator it = selected_nodes.begin(); it != selected_nodes.end(); it++) {
			(*it)->setSelect(true);
		}

		setEditorMode(EDITOR_NODE_QUERY_OBJECT);

		look_at_vector_history->clear();
		updateSelection();
	}
}

void EditorApplication::updateLookAtVectorGUI()
{
	is_update_look_at_vector = false;
	if (!vector_node || !show_look_at_dialog) return;

	Ogre::Vector3 position = vector_node->getPosition();
	look_at_x = position.x;
	look_at_y = position.y;
	look_at_z = position.z;

	is_update_look_at_vector = true;
}

void EditorApplication::updateLookAtPointVectorNode(Ogre::Vector3 v)
{
	vector_node->setPosition(v);
}

void EditorApplication::focusLookAtPointVector()
{
	if (vector_node)
		viewport->focusOnPoint(vector_node->getPosition());
}

bool EditorApplication::isUpdateLookAtVector()
{
	return is_update_look_at_vector;
}

void EditorApplication::queryLookAtObject(bool mode)
{
	is_pick_target_position = mode;
}

void EditorApplication::renderLookAtDialog() {
	if (!show_look_at_dialog) return;

	if (ImGui::Begin("Look At Point", &show_look_at_dialog, ImGuiWindowFlags_NoResize)) {
		ImGui::Text("Target Position:");
		
		if (ImGui::InputFloat("X##lookat", &look_at_x)) {
			if (is_update_look_at_vector) {
				updateLookAtPointVectorNode(Ogre::Vector3(look_at_x, look_at_y, look_at_z));
			}
		}
		
		if (ImGui::InputFloat("Y##lookat", &look_at_y)) {
			if (is_update_look_at_vector) {
				updateLookAtPointVectorNode(Ogre::Vector3(look_at_x, look_at_y, look_at_z));
			}
		}
		
		if (ImGui::InputFloat("Z##lookat", &look_at_z)) {
			if (is_update_look_at_vector) {
				updateLookAtPointVectorNode(Ogre::Vector3(look_at_x, look_at_y, look_at_z));
			}
		}
		
		ImGui::Separator();
		
		ImGui::Text("Forward Axis:");
		ImGui::RadioButton("X Axis", &look_at_axis, LIBGENS_MATH_AXIS_X); ImGui::SameLine();
		ImGui::RadioButton("Y Axis", &look_at_axis, LIBGENS_MATH_AXIS_Y); ImGui::SameLine();
		ImGui::RadioButton("Z Axis", &look_at_axis, LIBGENS_MATH_AXIS_Z);
		
		ImGui::Separator();
		
		if (ImGui::Button("Focus on Point")) {
			focusLookAtPointVector();
		}
		
		if (ImGui::Button("Apply")) {
			Ogre::Vector3 position(look_at_x, look_at_y, look_at_z);
			lookAtPoint(look_at_axis, position);
			updateSelection();
			closeLookAtPointGUI();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			closeLookAtPointGUI();
		}
	}
	ImGui::End();
}