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
#include "PAC.h"
#include "ObjectLibrary.h"
#include "ObjectSet.h"
#include "MTINode.h"
#include "InstanceMTI.h"

EditorLevel::EditorLevel(string folder_p, string slot_name_p, string geometry_name_p, string slot_id_name_p, size_t game_mode_p) {
	folder = folder_p;
	slot_name = slot_name_p;
	geometry_name = geometry_name_p;
	slot_id_name = slot_id_name_p;
	game_mode = game_mode_p;

	level=NULL;
	terrain=NULL;
	terrain_gi_info=NULL;
	direct_light = NULL;
	terrain_autodraw=NULL;

	cache_folder				   = SONICGLVL_CACHE_PATH + slot_name + "/";
	data_cache_folder			   = SONICGLVL_CACHE_PATH + slot_name + "/" + SONICGLVL_CACHE_DATA_PATH;
	gi_cache_folder				   = SONICGLVL_CACHE_PATH + slot_name + "/" + SONICGLVL_CACHE_GI_TEMP_PATH;
	terrain_cache_folder		   = SONICGLVL_CACHE_PATH + geometry_name + "/" + SONICGLVL_CACHE_TERRAIN_PATH;
	resources_cache_folder		   = SONICGLVL_CACHE_PATH + geometry_name + "/" + SONICGLVL_CACHE_RESOURCES_PATH;
	slot_resources_cache_folder	   = SONICGLVL_CACHE_PATH + slot_id_name + "/" + SONICGLVL_CACHE_SLOT_RESOURCES_PATH;

	model_library    = new LibGens::ModelLibrary(resources_cache_folder + "/");
	material_library = NULL;

	resources_unpacked = false;
	terrain_unpacked = false;

	loadHashes();
}


void EditorLevel::loadHashes() {
	data_hash = {};
	terrain_hash = {};
	resources_hash = {};

	TiXmlDocument doc(cache_folder + SONICGLVL_LEVEL_HASH_FILENAME);
	if (!doc.LoadFile()) {
		return;
	}

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);

	pElem=hDoc.FirstChildElement().Element();
	if (!pElem) {
		return;
	}

	pElem=pElem->FirstChildElement();
	for(pElem; pElem; pElem=pElem->NextSiblingElement()) {
		string entry_name="";

		entry_name = pElem->ValueStr();

		int *hash_pointer = NULL;
		if (entry_name==SONICGLVL_LEVEL_HASH_DATA)      hash_pointer = (int *) &data_hash;
		if (entry_name==SONICGLVL_LEVEL_HASH_TERRAIN)   hash_pointer = (int *) &terrain_hash;
		if (entry_name==SONICGLVL_LEVEL_HASH_RESOURCES) hash_pointer = (int *) &resources_hash;

		if (hash_pointer) {
			for (size_t i=0; i<4; i++) {
				pElem->QueryIntAttribute(SONICGLVL_LEVEL_HASH_VALUE_ATTRIBUTE + ToString(i), &hash_pointer[i]);
			}
		}
	}
}


void EditorLevel::saveHashes() {
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild( decl );

	TiXmlElement *hashRoot = new TiXmlElement(SONICGLVL_LEVEL_HASH_ROOT);

	TiXmlElement *dataRoot = new TiXmlElement(SONICGLVL_LEVEL_HASH_DATA);
	for (size_t i=0; i<4; i++) {
		dataRoot->SetAttribute(SONICGLVL_LEVEL_HASH_VALUE_ATTRIBUTE + ToString(i), ((int*)&data_hash)[i]);
	}
	hashRoot->LinkEndChild(dataRoot);

	TiXmlElement *terrainRoot = new TiXmlElement(SONICGLVL_LEVEL_HASH_TERRAIN);
	for (size_t i=0; i<4; i++) {
		terrainRoot->SetAttribute(SONICGLVL_LEVEL_HASH_VALUE_ATTRIBUTE + ToString(i), ((int*)&terrain_hash)[i]);
	}
	hashRoot->LinkEndChild(terrainRoot);

	TiXmlElement *resourcesRoot = new TiXmlElement(SONICGLVL_LEVEL_HASH_RESOURCES);
	for (size_t i=0; i<4; i++) {
		resourcesRoot->SetAttribute(SONICGLVL_LEVEL_HASH_VALUE_ATTRIBUTE + ToString(i), ((int*)&resources_hash)[i]);
	}
	hashRoot->LinkEndChild(resourcesRoot);

	doc.LinkEndChild(hashRoot);
	doc.SaveFile(cache_folder + SONICGLVL_LEVEL_HASH_FILENAME);
}


void EditorLevel::cleanData() {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((data_cache_folder+"/*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {} 
	else {
		do {
			const char *name=FindFileData.cFileName;
			if (name[0]=='.') continue;

			string new_filename=data_cache_folder+"/"+ToString(name);
			remove(new_filename.c_str());
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return;
}

void EditorLevel::unpackData() {
	string main_filename=folder + "#" + slot_name + ".ar.00";

	LibGens::ArPack *level_data_ar_pack=new LibGens::ArPack(main_filename);
	XXH128_hash_t hash = level_data_ar_pack->computeHash();
	bool unpack=!XXH128_isEqual(hash, data_hash);

	if (unpack) {
		cleanData();
		CreateDirectory(data_cache_folder.c_str(), NULL);
		level_data_ar_pack->extract(data_cache_folder+"/");
		data_hash = hash;
	}
	delete level_data_ar_pack;
}


void EditorLevel::deleteTerrain() {
	if (terrain) {
		terrain->clean();
	}

	if (terrain_block) {
		terrain_block->clean();
	}

	if (terrain_gi_info) {
		terrain_gi_info->clean();
	}
}

void EditorLevel::cleanTerrain() {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((terrain_cache_folder+"/*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {} 
	else {
		do {
			const char *name=FindFileData.cFileName;
			if (name[0]=='.') continue;

			string new_filename=terrain_cache_folder+"/"+ToString(name);
			remove(new_filename.c_str());
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return;
}

void EditorLevel::cleanGI() {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((gi_cache_folder+"/*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {} 
	else {
		do {
			const char *name=FindFileData.cFileName;
			if (name[0]=='.') continue;

			string new_filename=gi_cache_folder+"/"+ToString(name);
			remove(new_filename.c_str());
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return;
}

void EditorLevel::cleanTerrainResources() {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((resources_cache_folder+"/*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {} 
	else {
		do {
			const char *name=FindFileData.cFileName;
			if (name[0]=='.') continue;

			string new_filename=resources_cache_folder+"/"+ToString(name);
			bool delete_file=false;

			if (new_filename.find(LIBGENS_TERRAIN_GROUP_EXTENSION) != string::npos) {
				delete_file = true;
			}

			if (new_filename.find(LIBGENS_TERRAIN_EXTENSION) != string::npos) {
				delete_file = true;
			}

			if (new_filename.find(LIBGENS_TERRAIN_BLOCK_FILENAME) != string::npos) {
				delete_file = true;
			}

			if (delete_file) remove(new_filename.c_str());
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return;
}

void EditorLevel::unpackTerrain() {
	string main_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + SONICGLVL_LEVEL_PACKED_STAGE;
	string main_add_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + SONICGLVL_LEVEL_PACKED_STAGE_ADD;
	if (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED) {
		main_add_filename = folder + SONICGLVL_LEVEL_ADDITIONAL_FOLDER + "/" + geometry_name + "/" + SONICGLVL_LEVEL_PACKED_STAGE_ADD;
	}

	if (!LibGens::File::check(main_filename)) {
		printf("Error: Terrain file not found: %s\n", main_filename.c_str());
		return;
	}

	LibGens::ArPack *stage_data_ar_pack=new LibGens::ArPack(main_filename);
	printf("Opened %s\n", main_filename.c_str());
	
	if (LibGens::File::check(main_add_filename)) {
		LibGens::ArPack *stage_add_data_ar_pack=new LibGens::ArPack(main_add_filename);
		has_additional_gi = stage_add_data_ar_pack->getFileCount() != 0;
		printf("Opened %s\n", main_add_filename.c_str());
		stage_data_ar_pack->merge(stage_add_data_ar_pack);
		printf("Merged AR Packs\n");
	} else {
		printf("Warning: Stage-Add file not found: %s\n", main_add_filename.c_str());
		has_additional_gi = false;
	}

	XXH128_hash_t hash = stage_data_ar_pack->computeHash();
	bool unpack=!XXH128_isEqual(hash, terrain_hash);

	if (unpack) {
		if (MessageBox(NULL, "Do you want to unpack the terrain?", "SonicGLvl", MB_YESNO) != IDYES) unpack=false;
	}

	if (unpack) {
		cleanTerrain();
		cleanGI();
		CreateDirectory(terrain_cache_folder.c_str(), NULL);
		CreateDirectory(gi_cache_folder.c_str(), NULL);

		vector<string> unpacked_files;
		stage_data_ar_pack->extract(terrain_cache_folder + "/", "", "", &unpacked_files);

		vector<std::thread> gia_threads;
		for (size_t i=0; i<unpacked_files.size(); i++) {
			string uncompressed_filename=unpacked_files[i];

			if (uncompressed_filename.find("gia-") != string::npos) {
				printf("Extracting %s\n", uncompressed_filename.c_str());
				string prefix = stage_data_ar_pack->getFileByIndex(i)->getName();
				gia_threads.push_back(std::thread([uncompressed_filename, this, prefix]() {
					LibGens::ArPack *gia_ar_pack=new LibGens::ArPack(uncompressed_filename);
					gia_ar_pack->extract(gi_cache_folder + "/", "", prefix + "-");
					delete gia_ar_pack;
				}));
			}
		}
		for (auto &t : gia_threads) {
			if (t.joinable()) t.join();
		}

		terrain_hash = hash;
	}
	delete stage_data_ar_pack;
}

void EditorLevel::cleanResources() {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((resources_cache_folder+"/*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {} 
	else {
		do {
			const char *name=FindFileData.cFileName;
			if (name[0]=='.') continue;

			string new_filename=resources_cache_folder+"/"+ToString(name);
			remove(new_filename.c_str());
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	return;
}

void EditorLevel::unpackResources() {
	if (game_mode == LIBGENS_LEVEL_GAME_LOST_WORLD) {
		string main_filename=folder + slot_name;
		string trr_cmn_filename = main_filename + "_trr_cmn.pac";
		string sky_cmn_filename = main_filename + "_sky.pac";

		CreateDirectory(resources_cache_folder.c_str(), NULL);

		if (LibGens::File::check(trr_cmn_filename)) {
			LibGens::PacSet *pac_set = new LibGens::PacSet(trr_cmn_filename);
			pac_set->extract(resources_cache_folder + "/", true);
			delete pac_set;
		}

		if (LibGens::File::check(sky_cmn_filename)) {
			LibGens::PacSet *pac_set = new LibGens::PacSet(sky_cmn_filename);
			pac_set->extract(resources_cache_folder + "/", true);
			delete pac_set;
		}
	}
	else {
		string main_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + geometry_name + LIBGENS_AR_MULTIPLE_START;
		if (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED) {
			main_filename = folder + geometry_name + LIBGENS_AR_MULTIPLE_START;
		}

		LibGens::ArPack *resources_data_ar_pack=new LibGens::ArPack(main_filename);
		XXH128_hash_t hash = resources_data_ar_pack->computeHash();
		bool unpack=!XXH128_isEqual(hash, resources_hash);
		bool unpack_slot_resources = (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED && !slot_id_name.empty());

		if (unpack) {
			cleanResources();
			CreateDirectory(resources_cache_folder.c_str(), NULL);
			resources_data_ar_pack->extract(resources_cache_folder + "/");
			resources_hash = hash;
		}
		delete resources_data_ar_pack;

		if (unpack_slot_resources) {
			if (unpack) {
				CreateDirectory(slot_resources_cache_folder.c_str(), NULL);

				WIN32_FIND_DATA FindFileData;
				HANDLE hFind;
				
				// Search for common terrain archives with multiple patterns such as.
				// 2. Cmn<geometry_name>* (e.g., CmnActD_Terrain_EU.ar.00)
				// 3. Cmn<slot_id>* (e.g., CmnEU.ar.00, CmnMykonos.ar.00)
				// 4. Act*_Sub<slot_id>_* (e.g., ActD_SubEU_01.ar.00, ActD_SubMykonos_01.ar.00) for spagonia for example
				vector<string> search_patterns;
				search_patterns.push_back("Cmn*" + slot_id_name + "*.ar.00");
				search_patterns.push_back("Cmn" + geometry_name + "*.ar.00");
				if (geometry_name != slot_id_name) {
					search_patterns.push_back("Cmn" + slot_id_name + "*.ar.00");
				}
				search_patterns.push_back("Act*_Sub" + slot_id_name + "_*.ar.00");
				
				vector<string> slot_resource_files;
				for (size_t i = 0; i < search_patterns.size(); i++) {
					hFind = FindFirstFile((folder + search_patterns[i]).c_str(), &FindFileData);
					if (hFind == INVALID_HANDLE_VALUE) {
						continue;
					}
					
					do {
						const char* name = FindFileData.cFileName;
						if (name[0] == '.') continue;

						string full_path = folder + name;
						// Check if we already extracted this file to avoid duplicates
						if (LibGens::File::check(full_path)) {
							slot_resource_files.push_back(full_path);
						}

					} while (FindNextFile(hFind, &FindFileData) != 0);
					FindClose(hFind);
				}
				vector<std::thread> slot_threads;
				for (const auto &file_path : slot_resource_files) {
					slot_threads.push_back(std::thread([file_path, this]() {
						LibGens::ArPack slot_resources_data_ar_pack(file_path);
						slot_resources_data_ar_pack.extract(slot_resources_cache_folder + "/");
					}));
				}
				for (auto &t : slot_threads) {
					if (t.joinable()) t.join();
				}
			}
		}
	}
}




void EditorLevel::loadData(LibGens::ObjectLibrary *library, ObjectNodeManager *object_node_manager) {
	level = new LibGens::Level(data_cache_folder + "/", game_mode);

	// Fix anything inside the level to fit with the library
	level->learnFromLibrary(library);

	// Add any new templates from the level to the library
	library->learnFromLevel(level, library->getCategory(SONICGLVL_UNASSIGNED_OBJECT_CATEGORY));

	list<LibGens::ObjectSet *> sets=level->getSets();
	for (list<LibGens::ObjectSet *>::iterator set=sets.begin(); set!=sets.end(); set++) {
		list<LibGens::Object *> objects=(*set)->getObjects();

		for (list<LibGens::Object *>::iterator it=objects.begin(); it!=objects.end(); it++) {
			object_node_manager->createObjectNode(*it);
		}
	}
}

void EditorLevel::cleanCollision(list<HavokNode *> &havok_nodes_list) {
	for (list<HavokNode *>::iterator it=havok_nodes_list.begin(); it!=havok_nodes_list.end(); it++) {
		delete (*it);
	}
	havok_nodes_list.clear();
}

void EditorLevel::loadCollision(LibGens::HavokEnviroment *havok_enviroment, Ogre::SceneManager *scene_manager, list<HavokNode *> &havok_nodes_list) {
	if (!level) return;

	list<LibGens::LevelCollisionEntry *> collision_entries=level->getCollisionEntries();
	for (list<LibGens::LevelCollisionEntry *>::iterator it=collision_entries.begin(); it!=collision_entries.end(); it++) {
		havok_enviroment->addFolder(data_cache_folder + "/");
		
		LibGens::HavokPhysicsCache *physics_cache = havok_enviroment->getPhysics((*it)->name);

		if (physics_cache) {
			createHavokNodes(physics_cache, scene_manager, havok_nodes_list);
		}
	}
}


void EditorLevel::createHavokNodes(LibGens::HavokPhysicsCache *physics_cache, Ogre::SceneManager *scene_manager, list<HavokNode *> &havok_nodes_list) {
	hkpPhysicsData *physics_data = physics_cache->getPhysics();

	if (physics_data) {
		cout << "[+] Writing physics data..." << endl;
		const hkArray<hkpPhysicsSystem*> &systems = physics_data->getPhysicsSystems();
		cout << "# Physics Data"  << endl;
		cout << systems.getSize() << "# No. of Physics Systems" << endl;

		for (int i = 0; i < systems.getSize(); i++) {
			// Dump Physics System
			cout << "[+] Dumping physics system #" << (i + 1);

			if (systems[i]->getName() != NULL) {
				cout << " (" << systems[i]->getName() << ")";
			}

			cout << endl;

			const hkArray<hkpRigidBody*> &rigidbodies = systems[i]->getRigidBodies();
			const hkArray<hkpPhantom*> &phantoms      = systems[i]->getPhantoms();
    
			cout << "# Physics System" << endl;
			cout << rigidbodies.getSize() << "# No. of Rigidbodies" << endl;
			cout << phantoms.getSize()    << "# No. of Phantoms" << endl;
		
    
			for (int j = 0; j < rigidbodies.getSize(); j++) {
				// Dump Rigid Body
				hkpRigidBodyCinfo info;
				rigidbodies[j]->getCinfo(info);

				hkTransform transform=rigidbodies[j]->getTransform();
				hkGeometry *geometry=hkpShapeConverter::toSingleGeometry(info.m_shape);

				LibGens::Matrix4 matrix( transform(0, 0), transform(0, 1), transform(0, 2), transform(0, 3),
										 transform(1, 0), transform(1, 1), transform(1, 2), transform(1, 3),
										 transform(2, 0), transform(2, 1), transform(2, 2), transform(2, 3),
										 transform(3, 0), transform(3, 1), transform(3, 2), transform(3, 3));


				// Create Visual Editor Node
				HavokNode *havok_node=new HavokNode(rigidbodies[j]->getName(), geometry, matrix, scene_manager);
				havok_nodes_list.push_back(havok_node);
			}
		}
	}
}


void EditorLevel::loadTerrain(Ogre::SceneManager *scene_manager, list<TerrainNode *> *terrain_nodes_list) {
	if (game_mode == LIBGENS_LEVEL_GAME_LOST_WORLD) {
		string terrain_data_folder = resources_cache_folder;

		terrain          = new LibGens::Terrain();

		direct_light     = new LibGens::Light(terrain_data_folder + "/" + "Direct01.light");

		printf("Terrain Folder: %s\n", terrain_data_folder.c_str());

		material_library = new LibGens::MaterialLibrary(terrain_data_folder + "/");

		// Search for model files
		vector<string> model_files;
		{
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;
			hFind = FindFirstFile((terrain_data_folder+"/*.terrain-model").c_str(), &FindFileData);
			if (hFind == INVALID_HANDLE_VALUE) {} 
			else {
				do {
					const char *name=FindFileData.cFileName;
					if (name[0]=='.') continue;
					model_files.push_back(resources_cache_folder+"/"+ToString(name));
				} while (FindNextFile(hFind, &FindFileData) != 0);
				FindClose(hFind);
			}
		}
		
		vector<LibGens::Model *> terrain_models;
		std::mutex terrain_models_mutex;
		vector<std::thread> model_threads;
		for (const auto &model_file : model_files) {
			model_threads.push_back(std::thread([model_file, &terrain_models, &terrain_models_mutex, this]() {
				LibGens::Model *model = new LibGens::Model(model_file);
				//model->changeVertexFormat(LIBGENS_VERTEX_FORMAT_PC);
				std::lock_guard<std::mutex> lock(terrain_models_mutex);
				terrain_models.push_back(model);
				terrain->addModel(model);
			}));
		}
		for (auto &t : model_threads) {
			if (t.joinable()) t.join();
		}

		vector<LibGens::Model *> used_models;
		// Search for instance files
		vector<string> instance_files;
		{
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;
			hFind = FindFirstFile((terrain_data_folder+"/*.terrain-instanceinfo").c_str(), &FindFileData);
			if (hFind == INVALID_HANDLE_VALUE) {} 
			else {
				do {
					const char *name=FindFileData.cFileName;
					if (name[0]=='.') continue;
					instance_files.push_back(resources_cache_folder+"/"+ToString(name));
				} while (FindNextFile(hFind, &FindFileData) != 0);
				FindClose(hFind);
			}
		}

		std::mutex instance_mutex;
		vector<std::thread> instance_threads;
		vector<LibGens::TerrainInstance*> terrain_instances;
		terrain_instances.reserve(instance_files.size());
		for (const auto &instance_file : instance_files) {
			instance_threads.push_back(std::thread([instance_file, &terrain_models, &terrain_instances, &instance_mutex]() {
				string name = LibGens::File::nameFromFilename(instance_file);
				LibGens::TerrainInstance *instance = new LibGens::TerrainInstance(instance_file, name, &terrain_models);

				std::lock_guard<std::mutex> lock(instance_mutex);
				terrain_instances.push_back(instance);
			}));
		}
		for (auto &t : instance_threads) {
			if (t.joinable()) t.join();
		}

		std::mutex node_mutex;
		LibGens::MaterialLibrary *mat_lib = material_library;
		LibGens::Terrain *terr = terrain;
		
		const size_t num_threads = std::thread::hardware_concurrency() * 2;
		const size_t batch_size = (terrain_instances.size() + num_threads - 1) / num_threads;
		
		vector<std::thread> node_threads;
		for (size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
			size_t start_idx = thread_idx * batch_size;
			size_t end_idx = min(start_idx + batch_size, terrain_instances.size());
			
			if (start_idx >= terrain_instances.size()) break;
			
			node_threads.push_back(std::thread([start_idx, end_idx, &terrain_instances, scene_manager, mat_lib, terrain_nodes_list, &used_models, &node_mutex, terr]() {
				vector<TerrainNode*> local_nodes;
				vector<LibGens::Model*> local_models;
				
				for (size_t i = start_idx; i < end_idx; ++i) {
					TerrainNode *terrain_node=new TerrainNode(terrain_instances[i], scene_manager, mat_lib);
					terrain_node->setGIQualityLevel(NULL, 0);
					local_nodes.push_back(terrain_node);
					local_models.push_back(terrain_instances[i]->getModel());
					terr->addInstance(terrain_instances[i]);
				}
				
				std::lock_guard<std::mutex> lock(node_mutex);
				terrain_nodes_list->insert(terrain_nodes_list->end(), local_nodes.begin(), local_nodes.end());
				used_models.insert(used_models.end(), local_models.begin(), local_models.end());
			}));
		}
		for (auto &t : node_threads) {
			if (t.joinable()) t.join();
		}

		for (size_t i=0; i<terrain_models.size(); i++) {
			bool found=false;

			for (size_t j=0; j<used_models.size(); j++) {
				if (terrain_models[i] == used_models[j]) {
					found = true;
					break;
				}
			}

			if (!found) {
				LibGens::TerrainInstance *instance = new LibGens::TerrainInstance(terrain_models[i]->getName(), terrain_models[i], LibGens::Matrix4());
				instance->setFilename(terrain_data_folder + "/" + terrain_models[i]->getName()+".terrain-instanceinfo");

				// Add to scene
				TerrainNode *terrain_node=new TerrainNode(instance, scene_manager, material_library);
				terrain_node->setGIQualityLevel(NULL, 0);
				terrain_nodes_list->push_back(terrain_node);

				terrain->addInstance(instance);
			}
		}
	}
	else {
		if (!level) return;

		string terrain_data_folder = resources_cache_folder;

		// Terrain-related data files are stored in the data folder on Unleashed
		if (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED) {
			terrain_data_folder = data_cache_folder;
		}

		string ghost_filename = data_cache_folder + "/" + slot_name + LIBGENS_GHOST_EXTENSION;
		string terrain_filename    = terrain_data_folder + "/" + level->getTerrainInfo() + LIBGENS_TERRAIN_EXTENSION;
		string block_filename      = terrain_data_folder + "/" + LIBGENS_TERRAIN_BLOCK_FILENAME;
		string light_list_filename = terrain_data_folder + "/" + LIBGENS_LIGHT_LIST_FILENAME;
		string groups_folder       = terrain_data_folder + "/";
		string gi_info_filename    = terrain_data_folder + "/" + LIBGENS_GI_TEXTURE_GROUP_INFO_FILE;

		string autodraw_filename   = resources_cache_folder + "/" + LIBGENS_TERRAIN_AUTODRAW_TXT;

		terrain          = new LibGens::Terrain(terrain_filename, groups_folder, resources_cache_folder + "/", terrain_cache_folder + "/", gi_cache_folder + "/", false);
		terrain_gi_info  = new LibGens::GITextureGroupInfo(gi_info_filename, terrain_cache_folder + "/");
		terrain_block    = new LibGens::TerrainBlock(block_filename);
		light_list       = new LibGens::LightList(light_list_filename);
		terrain_autodraw = new LibGens::TerrainAutodraw(autodraw_filename);
		ghost    	     = new LibGens::Ghost(ghost_filename);

	material_library = terrain->getMaterialLibrary();
	
	bool has_geometry_sky = false;
	string sky_name = level->getSkybox();
	if (!sky_name.empty()) {
		string sky_model_path = resources_cache_folder + "/" + sky_name + ".model";
		if (LibGens::File::check(sky_model_path)) {
			has_geometry_sky = true;
		}
	}
	
	if (!slot_resources_cache_folder.empty()) {
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile((slot_resources_cache_folder + "/*.material").c_str(), &FindFileData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				const char* name = FindFileData.cFileName;
				if (name[0] == '.') continue;
				
				string material_name = string(name);
				material_name = material_name.substr(0, material_name.length() - 9);
				
				if (has_geometry_sky && material_name.find("sky") != string::npos) {
					continue;
				}
				
				if (!material_library->checkMaterial(material_name)) {
					LibGens::Material *mat = new LibGens::Material(slot_resources_cache_folder + "/" + name);
					material_library->addMaterial(mat);
				}
			} while (FindNextFile(hFind, &FindFileData) != 0);
			FindClose(hFind);
		}
	}		if (light_list) {
			direct_light     = light_list->getLight(level->getDirectLight());
		}
	}
}

void EditorLevel::loadMTI(Ogre::SceneManager *scene_manager, list<Ogre::SceneNode *> *mti_nodes_list) {
	if (!scene_manager) return;
	
	std::map<std::string, std::string> instancer_textures;
	
	string stage_xml = data_cache_folder + "/Stage.stg.xml";
	if (!LibGens::File::check(stage_xml)) {
		stage_xml = data_cache_folder + "/Terrain.stg.xml";
	}
	
	if (LibGens::File::check(stage_xml)) {
		TiXmlDocument doc(stage_xml);
		if (doc.LoadFile()) {
			TiXmlHandle hDoc(&doc);
			TiXmlElement* pElem = hDoc.FirstChildElement().Element();
			if (pElem) {
				for (TiXmlElement* instancer = pElem->FirstChildElement("Instancer"); instancer; instancer = instancer->NextSiblingElement("Instancer")) {
					std::string name;
					TiXmlElement* name_elem = instancer->FirstChildElement("Name");
					if (name_elem && name_elem->GetText()) {
						name = name_elem->GetText();
					}
					
					TiXmlElement* resources = instancer->FirstChildElement("Resources");
					if (resources) {
						TiXmlElement* resource = resources->FirstChildElement("Resource");
						if (resource) {
							TiXmlElement* tex0 = resource->FirstChildElement("Texture0");
							if (tex0 && tex0->GetText()) {
								instancer_textures[name] = std::string(tex0->GetText()) + ".dds";
							}
						}
					}
				}
			}
		}
	}
	
	string mti_folder = resources_cache_folder;
	
	vector<string> mti_files;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile((mti_folder + "/*.mti").c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			const char* name = FindFileData.cFileName;
			if (name[0] == '.') continue;
			mti_files.push_back(mti_folder + "/" + string(name));
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	
	size_t total_nodes = 0;
	for (const auto& mti_file : mti_files) {
		LibGens::InstanceBrush* brush = new LibGens::InstanceBrush(mti_file);
		const list<LibGens::InstanceBrushNode*>& nodes = brush->getNodes();
		
		string mti_basename = mti_file.substr(mti_file.find_last_of("/\\") + 1);
		mti_basename = mti_basename.substr(0, mti_basename.length() - 4);
		
		size_t node_count = nodes.size();
		total_nodes += node_count;
		
		std::string tex_name;
		if (instancer_textures.find(mti_basename) != instancer_textures.end()) {
			tex_name = instancer_textures[mti_basename];
		}
		
		std::string material_name = "MTI_" + mti_basename;
		
		if (!Ogre::MaterialManager::getSingleton().resourceExists(material_name)) {
			Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(material_name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
			pass->setLightingEnabled(false);
			pass->setVertexColourTracking(Ogre::TVC_DIFFUSE);
			pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
			pass->setDepthWriteEnabled(false);
			pass->setAlphaRejectSettings(Ogre::CMPF_GREATER, 128);
			pass->setCullingMode(Ogre::CULL_NONE);
			pass->setDepthBias(1, 1);
			
			if (!tex_name.empty()) {
				Ogre::TextureUnitState* tex_unit = pass->createTextureUnitState(tex_name);
				tex_unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
			}
		}
		
		Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual("MTI_Mesh_" + mti_basename, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		Ogre::SubMesh* submesh = mesh->createSubMesh();
		submesh->useSharedVertices = false;
		submesh->vertexData = new Ogre::VertexData();
		submesh->vertexData->vertexCount = 4;
		
		Ogre::VertexDeclaration* decl = submesh->vertexData->vertexDeclaration;
		size_t offset = 0;
		decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
		
		Ogre::HardwareVertexBufferSharedPtr vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
			offset, 4, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		
		float size = 2.0f;
		float half_size = size * 0.5f;
		float vertices[] = {
			-half_size, 0, 0,  0, 1,
			 half_size, 0, 0,  1, 1,
			 half_size, size, 0,  1, 0,
			-half_size, size, 0,  0, 0
		};
		vbuf->writeData(0, sizeof(vertices), vertices, true);
		submesh->vertexData->vertexBufferBinding->setBinding(0, vbuf);
		
		submesh->indexData = new Ogre::IndexData();
		submesh->indexData->indexCount = 6;
		submesh->indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
			Ogre::HardwareIndexBuffer::IT_16BIT, 6, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		unsigned short indices[] = {0, 1, 2, 0, 2, 3};
		submesh->indexData->indexBuffer->writeData(0, sizeof(indices), indices, true);
		
		const float CHUNK_SIZE = 100.0f;
		std::map<std::pair<int, int>, std::vector<LibGens::InstanceBrushNode*>> spatial_chunks;
		
		for (auto node : nodes) {
			LibGens::Vector3 pos = node->getPosition();
			int chunk_x = (int)(pos.x / CHUNK_SIZE);
			int chunk_z = (int)(pos.z / CHUNK_SIZE);
			spatial_chunks[std::make_pair(chunk_x, chunk_z)].push_back(node);
		}
		
		size_t chunk_id = 0;
		for (const auto& chunk_pair : spatial_chunks) {
			const auto& chunk_nodes = chunk_pair.second;
			size_t chunk_count = chunk_nodes.size();
			
			Ogre::BillboardSet* billboard_set = scene_manager->createBillboardSet(
				"MTI_BillboardSet_" + mti_basename + "_" + std::to_string(chunk_id++), chunk_count);
			billboard_set->setMaterialName(material_name);
			billboard_set->setDefaultDimensions(size, size);
			billboard_set->setBillboardType(Ogre::BBT_POINT);
			billboard_set->setCommonDirection(Ogre::Vector3::UNIT_Y);
			billboard_set->setCommonUpVector(Ogre::Vector3::UNIT_Y);
			billboard_set->setBillboardOrigin(Ogre::BBO_BOTTOM_CENTER);
			
			float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
			float max_x = -FLT_MAX, max_y = -FLT_MAX, max_z = -FLT_MAX;
			
			for (auto node : chunk_nodes) {
				LibGens::Vector3 pos = node->getPosition();
				float px = pos.x, py = pos.y, pz = pos.z;
				
				min_x = std::min(min_x, px - half_size); max_x = std::max(max_x, px + half_size);
				min_y = std::min(min_y, py); max_y = std::max(max_y, py + size);
				min_z = std::min(min_z, pz - half_size); max_z = std::max(max_z, pz + half_size);
				
				Ogre::Billboard* billboard = billboard_set->createBillboard(px, py, pz);
			}
			
			billboard_set->setBounds(Ogre::AxisAlignedBox(min_x, min_y, min_z, max_x, max_y, max_z), 0);
			
			Ogre::SceneNode* chunk_node = scene_manager->getRootSceneNode()->createChildSceneNode();
			chunk_node->attachObject(billboard_set);
			if (mti_nodes_list) mti_nodes_list->push_back(chunk_node);
		}
	}
}

void EditorLevel::importTerrainFBX(LibGens::FBX *fbx) {
	if (!terrain) return;

	// Merge material libraries
	if (fbx->getMaterialLibrary()) {
		terrain->getMaterialLibrary()->merge(fbx->getMaterialLibrary(), true);
		fbx->setMaterialLibrary(NULL);
	}

	// Add unorganized instances and models for group generations
	list<LibGens::TerrainInstance *> terrain_instances=fbx->getInstances();
	for (list<LibGens::TerrainInstance *>::iterator it=terrain_instances.begin(); it!=terrain_instances.end(); it++) {
		terrain->addInstance(*it);
	}

	list<LibGens::Model *> terrain_models=fbx->getModels();
	for (list<LibGens::Model *>::iterator it=terrain_models.begin(); it!=terrain_models.end(); it++) {
		terrain->addModel(*it);
	}
}


void EditorLevel::saveData(string filename) {
	if (!level) return;

	level->saveSpawn();

	list<LibGens::ObjectSet *> sets=level->getSets();
	for (list<LibGens::ObjectSet *>::iterator set=sets.begin(); set!=sets.end(); set++) {
		(*set)->saveXML((*set)->getFilename());
	}

	LibGens::ArPack *data_ar_pack=new LibGens::ArPack(data_cache_folder + "/");
	data_ar_pack->save(filename);
	data_hash = data_ar_pack->computeHash();
	delete data_ar_pack;
}


void EditorLevel::saveTerrain() {
	if (!level) return;
	if (!terrain) return;

	if (terrain) {
		string filename = resources_cache_folder + "/" + level->getTerrainInfo() + LIBGENS_TERRAIN_EXTENSION;
		terrain->save(filename);

		vector<LibGens::TerrainGroup*> terrain_groups = terrain->getGroups();
		for (vector<LibGens::TerrainGroup*>::iterator it = terrain_groups.begin(); it != terrain_groups.end(); it++) {
			string filename = resources_cache_folder + "/" + (*it)->getName() + LIBGENS_TERRAIN_GROUP_EXTENSION;
			(*it)->save(filename);
		}
	}

	if (terrain_block) {
		string filename = resources_cache_folder + "/" + LIBGENS_TERRAIN_BLOCK_FILENAME;
		terrain_block->save(filename);
	}

	if (terrain_gi_info) {
		string filename = resources_cache_folder + "/" + LIBGENS_GI_TEXTURE_GROUP_INFO_FILE;
		terrain_gi_info->save(filename);
	}

	// Pack Stage.pfd and Stage-Add.pfd
	string main_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + SONICGLVL_LEVEL_PACKED_STAGE;
	string main_add_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + SONICGLVL_LEVEL_PACKED_STAGE_ADD;

	vector<string> stage_files;
	vector<string> stage_add_files;
	vector<LibGens::TerrainGroup *> terrain_groups = terrain->getGroups();
	for (vector<LibGens::TerrainGroup *>::iterator it=terrain_groups.begin(); it!=terrain_groups.end(); it++) {
		string filename=terrain_cache_folder + "/" + (*it)->getName() + LIBGENS_TERRAIN_GROUP_FOLDER_EXTENSION;
		(*it)->savePack(filename);
		stage_files.push_back(filename);
	}

	vector<LibGens::GITextureGroup *> gi_groups=terrain_gi_info->getGroups();
	for (vector<LibGens::GITextureGroup *>::iterator it=gi_groups.begin(); it!=gi_groups.end(); it++) {
		if ((*it)->getQualityLevel() == LIBGENS_GI_TEXTURE_GROUP_LOWEST_QUALITY) {
			stage_files.push_back(terrain_cache_folder+"/"+(*it)->getFilename());
		}
		else {
			stage_add_files.push_back(terrain_cache_folder+"/"+(*it)->getFilename());
		}
	}

	LibGens::ArPack *stage_ar_pack=new LibGens::ArPack();
	for (size_t i=0; i<stage_files.size(); i++) {
		string internal_name=stage_files[i];
		string target=internal_name+".cab";
		string command="makecab /D CompressionType=LZX /D CompressionMemory=18 \"" + stage_files[i] + "\" \"" + target + "\"";
		system(command.c_str());
		stage_ar_pack->addFile(target, internal_name);
	}
	stage_ar_pack->save(main_filename, 0x800);
	stage_ar_pack->savePFI(resources_cache_folder + "/" + "Stage.pfi");
	terrain_hash = stage_ar_pack->computeHash();
	delete stage_ar_pack;


	LibGens::ArPack *stage_add_ar_pack=new LibGens::ArPack();
	for (size_t i=0; i<stage_add_files.size(); i++) {
		string internal_name=stage_add_files[i];
		string target=internal_name+".cab";
		string command="makecab /D CompressionType=LZX /D CompressionMemory=18 \"" + stage_add_files[i] + "\" \"" + target + "\"";
		system(command.c_str());
		stage_add_ar_pack->addFile(target, internal_name);
	}
	stage_add_ar_pack->save(main_add_filename, 0x800);
	stage_add_ar_pack->savePFI(resources_cache_folder + "/" + "Stage-Add.pfi");
	delete stage_add_ar_pack;
}

void EditorLevel::saveResources() {
	if (!level) return;

	if (terrain) {
		LibGens::MaterialLibrary *terrain_material_library = terrain->getMaterialLibrary();
		if (terrain_material_library) {
			int root_type = LIBGENS_MATERIAL_ROOT_GENERATIONS;
			if (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED) {
				root_type = LIBGENS_MATERIAL_ROOT_UNLEASHED;
			}
			terrain_material_library->save(resources_cache_folder + "/", root_type);
		}
	}

	string main_filename=folder + SONICGLVL_LEVEL_PACKED_FOLDER + "/" + geometry_name + "/" + geometry_name + LIBGENS_AR_MULTIPLE_START;
	if (game_mode == LIBGENS_LEVEL_GAME_UNLEASHED) {
		main_filename = folder + geometry_name + LIBGENS_AR_MULTIPLE_START;
	}

	LibGens::ArPack *data_ar_pack=new LibGens::ArPack(resources_cache_folder + "/");
	data_ar_pack->save(main_filename);

	resources_hash = data_ar_pack->computeHash();

	delete data_ar_pack;
}


void EditorLevel::generateTerrainGroups(unsigned int cell_size) {
	if (!terrain) return;

	terrain->generateGroups(cell_size);
}

void EditorLevel::unpackResourcesAsync() {
	unpack_resources_thread = std::thread([this]() {
		this->unpackResources();
		std::lock_guard<std::mutex> lock(unpack_mutex);
		resources_unpacked = true;
	});
}

void EditorLevel::unpackTerrainAsync() {
	unpack_terrain_thread = std::thread([this]() {
		this->unpackTerrain();
		std::lock_guard<std::mutex> lock(unpack_mutex);
		terrain_unpacked = true;
	});
}

bool EditorLevel::isResourcesUnpacked() {
	std::lock_guard<std::mutex> lock(unpack_mutex);
	return resources_unpacked;
}

bool EditorLevel::isTerrainUnpacked() {
	std::lock_guard<std::mutex> lock(unpack_mutex);
	return terrain_unpacked;
}

void EditorLevel::waitForUnpacking() {
	if (unpack_resources_thread.joinable()) {
		unpack_resources_thread.join();
	}
	if (unpack_terrain_thread.joinable()) {
		unpack_terrain_thread.join();
	}
} // :ChineseThumbsUp:
