# ViewControlプラグイン for Shade3D

「ViewControl」は、Shade3Dの透視図で、他の3DCGツールと似たような「スクロール」「回転」「ズーム」を行うプラグインです。  
また、透視図でのカメラの向きを透視図のほか正面/右面/上面などに切り替える機能を設けています。  
指定の形状を注視点とする機能も同一ウィンドウに置いています。  
<img src="https://github.com/ft-lab/Shade3D_plugin_ViewControl/blob/master/wiki_images/ViewControl_01.jpg"/>

## 動作環境

* Windows 7/8/10以降のOS
* Mac 10.9以降のOS
* Shade3D ver.15/16以降で、Standard/Professional版（Basic版では動作しません）

## ソースのビルド (開発向け)
WindowsはVisual Studio 2013を使用。  
MacはXcode 6.4を使用。  
Shade3D Plugin SDK ver.15.1が必要です ( https://shade3d.jp/community/sdn/sdk.html )。  

## ビルド方法 (開発向け)

Shade3D Plugin SDK ver.15.1をダウンロード。  
Shade3D_plugin_ViewControlでダウンロードしたソースのViewControlディレクトリを、Shade3D Plugin SDKのplugin_projectsディレクトリ内に配置。  

### Windows

Visual Studio 2013で、ViewControl/win/Template.slnを読み込みビルドします。  

### Mac

Xcode6.xで、ViewControl/mac/plugins/Template.xcodeprojを読み込みビルドします。  

## 実行方法

### Windows

ビルドされた ViewControl64.dll (32bits版の場合はViewControl.dll)をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
メインメニューの「表示」-「ビューコントロール」からビューコントロールウィンドウが表示されるのを確認。  

### Mac

ビルドされた ViewControl64.shdpluginをShade3Dのpluginsディレクトリに格納してShade 3Dを起動。  
メインメニューの「表示」-「ビューコントロール」からビューコントロールウィンドウが表示されるのを確認。  

## 使い方

メインメニューの「表示」-「ビューコントロール」を選択すると、「ビューコントロール」ウィンドウが表示されます。  
このウィンドウで、透視図のカメラの操作を行います。  
左上のアイコンは、左から「移動」「回転」「ズーム」になります。それぞれのアイコンをマウスクリックした状態でドラッグすることで、
透視図のカメラ操作を行います。  
<img src="https://github.com/ft-lab/Shade3D_plugin_ViewControl/blob/master/wiki_images/ViewControl_02.jpg"/>  
「移動」で、スクリーンに平行にカメラ移動させます。カメラの位置と注視点のどちらも動きます。  
「回転」で、カメラの注視点を固定としてカメラを周囲に回転させます。  
「ズーム」で、カメラの視線はそのままでカメラ位置を注視点から近づけたり遠ざけたりします。  

「カメラの向き」で、「透視図」「上面」「底面」「正面」「背面」「右面」「左面」を切り替えます。  
これは、透視図上でカメラの注視点を変えずにカメラ位置を変更するものになります。  

「選択要素に注視点を合わせる」ボタンを押すと、ブラウザで選択した形状の中心を注視点に変更します。  
形状編集モード時は、選択したポイントや選択されたポリゴンメッシュの頂点/稜線/面の中心が、カメラの注視点となります。  

表示されている「カメラ位置」「カメラ注視点」の座標は、ワールド座標での位置となります。  

## 制限事項

* カメラ操作のUNDO/REDOには対応していません。

## 更新履歴

[2017/06/25]  ver.1.0.0.0  
* 初回版

