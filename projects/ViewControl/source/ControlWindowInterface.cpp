/**
 * コントロールウィンドウ.
 * 2Dパン、回転、ズームを行うためのボタンがある.
 */
#include "ControlWindowInterface.h"
#include "MathUtil.h"
#include "SelectionUtil.h"

#include <sstream>
#include <boost/format.hpp>

//------------------------------------------------------.
//	マウスドラッグ検出クラス.
//------------------------------------------------------.
CControlWindowMouseTracker::CControlWindowMouseTracker (sxsdk::window_interface &w, const sx::vec<int, 2> &where) : mouse_tracker_class(w, where), shade(w.shade) {
	m_pParent = (CWindowIcon *)&w;
}
/**
 * マウスドラッグ中.
 */
void CControlWindowMouseTracker::mouse_move (sx::vec<int, 2> p)
{
	m_pParent->mouseDrag(p);
}

/**
 * マウスが離された.
 */
void CControlWindowMouseTracker::mouse_release ()
{
	m_pParent->mouseRelease();
}

//------------------------------------------------------.
// 移動/回転/ズームアイコン用.
//------------------------------------------------------.
CWindowIcon::CWindowIcon (sxsdk::window_interface &parent, const int type) : sxsdk::window_interface(parent, 0), m_pParent((CControlWindowInterface *)&parent), m_type(type)
{
	this->set_client_size(sx::vec<int,2>(24, 24));

	// リソースからイメージを読み込み.
	m_iconImage = NULL;
	try {
		switch (m_type) {
		case 0:
			m_iconImage = shade.create_image_interface("move_icon_16x16");
			break;
		case 1:
			m_iconImage = shade.create_image_interface("rotate_icon_16x16");
			break;
		case 2:
			m_iconImage = shade.create_image_interface("zoom_icon_16x16");
			break;
		}
	} catch (...) {
		m_iconImage = NULL;
	}

	m_pushF = false;
}

void CWindowIcon::paint (sxsdk::graphic_context_interface &gc, const sx::rectangle_class *visible_rectangle, void *)
{
	clear(gc);
	const sx::vec<int,2> size = this->get_client_size();

	// 背景を描画.
	if (m_pushF) {
		gc.set_color(sxsdk::rgb_class(0.7f, 0.7f, 0));
		gc.paint_rectangle(sx::rectangle_class(sx::vec<int,2>(0, 0), size));
		gc.restore_color();
	}

	// アイコンを描画.
	if (m_iconImage) {
		gc.draw_image(m_iconImage, sx::rectangle_class(sx::vec<int,2>(0, 0), size));
	}

	// 外枠を描画.
	{
		gc.set_color(sxsdk::rgb_class(0, 0, 0));
		gc.frame_rectangle(sx::rectangle_class(sx::vec<int,2>(0, 0), size));
		gc.restore_color();
	}
}

//------------------------------------------.
// マウス操作用.
//------------------------------------------.
/**
 * マウスダウン.
 */
bool CWindowIcon::mouse_down (int button, sx::vec<int, 2> p, int key_down, int time, bool double_click, void *)
{
	if (!m_pParent->IsSceneActive()) return false;

	// マウスドラッグ（待たずにすぐに次に進む）.
	new CControlWindowMouseTracker(*this, p);

	m_pushF = true;
	this->obsolete();

	m_prevPos = p;

	return true;
}

/**
 * マウスドラッグ中.
 */
void CWindowIcon::mouseDrag (const sx::vec<int, 2>& p)
{
	const sx::vec<int,2> dV = p - m_prevPos;
	if (m_type == 0) m_moveCamera(dV);
	else if (m_type == 1) m_rotateCamera(dV);
	else if (m_type == 2) m_zoomCamera(dV);

	m_prevPos = p;
}

/**
 * ドラッグからのマウスリリース.
 */
void CWindowIcon::mouseRelease ()
{
	m_pushF = false;
	this->obsolete();
}

/**
 * 移動処理.
 * @param[in] dV   ドラッグでの移動量.
 */
void CWindowIcon::m_moveCamera (const sx::vec<int,2>& dV)
{
	if (dV.x == 0 && dV.y == 0) return;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;

		sxsdk::camera_class& camera = scene->get_camera();

		const sxsdk::vec3 eyePos    = m_pParent->GetCameraWorldPos();
		const sxsdk::vec3 targetPos = m_pParent->GetCameraTargetWorldPos();
		const float eyeTargetDist   = sxsdk::absolute(eyePos - targetPos);
		const sxsdk::vec3 cameraDir = (targetPos - eyePos) / eyeTargetDist;

		const sxsdk::mat4 wvMat = camera.get_world_to_view_matrix();
		const sxsdk::mat4 wpMat = camera.get_world_to_perspective_matrix();

		const sxsdk::vec3 cameraWPos = sxsdk::vec3(0, 0, 0) * inv(wvMat);	// ワールド座標でのカメラ位置.

		const sxsdk::mat4 wdMat = scene->get_world_to_device_matrix();
		sxsdk::mat4 dwMat;
		MathUtil::MatrixInverse(dwMat, wdMat);

		// 投影面までの距離.
		float nearPlaneDist;
		{
			sxsdk::vec3 p = cameraWPos + (cameraDir * 100.0f);
			sxsdk::vec4 v4 = sxsdk::vec4(p, 1) * wdMat;
			nearPlaneDist = v4.w - v4.z;
		}

		// 図形ウィンドウ上のスクリーンのサイズ.
		//const sx::vec<int,2> screenSize = m_pParent->GetPersViewSize(scene);

		// スクリーン上で、Xが+1移動するときのワールド座標での移動量.
		const float ddScale = 10.0f;
		sxsdk::vec3 dVx(0, 0, 0);
		{
			sxsdk::vec4 vA(0, 0, 0, nearPlaneDist);
			sxsdk::vec4 vB(ddScale * nearPlaneDist, 0, 0, nearPlaneDist);

			vA = vA * dwMat;
			vB = vB * dwMat;
			dVx = sxsdk::vec3(vB.x, vB.y, vB.z) - sxsdk::vec3(vA.x, vA.y, vA.z);
		}

		// スクリーン上で、Yが+1移動するときのワールド座標での移動量.
		sxsdk::vec3 dVy(0, 0, 0);
		{
			sxsdk::vec4 vA(0, 0, 0, nearPlaneDist);
			sxsdk::vec4 vB(0, ddScale * nearPlaneDist, 0, nearPlaneDist);

			vA = vA * dwMat;
			vB = vB * dwMat;
			dVy = sxsdk::vec3(vB.x, vB.y, vB.z) - sxsdk::vec3(vA.x, vA.y, vA.z);
		}

		{
			const sxsdk::vec3 dd = (dV.x * dVx) + (dV.y * dVy);

			m_pParent->SetCameraWorldPos(eyePos - dd);
			m_pParent->SetCameraTargetWorldPos(targetPos - dd);

			// カメラ位置の更新のため、ウィンドウを再描画.
			m_pParent->obsolete();
		}

	} catch (...) { }
}

/**
 * 回転処理.
 * @param[in] dV   ドラッグでの移動量.
 */
void CWindowIcon::m_rotateCamera (const sx::vec<int,2>& dV)
{
	if (dV.x == 0 && dV.y == 0) return;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;

		sxsdk::camera_class& camera = scene->get_camera();

		const sxsdk::vec3 eyePos    = m_pParent->GetCameraWorldPos();
		const sxsdk::vec3 targetPos = m_pParent->GetCameraTargetWorldPos();
		const float eyeTargetDist   = sxsdk::absolute(eyePos - targetPos);
		const sxsdk::vec3 cameraDir = (targetPos - eyePos) / eyeTargetDist;

		const sxsdk::mat4 wvMat = camera.get_world_to_view_matrix();
		const sxsdk::mat4 wpMat = camera.get_world_to_perspective_matrix();

		const sxsdk::vec3 cameraWPos = sxsdk::vec3(0, 0, 0) * inv(wvMat);	// ワールド座標でのカメラ位置.

		const sxsdk::mat4 wdMat = scene->get_world_to_device_matrix();
		sxsdk::mat4 dwMat;
		MathUtil::MatrixInverse(dwMat, wdMat);

		// 投影面までの距離.
		float nearPlaneDist;
		{
			sxsdk::vec3 p = cameraWPos + (cameraDir * 100.0f);
			sxsdk::vec4 v4 = sxsdk::vec4(p, 1) * wdMat;
			nearPlaneDist = v4.w - v4.z;
		}

		// スクリーン上で、Xが+1移動するときのワールド座標での移動量.
		const float ddScale = 10.0f;
		sxsdk::vec3 dVx(0, 0, 0);
		{
			sxsdk::vec4 vA(0, 0, 0, nearPlaneDist);
			sxsdk::vec4 vB(ddScale * nearPlaneDist, 0, 0, nearPlaneDist);

			vA = vA * dwMat;
			vB = vB * dwMat;
			dVx = sxsdk::vec3(vB.x, vB.y, vB.z) - sxsdk::vec3(vA.x, vA.y, vA.z);
		}

		// スクリーン上で、Yが+1移動するときのワールド座標での移動量.
		sxsdk::vec3 dVy(0, 0, 0);
		{
			sxsdk::vec4 vA(0, 0, 0, nearPlaneDist);
			sxsdk::vec4 vB(0, ddScale * nearPlaneDist, 0, nearPlaneDist);

			vA = vA * dwMat;
			vB = vB * dwMat;
			dVy = sxsdk::vec3(vB.x, vB.y, vB.z) - sxsdk::vec3(vA.x, vA.y, vA.z);
		}

		{
			const sxsdk::vec3 eyeDir = -cameraDir;
			const float scale2 = 10.0f;
			sxsdk::vec3 dd = ((dV.x * dVx) + (dV.y * dVy)) * scale2;

			sxsdk::vec3 newEyePos = eyePos - dd;
			const sxsdk::vec3 eyeDir2 = normalize(newEyePos - targetPos);
			if (sx::inner_product(eyeDir, eyeDir2) < 0.0f) return;
			if (std::abs(eyeDir2.y) > 0.999f) return;		// 真上真下を向くと反転するのを防ぐ.

			newEyePos = (eyeDir2 * eyeTargetDist) + targetPos;

			m_pParent->SetCameraWorldPos(newEyePos);

			// カメラ位置の更新のため、ウィンドウを再描画.
			m_pParent->obsolete();
		}

	} catch (...) { }
}

/**
 * ズーム処理.
 * @param[in] dV   ドラッグでの移動量.
 */
void CWindowIcon::m_zoomCamera (const sx::vec<int,2>& dV)
{
	if (dV.x == 0 && dV.y == 0) return;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;

		sxsdk::camera_class& camera = scene->get_camera();

		const sxsdk::vec3 eyePos    = m_pParent->GetCameraWorldPos();
		const sxsdk::vec3 targetPos = m_pParent->GetCameraTargetWorldPos();

		const float dist = sxsdk::absolute(eyePos - targetPos);
		const sxsdk::vec3 eyeDir = (eyePos - targetPos) / dist;
		const float dist2 = dist - (dist * 0.01f) * dV.y;
		if (dist2 < 0.01f) return;

		m_pParent->SetCameraWorldPos(eyeDir * dist2 + targetPos);

		// カメラ位置の更新のため、ウィンドウを再描画.
		m_pParent->obsolete();

	} catch (...) { }

}

//------------------------------------------------------.
// CControlWindowInterface.
//------------------------------------------------------.
CControlWindowInterface::CControlWindowInterface (sxsdk::shade_interface &shade) : sxsdk::window_interface(shade), shade(shade) {
	m_pMoveIcon   = NULL;
	m_pRotateIcon = NULL;
	m_pZoomIcon   = NULL;
	m_prevTimeMS = 0;
	m_param.Clear();
}

CControlWindowInterface::~CControlWindowInterface ()
{
	if (m_pMoveIcon) {
		delete m_pMoveIcon;
		m_pMoveIcon = NULL;
	}
	if (m_pRotateIcon) {
		delete m_pRotateIcon;
		m_pRotateIcon = NULL;
	}
	if (m_pZoomIcon) {
		delete m_pZoomIcon;
		m_pZoomIcon = NULL;
	}
}

void CControlWindowInterface::initialize (void *)
{
	set_trigger(sxsdk::enums::trigger_enum(sxsdk::enums::active_scene_changed));

#if _WINDOWS
	m_pMoveIcon   = new CWindowIcon(*this, 0);
	m_pRotateIcon = new CWindowIcon(*this, 1);
	m_pZoomIcon   = new CWindowIcon(*this, 2);

	load_sxul("control_window");
#else
	// load_sxulの後に、CWindowIconを生成しないと、Macの場合に追加コントロールが描画されない.
	load_sxul("control_window");

	m_pMoveIcon   = new CWindowIcon(*this, 0);
	m_pRotateIcon = new CWindowIcon(*this, 1);
	m_pZoomIcon   = new CWindowIcon(*this, 2);
#endif

	const int iconSize = 24;

	this->set_client_size(get_layout_bounds().size());
	this->set_title(CControlWindowInterface::name(&shade));
}

void CControlWindowInterface::idle (void *)
{
	if (!this->is_shown()) return;

	// カメラ位置が変更された場合は再描画を促す.
	const int nowTimeMS  = shade.get_current_time();
	const int diffTimeMS = nowTimeMS - m_prevTimeMS;
	const int chkMS = 1000;		// チェック間隔.
	if (diffTimeMS > chkMS) {
		const sxsdk::vec3 cPos       = GetCameraWorldPos();
		const sxsdk::vec3 cTargetPos = GetCameraTargetWorldPos();
		const ViewControlParam::ViewDisplayType dType = m_GetViewDisplayType();
		const bool showBoundingBox   = m_GetShowBoundingBox();
		if (!sx::zero(cPos - m_param.cameraEyePos) || !sx::zero(cTargetPos - m_param.cameraTargetPos) ||
			dType != m_param.viewDisplayType || showBoundingBox != m_param.showBoundingBox) {
			m_param.cameraEyePos    = cPos;
			m_param.cameraTargetPos = cTargetPos;
			m_param.viewDisplayType = dType;
			m_param.showBoundingBox = showBoundingBox;

			// 視線向きのUI表示を更新.
			m_param.viewType = m_GetCameraViewType();

			this->obsolete();
		}
		m_prevTimeMS = nowTimeMS;
	}
}

/**
 * リサイズイベント.
 */
void CControlWindowInterface::resize (int x, int y, bool remake, void *)
{
	if (!m_pMoveIcon) return;
	
	const int iconSize = 24;
	sx::vec<int,2> p(8, 8);
	m_pMoveIcon->set_client_rectangle(sx::rectangle_class(p, sx::vec<int,2>(p.x + iconSize, p.y + iconSize)));

	p.x += iconSize + 4;
	m_pRotateIcon->set_client_rectangle(sx::rectangle_class(p, sx::vec<int,2>(p.x + iconSize, p.y + iconSize)));

	p.x += iconSize + 4;
	m_pZoomIcon->set_client_rectangle(sx::rectangle_class(p, sx::vec<int,2>(p.x + iconSize, p.y + iconSize)));
}

//------------------------------------------.
// setup時のコールバック.
//------------------------------------------.
bool CControlWindowInterface::setup_static_text (sxsdk::window_interface::static_text_class &static_text, void *)
{
	static_text.set_active(IsSceneActive());
	if (!this->is_shown()) {
		return false;
	}

	const std::string idName = static_text.get_control_idname();

	if (idName == "camera_pos") {
		std::stringstream st;
		const sxsdk::vec3 cameraPos = GetCameraWorldPos();		// カメラ位置を取得.
		m_param.cameraEyePos = cameraPos;
		st << ": (" << boost::format("%.3f")%cameraPos.x << ", " << boost::format("%.3f")%cameraPos.y << ", " << boost::format("%.3f")%cameraPos.z << ")";
		static_text.set_title(st.str().c_str());
		return true;
	}
	if (idName == "camera_target") {
		std::stringstream st;
		const sxsdk::vec3 cameraTPos = GetCameraTargetWorldPos();		// カメラ注視点を取得.
		m_param.cameraTargetPos = cameraTPos;
		st << ": (" << boost::format("%.3f")%cameraTPos.x << ", " << boost::format("%.3f")%cameraTPos.y << ", " << boost::format("%.3f")%cameraTPos.z << ")";
		static_text.set_title(st.str().c_str());
		return true;
	}

	return false;
}

bool CControlWindowInterface::setup_push_button (sxsdk::window_interface::push_button_class &push_button, void *)
{
	push_button.set_active(IsSceneActive());
	if (!this->is_shown()) return false;

	const std::string idName = push_button.get_control_idname();

	if (idName == "set_eye_target_but") {
		return true;
	}
	return false;
}

bool CControlWindowInterface::setup_popup_menu (sxsdk::window_interface::popup_menu_class &popup_menu, void *)
{
	popup_menu.set_active(IsSceneActive());
	if (!this->is_shown()) return false;

	const std::string idName = popup_menu.get_control_idname();

	if (idName == "view_type") {
		m_param.viewType = m_GetCameraViewType();

		popup_menu.set_value((int)m_param.viewType);
		return true;
	}

	if (idName == "view_show_type") {
		m_param.viewDisplayType = m_GetViewDisplayType();
		popup_menu.set_value((int)m_param.viewDisplayType);
		return true;
	}

	return false;
}

bool CControlWindowInterface::setup_checkbox (sxsdk::window_interface::checkbox_class &checkbox, void *)
{
	checkbox.set_active(IsSceneActive());
	if (!this->is_shown()) return false;

	const std::string idName = checkbox.get_control_idname();
	if (idName == "view_boundingbox") {
		m_param.showBoundingBox = m_GetShowBoundingBox();
		checkbox.set_value(m_param.showBoundingBox ? 1 : 0);
		return true;
	}

	return false;
}

//------------------------------------------.
// イベント処理のコールバック.
//------------------------------------------.
void CControlWindowInterface::push_button_clicked (sxsdk::window_interface::push_button_class& push_button, void*)
{
	if (!IsSceneActive()) return;

	const std::string idName = push_button.get_control_idname();
	if (idName == "set_eye_target_but") {
		m_SetEyeTarget();
	}
}

void CControlWindowInterface::popup_menu_value_changed (sxsdk::window_interface::popup_menu_class& popup_menu, void*)
{
	if (!IsSceneActive()) return;

	const std::string idName = popup_menu.get_control_idname();
	if (idName == "view_type") {
		// カメラの向きを変更.
		m_ChangeCameraDirection((ViewControlParam::CameraViewType)(popup_menu.get_value()));

		// 各種ボタンが消えるので再描画を促す.
		m_pMoveIcon->obsolete();
		m_pRotateIcon->obsolete();
		m_pZoomIcon->obsolete();
	}

	if (idName == "view_show_type") {
		// 表示の種類を変更.
		m_ChangeViewDisplayType((ViewControlParam::ViewDisplayType)(popup_menu.get_value()));

		// 各種ボタンが消えるので再描画を促す.
		m_pMoveIcon->obsolete();
		m_pRotateIcon->obsolete();
		m_pZoomIcon->obsolete();
	}
}

void CControlWindowInterface::checkbox_value_changed (sxsdk::window_interface::checkbox_class &checkbox, void *)
{
	if (!IsSceneActive()) return;

	const std::string idName = checkbox.get_control_idname();
	if (idName == "view_boundingbox") {
		m_SetShowBoundingBox((checkbox.get_value() != 0) ? true : false);
	}
}

//-------------------------------------------------.
/**
 * 選択要素に注視点を合わせる.
 */
void CControlWindowInterface::m_SetEyeTarget ()
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		sxsdk::camera_class& camera = scene->get_camera();

		// 選択要素の中心を取得.
		const sxsdk::vec3 selectionCenterPos = SelectionUtil::GetSelectionCenter(scene);

		SetCameraTargetWorldPos(selectionCenterPos);
		m_param.cameraTargetPos = selectionCenterPos;

		// 視線向きのUI表示を更新.
		m_param.viewType = m_GetCameraViewType();

		// カメラ位置の更新のため、ウィンドウを再描画.
		this->obsolete();
	} catch (...) { }
}

/**
 * カメラのワールド位置を取得.
 */
sxsdk::vec3 CControlWindowInterface::GetCameraWorldPos ()
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return sxsdk::vec3(0, 0, 0);
		sxsdk::camera_class& camera = scene->get_camera();
		sxsdk::part_class* partC = camera.get_camera_object();
		sxsdk::mat4 lwMat = sxsdk::mat4::identity;
		if (partC) {
			lwMat = partC->get_local_to_world_matrix();
		}
		return camera.get_eye() * lwMat;
	} catch (...) { }

	return sxsdk::vec3(0, 0, 0);
}

/**
 * カメラの注視点ワールド位置を取得.
 */
sxsdk::vec3 CControlWindowInterface::GetCameraTargetWorldPos ()
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return sxsdk::vec3(0, 0, 0);
		sxsdk::camera_class& camera = scene->get_camera();
		sxsdk::part_class* partC = camera.get_camera_object();
		sxsdk::mat4 lwMat = sxsdk::mat4::identity;
		if (partC) {
			lwMat = partC->get_local_to_world_matrix();
		}
		return camera.get_target() * lwMat;
	} catch (...) { }

	return sxsdk::vec3(0, 0, 0);
}

/**
 * カメラのワールド座標位置を指定.
 */
void CControlWindowInterface::SetCameraWorldPos (const sxsdk::vec3& wPos)
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		sxsdk::camera_class& camera = scene->get_camera();
		sxsdk::part_class* partC = camera.get_camera_object();
		sxsdk::mat4 lwMat = sxsdk::mat4::identity;
		if (partC) {
			lwMat = partC->get_local_to_world_matrix();
		}
		camera.set_eye(wPos * inv(lwMat));
#if _WINDOWS
#else
		scene->force_update();	// 強制再描画.
#endif
	} catch (...) { }
}

/**
 * カメラの注視点のワールド座標位置を指定.
 */
void CControlWindowInterface::SetCameraTargetWorldPos (const sxsdk::vec3& wPos)
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		sxsdk::camera_class& camera = scene->get_camera();
		sxsdk::part_class* partC = camera.get_camera_object();
		sxsdk::mat4 lwMat = sxsdk::mat4::identity;
		if (partC) {
			lwMat = partC->get_local_to_world_matrix();
		}
		camera.set_target(wPos * inv(lwMat));
#if _WINDOWS
#else
		scene->force_update();	// 強制再描画.
#endif
	} catch (...) { }
}

/**
 * シーンが選択されているかどうか.
 */
bool CControlWindowInterface::IsSceneActive ()
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (scene) return true;
	} catch (...) { }

	return false;
}

/**
 * カメラの向きによる視線ベクトル.
 */
sxsdk::vec3 CControlWindowInterface::m_GetCameraDirection (const ViewControlParam::CameraViewType viewType)
{
	sxsdk::vec3 cameraDir = sxsdk::vec3(0, 0, 0);
	switch (viewType) {
	case ViewControlParam::view_perspective:
		cameraDir = normalize(sxsdk::vec3(-1, -1, -1));
		break;

	case ViewControlParam::view_front:
		cameraDir = sxsdk::vec3(0, 0, -1);
		break;

	case ViewControlParam::view_back:
		cameraDir = sxsdk::vec3(0, 0, 1);
		break;

	case ViewControlParam::view_top:
		// Y-up時は、少しずらす.
		cameraDir = normalize(sxsdk::vec3(0.01f, -1, 0));
		break;

	case ViewControlParam::view_bottom:
		// Y-up時は、少しずらす.
		cameraDir = normalize(sxsdk::vec3(0.01f, 1, 0));
		break;

	case ViewControlParam::view_left:
		cameraDir = sxsdk::vec3(1, 0, 0);
		break;

	case ViewControlParam::view_right:
		cameraDir = sxsdk::vec3(-1, 0, 0);
		break;
	}
	return cameraDir;
}

/**
 * カメラの向きを変更.
 */
void CControlWindowInterface::m_ChangeCameraDirection (const ViewControlParam::CameraViewType viewType)
{
	ViewControlParam::CameraViewType prevCameraViewType = m_param.viewType;
	const float fAngleMin = (float)(1e-4);

	// 視線方向.
	sxsdk::vec3 prevCameraDir = m_param.cameraTargetPos - m_param.cameraEyePos;
	if (sx::zero(prevCameraDir)) {
		m_param.cameraEyePos = m_param.cameraTargetPos - sxsdk::vec3(0, 0, 1.0f);
		prevCameraDir = m_param.cameraTargetPos - m_param.cameraEyePos;
	}
	const float eLen = sxsdk::absolute(prevCameraDir);
	prevCameraDir = prevCameraDir / eLen;

	// 新しい視線方向.
	sxsdk::vec3 newCameraDir = m_GetCameraDirection(viewType);
	if (viewType == ViewControlParam::view_perspective) {
		if (m_param.viewType == viewType) return;
	}
	m_param.viewType = viewType;

	sxsdk::mat4 rotM = sxsdk::mat4::identity;
	const float angleV = sx::inner_product(prevCameraDir, newCameraDir);
	if (angleV >= 1.0f - fAngleMin) return;

	rotM = sxsdk::mat4::rotate(prevCameraDir, newCameraDir);

	// カメラの位置を変更.
	sxsdk::vec3 newCameraPos = m_param.cameraTargetPos - (prevCameraDir * rotM) * eLen;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		sxsdk::camera_class& camera = scene->get_camera();

		SetCameraWorldPos(newCameraPos);

		// カメラの視線を中心に90度傾いている場合は、補正.
		if (m_param.viewType == ViewControlParam::view_top || m_param.viewType == ViewControlParam::view_bottom) {
			const sxsdk::vec3 tX = (m_param.viewType == ViewControlParam::view_top) ? sxsdk::vec3(-1, 0, 0) : sxsdk::vec3(1, 0, 0);

			// X軸方向をビュー座標変換し、ビュー座標系でX軸を向いていない場合は回転させる.
			const float fAngleMax = 0.9999f;
			const sxsdk::mat4 wvMat = camera.get_world_to_view_matrix();
			const sxsdk::vec4 v4 = sxsdk::vec4(tX, 0) * wvMat;
			const sxsdk::vec3 vX = normalize(sxsdk::vec3(v4.x, v4.y, v4.z));
			float rotV = 0.0f;
			const float angleV = sx::inner_product(vX, tX);
			if (std::abs(angleV) < fAngleMax) {
				const sxsdk::mat4 rM = sxsdk::mat4::rotate(vX, sxsdk::vec3(1, 0, 0));
				sxsdk::vec3 scale, shear, rotate, trans;
				rM.unmatrix(scale, shear, rotate, trans);
				rotV = rotate.z;
			}
			camera.rotate_eye(rotV);
		}
		m_param.cameraEyePos    = GetCameraWorldPos();
		m_param.cameraTargetPos = GetCameraTargetWorldPos();

		// カメラ位置の更新のため、ウィンドウを再描画.
		this->obsolete();
	} catch (...) { }
}

/**
 * 視線の向きより、正面や側面、透視図の判断.
 */
ViewControlParam::CameraViewType CControlWindowInterface::m_GetCameraViewType ()
{
	ViewControlParam::CameraViewType viewType = ViewControlParam::view_perspective;
	const float chkAngle = 0.999f;
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return viewType;

		sxsdk::vec3 cameraEyePos    = GetCameraWorldPos();
		sxsdk::vec3 cameraTargetPos = GetCameraTargetWorldPos();

		// 視線方向.
		sxsdk::vec3 eyeDir = cameraTargetPos - cameraEyePos;
		if (sx::zero(eyeDir)) {
			cameraEyePos = cameraTargetPos - sxsdk::vec3(0, 0, 1.0f);
			eyeDir = cameraTargetPos - cameraEyePos;
		}
		eyeDir = normalize(eyeDir);

		// 視線方向のチェック.
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_back);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_back;
		}
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_front);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_front;
		}
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_bottom);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_bottom;
		}
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_top);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_top;
		}
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_left);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_left;
		}
		{
			const sxsdk::vec3 chkCameraDir = m_GetCameraDirection(ViewControlParam::view_right);
			if (sx::inner_product(chkCameraDir, eyeDir) > chkAngle) return ViewControlParam::view_right;
		}
		return ViewControlParam::view_perspective;

	} catch (...) { }

	return viewType;
}

/**
 * 透視図が表示されているViewPaneを取得.
 */
int CControlWindowInterface::m_GetPerspectiveViewPane ()
{
	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return -1;
		return m_GetPerspectiveViewPane(scene);
	} catch (...) { }

	return -1;
}
int CControlWindowInterface::m_GetPerspectiveViewPane (sxsdk::scene_interface* scene)
{
	int viewPane = -1;
	try {
		compointer<sxsdk::display_interface> display(scene->get_display_interface());
		if (!display) return viewPane;

		// 「透視図」が選択されているViewpaneを取得.
		for (int i = 0; i < 4; ++i) {
			const int cameraType  = display->get_camera_type(i);		// 3が透視図、8がオブジェクトカメラ、9がメタカメラ.
			if (cameraType == 3) {
				viewPane = i;
				break;
			}
		}
		if (viewPane >= 0) return viewPane;

		// 「メタカメラ」または「オブジェクトカメラ」が選択されているViewpaneを取得.
		for (int i = 0; i < 4; ++i) {
			const int cameraType  = display->get_camera_type(i);
			if (cameraType == 8 || cameraType == 9) {
				viewPane = i;
				break;
			}
		}
		if (viewPane >= 0) return viewPane;

	} catch (...) { }

	return viewPane;
}

/**
 * ビューでの表示の種類を取得.
 */
ViewControlParam::ViewDisplayType CControlWindowInterface::m_GetViewDisplayType ()
{
	ViewControlParam::ViewDisplayType vDisplayType = ViewControlParam::view_display_type_texture_wireframe;

	// 4面図のうち、透視図のViewpaneを取得.
	const int currentViewPane = m_GetPerspectiveViewPane();
	if (currentViewPane < 0) return vDisplayType;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return vDisplayType;
		compointer<sxsdk::display_interface> display(scene->get_display_interface());
		if (!display) return vDisplayType;

		// currentViewPaneでのシェーディングの種類を取得.
		const int shadingMode = display->get_shading_mode(currentViewPane);
		switch (shadingMode) {
		case 0:
			vDisplayType = ViewControlParam::view_display_type_wireframe;
			break;
		case 1:
			vDisplayType = ViewControlParam::view_display_type_wireframe_hidden_line;
			break;
		case 2:
			vDisplayType = ViewControlParam::view_display_type_shading;
			break;
		case 3:
			vDisplayType = ViewControlParam::view_display_type_shading_wireframe;
			break;
		case 4:
			vDisplayType = ViewControlParam::view_display_type_texture;
			break;
		case 5:
			vDisplayType = ViewControlParam::view_display_type_texture_wireframe;
			break;
		case 7:
			vDisplayType = ViewControlParam::view_display_type_preview_rendering;
			break;
		}

	} catch (...) { }
	return vDisplayType;
}

/**
 * ViewControlParam::ViewDisplayTypeからShade3Dでのshading_modeに変換.
 */
int CControlWindowInterface::m_DisplayTypeToShadingMode (const ViewControlParam::ViewDisplayType displayType) {
	switch (displayType) {
	case ViewControlParam::view_display_type_wireframe:
		return 0;
	case ViewControlParam::view_display_type_wireframe_hidden_line:
		return 1;
	case ViewControlParam::view_display_type_shading:
		return 2;
	case ViewControlParam::view_display_type_shading_wireframe:
		return 3;
	case ViewControlParam::view_display_type_texture:
		return 4;
	case ViewControlParam::view_display_type_texture_wireframe:
		return 5;
	case ViewControlParam::view_display_type_preview_rendering:
		return 7;
	}
	return 0;
}

/**
 * ビューでの表示の種類を変更.
 */
void CControlWindowInterface::m_ChangeViewDisplayType (const ViewControlParam::ViewDisplayType viewDisplayType)
{
	m_param.viewDisplayType = viewDisplayType;

	// 4面図のうち、透視図のViewpaneを取得.
	const int currentViewPane = m_GetPerspectiveViewPane();
	if (currentViewPane < 0) return;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		compointer<sxsdk::display_interface> display(scene->get_display_interface());
		if (!display) return;

		display->set_shading_mode(currentViewPane, m_DisplayTypeToShadingMode(viewDisplayType));
	} catch (...) { }
}

/**
 * 透視図での、バウンディングボックス表示のOn/Offを取得.
 */
bool CControlWindowInterface::m_GetShowBoundingBox ()
{
	// 4面図のうち、透視図のViewpaneを取得.
	const int currentViewPane = m_GetPerspectiveViewPane();
	if (currentViewPane < 0) return false;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return false;
		compointer<sxsdk::display_interface> display(scene->get_display_interface());
		if (!display) return false;

		return display->get_show_bbox(currentViewPane);
	} catch (...) { }

	return false;
}

/**
 * 透視図での、バウンディングボックス表示のOn/Offを指定.
 */
void CControlWindowInterface::m_SetShowBoundingBox (const bool showBBox)
{
	// 4面図のうち、透視図のViewpaneを取得.
	const int currentViewPane = m_GetPerspectiveViewPane();
	if (currentViewPane < 0) return;

	m_param.showBoundingBox = showBBox;

	try {
		compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
		if (!scene) return;
		compointer<sxsdk::display_interface> display(scene->get_display_interface());
		if (!display) return;

		display->set_show_bbox(currentViewPane, showBBox);
	} catch (...) { }
}

/**
 * 透視図での、図形ウィンドウとしての描画サイズを取得.
 */
sx::vec<int,2> CControlWindowInterface::GetPersViewSize (sxsdk::scene_interface* scene)
{
	sx::vec<int,2> size(640, 480);

	// 4面図のうち、透視図のViewpaneを取得.
	const int currentViewPane = m_GetPerspectiveViewPane();
	if (currentViewPane < 0) return size;

	try {
		sx::rectangle_class rect = scene->get_view_rectangle(currentViewPane);
		size = rect.size();
	} catch (...) { }
	return size;
}
