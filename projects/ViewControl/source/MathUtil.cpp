/**
 * 数値演算関数群.
 */
#include "MathUtil.h"

/**
 * ガウスの消去法で逆行列を計算.
 */
bool MathUtil::MatrixInverse (sxsdk::mat4& pOut, const sxsdk::mat4& pM)
{
	sxsdk::mat4 mat;
	double fDat, fDat2;
	double mat_8x4[4][8];
	int flag;

	// 8 x 4行列に値を入れる.
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) mat_8x4[i][j] = (double)(pM[i][j]);
		for (int j = 0; j < 4; ++j) {
			mat_8x4[i][j + 4] = (i == j) ? 1.0 : 0.0;
		}
	}

	flag = 1;
	for (int loop = 0; loop < 4; ++loop) {
		fDat = mat_8x4[loop][loop];
		if (fDat != 1.0) {
			int i;
			if (fDat == 0.0) {
				for (i = loop + 1; i < 4; ++i) {
					fDat = mat_8x4[i][loop];
					if (fDat != 0.0) break;
				}
				if (i >= 4) {
					flag = 0;
					break;
				}

				// 行を入れ替える
				for (int j = 0; j < 8; ++j) {
					fDat = mat_8x4[i][j];
					mat_8x4[i][j] = mat_8x4[loop][j];
					mat_8x4[loop][j] = fDat;
				}
				fDat = mat_8x4[loop][loop];
			}

			for (i = 0; i < 8; ++i) mat_8x4[loop][i] /= fDat;
		}
		for (int i = 0; i < 4; ++i) {
			if (i != loop) {
				fDat = mat_8x4[i][loop];
				if (fDat != 0.0f) {
					// mat[i][loop]をmat[loop]の行にかけて.
					// (mat[j] - mat[loop] * fDat)を計算.
					for (int j = 0; j < 8; ++j) {
						fDat2 = mat_8x4[loop][j] * fDat;
						mat_8x4[i][j] -= fDat2;
					}
				}
			}
		}
	}

	if (flag) {
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				mat[i][j] = (float)(mat_8x4[i][j + 4]);
			}
		}
	} else {
		// 単位行列を求める.
		mat = sxsdk::mat4::identity;
	}
	pOut = mat;

	if (flag) return true;
	return false;
}

