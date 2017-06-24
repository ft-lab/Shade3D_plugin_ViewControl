/**
 * Shade3Dでの選択情報取得用.
 */
#include "SelectionUtil.h"

namespace {
	/**
	 * 選択形状を取得する再帰.
	 * @param[in]  shape           形状の参照.
	 * @param[out] selectShapes    選択形状のポインタを格納.
	 * @param[in]  objectMode      オブジェクトモードの場合はtrue.
	 */
	void GetSelectionShapesLoop (sxsdk::shape_class& shape, std::vector<sxsdk::shape_class *>& selectShapes, const bool objectMode) {
		if (SelectionUtil::IsSelect(shape)) {
			if (objectMode) {
				sxsdk::vec3 bbSize = shape.get_bounding_box_size();
				if (!sx::zero(bbSize)) {
					selectShapes.push_back(&shape);
					return;
				}
			} else {
				const int type = shape.get_type();
				if (type == sxsdk::enums::polygon_mesh || type == sxsdk::enums::line || type == sxsdk::enums::disk || type == sxsdk::enums::sphere) {
					selectShapes.push_back(&shape);
				}
			}
		}

		if (shape.has_son()) {
			sxsdk::shape_class* pS = shape.get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				GetSelectionShapesLoop(*pS, selectShapes, objectMode);
			}
		}
	}

	/**
	 * 選択形状を取得.
	 * @param[in]  shape           形状の参照.
	 * @param[out] selectShapes    選択形状のポインタを格納.
	 * @param[in]  objectMode      オブジェクトモードの場合はtrue.
	 */
	void GetSelectionShapes (sxsdk::scene_interface* scene, std::vector<sxsdk::shape_class *>& selectShapes, const bool objectMode) {
		selectShapes.clear();
		sxsdk::shape_class& shape = scene->get_shape();
		GetSelectionShapesLoop(shape, selectShapes, objectMode);
	}
}

/**
 * 指定の形状が選択されているか (親までたどる).
 */
bool SelectionUtil::IsSelect (sxsdk::shape_class& shape)
{
	bool selectF = false;

	sxsdk::shape_class* pS = &shape;
	while (pS) {
		if (pS->is_one_of_active_shapes()) {
			selectF = true;
			break;
		}
		if (!pS->has_dad()) break;
		pS = pS->get_dad();
	}
	return selectF;
}

/**
 * 選択形状の中心を取得.
 */
sxsdk::vec3 SelectionUtil::GetSelectionCenter (sxsdk::scene_interface* scene)
{
	if (!scene) return sxsdk::vec3(0, 0, 0);

	// 選択形状を取得.
	const bool modifyMode = scene->is_modify_mode();
	std::vector<sxsdk::shape_class *> selectShapes;
	::GetSelectionShapes(scene, selectShapes, !modifyMode);
	if (selectShapes.empty()) return sxsdk::vec3(0, 0, 0);
	const int shapesCou = (int)selectShapes.size();

	try {
		if (modifyMode) {		// 形状編集モードの場合.
			sxsdk::vec3 bbMin, bbMax;
			int cou = 0;
			for (int i = 0; i < shapesCou; ++i) {
				sxsdk::shape_class* shape = selectShapes[i];
				const int selectionMode = scene->get_selection_mode();		// 選択モード.
				const int type = shape->get_type();
				const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
				std::vector<sxsdk::vec3> pointsList;

				// ポリゴンメッシュが選択されている場合.
				if (type == sxsdk::enums::polygon_mesh) {
					sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
					sxsdk::polygon_mesh_saver_class* pSaver = pMesh.get_polygon_mesh_saver();

					// 頂点選択モードの場合.
					if (selectionMode == sxsdk::enums::vertex_selection_mode) {
						const int vCou = pMesh.get_total_number_of_control_points();
						if (vCou == 0) continue;
						for (int j = 0; j < vCou; ++j) {
							sxsdk::vertex_class& v = pMesh.vertex(j);
							if (!v.get_active()) continue;

							const sxsdk::vec3 p = pSaver->get_point(j) * lwMat;
							pointsList.push_back(p);
						}

					} else if (selectionMode == sxsdk::enums::edge_selection_mode) {
						// 稜線選択モードの場合.
						const int eCou = pMesh.get_number_of_edges();
						std::vector<int> vIndexList;
						for (int j = 0; j < eCou; ++j) {
							sxsdk::edge_class& e = pMesh.edge(j);
							if (!e.get_active()) continue;
							vIndexList.push_back(e.get_v0());
							vIndexList.push_back(e.get_v1());
						}
						if (!vIndexList.empty()) {
							for (size_t j = 0; j < vIndexList.size(); ++j) {
								const sxsdk::vec3 p = pSaver->get_point(vIndexList[j]) * lwMat;
								pointsList.push_back(p);
							}
						}

					} else if (selectionMode == sxsdk::enums::face_selection_mode) {
						// 面選択モードの場合.
						const int fCou = pMesh.get_number_of_faces();
						std::vector<int> vIndexList;
						std::vector<int> indices;
						for (int j = 0; j < fCou; ++j) {
							sxsdk::face_class& f = pMesh.face(j);
							if (!f.get_active()) continue;
							const int vCou = f.get_number_of_vertices();
							indices.resize(vCou);
							f.get_vertex_indices(&(indices[0]));
							for (int k = 0; k < vCou; ++k) {
								vIndexList.push_back(indices[k]);
							}
						}
						if (!vIndexList.empty()) {
							for (size_t j = 0; j < vIndexList.size(); ++j) {
								const sxsdk::vec3 p = pSaver->get_point(vIndexList[j]) * lwMat;
								pointsList.push_back(p);
							}
						}
					}

				} else if (type == sxsdk::enums::line) {
					// 線形状が選択されている場合.
					const int vCou = shape->get_active_control_points(NULL);
					if (vCou == 0) continue;
					std::vector<int> vIndices;
					vIndices.resize(vCou);
					shape->get_active_control_points(&(vIndices[0]));

					const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
					for (int j = 0; j < vCou; ++j) {
						sxsdk::vec3 p = (shape->control_point(vIndices[j]).get_position()) * lwMat;
						pointsList.push_back(p);
					}

				} else if (type == sxsdk::enums::disk) {
					// 円が選択されている場合.
					sxsdk::disk_class& disk = shape->get_disk();
					sxsdk::vec3 p = disk.get_center() * lwMat;
					pointsList.push_back(p);

				} else if (type == sxsdk::enums::sphere) {
					// 球が選択されている場合.
					sxsdk::sphere_class& sphere = shape->get_sphere();
					sxsdk::vec3 p = sphere.get_center() * lwMat;
					pointsList.push_back(p);
				}

				for (size_t j = 0; j < pointsList.size(); ++j) {
					const sxsdk::vec3& p = pointsList[j];
					if (cou == 0) {
						bbMin = bbMax = p;
					} else {
						bbMin.x = std::min(bbMin.x, p.x);
						bbMin.y = std::min(bbMin.y, p.y);
						bbMin.z = std::min(bbMin.z, p.z);
						bbMax.x = std::max(bbMax.x, p.x);
						bbMax.y = std::max(bbMax.y, p.y);
						bbMax.z = std::max(bbMax.z, p.z);
					}
					cou++;
				}
			}
			if (cou == 0) return sxsdk::vec3(0, 0, 0);
			return (bbMin + bbMax) * 0.5f;

		} else {							// オブジェクトモードの場合.
			sxsdk::vec3 bbMin, bbMax;
			for (int i = 0; i < shapesCou; ++i) {
				sxsdk::shape_class* shape = selectShapes[i];
				const sxsdk::vec3 p = (shape->get_bounding_box_center());	// ここのバウンディングボックスの中心は、ワールド座標である模様.
				if (i == 0) {
					bbMin = bbMax = p;
				} else {
					bbMin.x = std::min(bbMin.x, p.x);
					bbMin.y = std::min(bbMin.y, p.y);
					bbMin.z = std::min(bbMin.z, p.z);
					bbMax.x = std::max(bbMax.x, p.x);
					bbMax.y = std::max(bbMax.y, p.y);
					bbMax.z = std::max(bbMax.z, p.z);
				}
			}
			return (bbMin + bbMax) * 0.5f;
		}
	} catch (...) { }

	return sxsdk::vec3(0, 0, 0);
}

