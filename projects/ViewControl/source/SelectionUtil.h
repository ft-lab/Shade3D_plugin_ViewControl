/**
 * Shade3Dでの選択情報取得用.
 */

#ifndef _SELECTIONUTIL_H
#define _SELECTIONUTIL_H

#include "GlobalHeader.h"

namespace SelectionUtil
{
	/**
	 * 選択形状の中心を取得.
	 */
	sxsdk::vec3 GetSelectionCenter (sxsdk::scene_interface* scene);

	/**
	 * 指定の形状が選択されているか (親までたどる).
	 */
	bool IsSelect (sxsdk::shape_class& shape);
}

#endif
