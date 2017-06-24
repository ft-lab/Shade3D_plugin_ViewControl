/**
 * 数値演算関数群.
 */
#ifndef _MATHUTIL_H
#define _MATHUTIL_H

#include "GlobalHeader.h"

namespace MathUtil
{
	/**
	 * ガウスの消去法で逆行列を計算.
	 */
	bool MatrixInverse (sxsdk::mat4& pOut, const sxsdk::mat4& pM);
}

#endif
