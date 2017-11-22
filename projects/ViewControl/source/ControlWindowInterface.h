/**
 * コントロールウィンドウ.
 * 2Dパン、回転、ズームを行うためのボタンがある.
 */
#ifndef _CONTROLWINDOWINTERFACE_H
#define _CONTROLWINDOWINTERFACE_H

#include "GlobalHeader.h"

//------------------------------------------------------.
//	マウスドラッグ検出クラス.
//------------------------------------------------------.
class CControlWindowInterface;
class CWindowIcon;
class CControlWindowMouseTracker : public sxsdk::window_interface::mouse_tracker_class
{
private:
	int m_px, m_py;
	sxsdk::shade_interface& shade;
	CWindowIcon *m_pParent;

public:
	CControlWindowMouseTracker (sxsdk::window_interface &w, const sx::vec<int, 2> &where);

	/**
	 * マウスドラッグ中.
	 */
	virtual void mouse_move (sx::vec<int, 2> p);

	/**
	 * マウスが離された.
	 */
	virtual void mouse_release ();
};

//------------------------------------------------------.
// 移動/回転/ズームアイコン用.
//------------------------------------------------------.
class CWindowIcon : public sxsdk::window_interface {
private:
	CControlWindowInterface *m_pParent;
	int m_type;											// 0:移動  1:回転  2:ズーム.
	compointer<sxsdk::image_interface> m_iconImage;		// アイコン画像.
	bool m_pushF;										// 操作中はtrue.

	sx::vec<int, 2> m_prevPos;

private:
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }

	virtual void paint (sxsdk::graphic_context_interface &gc, const sx::rectangle_class *visible_rectangle, void *aux=0);

private:
	/**
	 * 移動処理.
	 * @param[in] dV   ドラッグでの移動量.
	 */
	void m_moveCamera (const sx::vec<int,2>& dV);

	/**
	 * 回転処理.
	 * @param[in] dV   ドラッグでの移動量.
	 */
	void m_rotateCamera (const sx::vec<int,2>& dV);

	/**
	 * ズーム処理.
	 * @param[in] dV   ドラッグでの移動量.
	 */
	void m_zoomCamera (const sx::vec<int,2>& dV);

	//------------------------------------------.
	// マウス操作用.
	//------------------------------------------.
	/**
	 * マウスダウン.
	 */
	virtual bool mouse_down (int button, sx::vec<int, 2> p, int key_down, int time, bool double_click, void *aux=0);

public:
	/**
	 * マウスドラッグ中.
	 */
	void mouseDrag (const sx::vec<int, 2>& p);

	/**
	 * ドラッグからのマウスリリース.
	 */
	void mouseRelease ();

public:
	CWindowIcon (sxsdk::window_interface &parent, const int type);

};

//------------------------------------------------------.
// CControlWindowInterface.
//------------------------------------------------------.
class CControlWindowInterface : public sxsdk::window_interface {
private:
	sxsdk::shade_interface &shade;

	CWindowIcon* m_pMoveIcon;		// 移動アイコン.
	CWindowIcon* m_pRotateIcon;		// 回転アイコン.
	CWindowIcon* m_pZoomIcon;		// ズームアイコン.

	int m_prevTimeMS;						// 経過時間.

	CControlParam m_param;					// ウィンドウパラメータ.

private:
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }
	virtual sx::uuid_class get_uuid (void *) { return CONTROL_WINDOW_ID; }

	virtual int get_placement_flags (void *aux=0) {
		return sxsdk::window_interface::view_menu_placement_flag;
	}
	virtual int get_flags (void *aux=0) {
		return 0;
	}

	/**
	 * 初期化処理.
	 */
	virtual void initialize (void *);

	/**
	 * リサイズ無効.
	 */
	virtual bool is_resizable (void *aux=0) { return false; }

	/**
	 * リサイズイベント.
	 */
	virtual void resize (int x, int y, bool remake, void *aux = 0);

	virtual void idle (void *aux=0);

	//------------------------------------------.
	// setup時のコールバック.
	//------------------------------------------.
	virtual bool setup_static_text (sxsdk::window_interface::static_text_class &static_text, void *aux=0);
	virtual bool setup_push_button (sxsdk::window_interface::push_button_class &push_button, void *aux=0);
	virtual bool setup_popup_menu (sxsdk::window_interface::popup_menu_class &popup_menu, void *aux=0);
	virtual bool setup_checkbox (sxsdk::window_interface::checkbox_class &checkbox, void *aux=0);

	//------------------------------------------.
	// イベント処理のコールバック.
	//------------------------------------------.
	virtual void push_button_clicked (sxsdk::window_interface::push_button_class& push_button, void* aux = 0);
	virtual void popup_menu_value_changed (sxsdk::window_interface::popup_menu_class& popup_menu, void* aux = 0);
	virtual void checkbox_value_changed (sxsdk::window_interface::checkbox_class &checkbox, void *aux=0);

private:
	/**
	 * 選択要素に注視点を合わせる.
	 */
	void m_SetEyeTarget ();

	/**
	 * カメラの向きを変更.
	 */
	void m_ChangeCameraDirection (const ViewControlParam::CameraViewType viewType);

	/**
	 * カメラの向きによる視線ベクトル.
	 */
	sxsdk::vec3 m_GetCameraDirection (const ViewControlParam::CameraViewType viewType);

	/**
	 * 視線の向きより、正面や側面、透視図の判断.
	 */
	ViewControlParam::CameraViewType m_GetCameraViewType ();

	/**
	 * ビューでの表示の種類を変更.
	 */
	void m_ChangeViewDisplayType (const ViewControlParam::ViewDisplayType viewDisplayType);

	/**
	 * ビューでの表示の種類を取得.
	 */
	ViewControlParam::ViewDisplayType m_GetViewDisplayType ();

	/**
	 * 透視図が表示されているViewPaneを取得.
	 */
	int m_GetPerspectiveViewPane ();
	int m_GetPerspectiveViewPane (sxsdk::scene_interface* scene);

	/**
	 * ViewControlParam::ViewDisplayTypeからShade3Dでのshading_modeに変換.
	 */
	int m_DisplayTypeToShadingMode (const ViewControlParam::ViewDisplayType displayType);

	/**
	 * 透視図での、バウンディングボックス表示のOn/Offを取得.
	 */
	bool m_GetShowBoundingBox ();

	/**
	 * 透視図での、バウンディングボックス表示のOn/Offを指定.
	 */
	void m_SetShowBoundingBox (const bool showBBox);

public:
	explicit CControlWindowInterface (sxsdk::shade_interface &shade);
	virtual ~CControlWindowInterface ();

	static const char *name (sxsdk::shade_interface *shade) { return shade->gettext("title"); }

	/**
	 * シーンが選択されているかどうか.
	 */
	bool IsSceneActive ();

	/**
	 * カメラのワールド位置を取得.
	 */
	sxsdk::vec3 GetCameraWorldPos ();

	/**
	 * カメラの注視点ワールド位置を取得.
	 */
	sxsdk::vec3 GetCameraTargetWorldPos ();

	/**
	 * カメラのワールド座標位置を指定.
	 */
	void SetCameraWorldPos (const sxsdk::vec3& wPos);

	/**
	 * カメラの注視点のワールド座標位置を指定.
	 */
	void SetCameraTargetWorldPos (const sxsdk::vec3& wPos);

	/**
	 * 透視図での、図形ウィンドウとしての描画サイズを取得.
	 */
	sx::vec<int,2> GetPersViewSize (sxsdk::scene_interface* scene);

	/**
	 * 平行移動量を取得.
	 */
	float GetMoveScale ();
}; 

#endif
