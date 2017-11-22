/**
 *  @file   GlobalHeader.h
 *  @brief  共通して使用する変数など.
 */

#ifndef _GLOBALHEADER_H
#define _GLOBALHEADER_H

#include "sxsdk.cxx"

/**
 * ウィンドウインターフェイス派生クラスのプラグインID.
 */
#define CONTROL_WINDOW_ID sx::uuid_class("A25A8DB3-D149-4A0C-8EBD-E0BD6FAE7DBC")

namespace ViewControlParam {
	/**
	 * カメラの向きの種類.
	 */
	enum CameraViewType {
		view_perspective = 0,		// 透視図.
		view_top,					// 上面図.
		view_bottom,				// 底面図.
		view_front,					// 正面図.
		view_back,					// 背面図.
		view_right,					// 右面図.
		view_left,					// 左面図.
	};

	/**
	 * ビューでの表示の種類.
	 */
	enum ViewDisplayType {
		view_display_type_wireframe = 0,			// ワイヤーフレーム.
		view_display_type_wireframe_hidden_line,	// ワイヤーフレーム (陰線消去).
		view_display_type_shading,					// シェーディング.
		view_display_type_shading_wireframe,		// シェーディング + ワイヤーフレーム.
		view_display_type_texture,					// テクスチャ.
		view_display_type_texture_wireframe,		// テクスチャ + ワイヤーフレーム.
		view_display_type_preview_rendering,		// プレビューレンダリング.
	};
}

/**
 * 操作パラメータ.
 */
class CControlParam
{
public:
	ViewControlParam::CameraViewType viewType;				// カメラの向き.
	sxsdk::vec3 cameraEyePos;								// カメラの視点位置.
	sxsdk::vec3 cameraTargetPos;							// カメラの注視点位置.
	ViewControlParam::ViewDisplayType viewDisplayType;		// ビューでの表示の種類.
	bool showBoundingBox;									// バウンディングボックス表示.

public:
	CControlParam () {
		Clear();
	}

	void Clear () {
		viewType = ViewControlParam::view_perspective;
		cameraEyePos    = sxsdk::vec3(0, 0, 0);
		cameraTargetPos = sxsdk::vec3(0, 0, 0);
		viewDisplayType = ViewControlParam::view_display_type_texture_wireframe;
		showBoundingBox = true;
	}
};

#endif
