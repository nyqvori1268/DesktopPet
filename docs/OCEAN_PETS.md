# v1.3.0 海洋寵物素材

來源：此任務附件 DesktopPet-v1.3.0-OceanPets-GIF(1).zip 中六款原創 GIF，原始畫布 418×627、每款六影格、每格 180 ms。沒有網路素材、重繪、縮小、重新量化或調色。原有 src/sprite.h 不變。

| 設定 ID／外觀 | 來源 | 共用裁切後尺寸 | 閉眼影格（從 0 起算） |
| --- | --- | --- | --- |
| pet=5, coat=0 經典藍海豚 | A1_Dolphin_ClassicBlue.gif | 349×460 | 2 |
| pet=5, coat=1 粉紅海豚 | A2_Dolphin_Pink.gif | 374×383 | 3 |
| pet=5, coat=2 灰藍海豚 | A3_Dolphin_GrayBlue.gif | 373×318 | 2 |
| pet=6, coat=0 深藍鯨鯊 | B1_WhaleShark_DeepBlue.gif | 396×305 | 2 |
| pet=6, coat=1 淺藍鯨鯊 | B2_WhaleShark_LightBlue.gif | 392×304 | 3 |
| pet=6, coat=2 灰藍鯨鯊 | B3_WhaleShark_GrayBlue.gif | 379×283 | 3 |

原有 pet=0／1／2／3／4 分別為貓／狗／自訂／鸚鵡／熊貓，ID 未改。鸚鵡 coat=0..2 為虎皮、3..4 為玄鳳，因此共七種對話物種。海洋同物種三配色共用台詞。

## 開發轉換

可選開發工具 Python＋Pillow：`python tools/Convert-Ocean.py <原附件.zip>`。只在重新產生指定素材時使用；正常 CMake／MSVC 建置不需要 Python、Pillow 或原 GIF。

流程：GIF 正確合成各影格 → 只移除小於 max(12, 最大連通區像素數/2000) 的孤立 alpha 雜點 → 每款全部影格共用同一聯集裁切框 → 不改像素的調色盤索引／PackBits → Win32 RCDATA。此附件 36 影格實際未找到需刪除的孤立區，移除像素數均為零。所有透明像素正規化為零，不影響可見圖樣；不做逐影格縮放／重定位，保留原有原地漂浮及擺尾。

assets/ocean/provenance.json 記錄原始 GIF SHA-256、裁切框、時長、逐影格 RGBA SHA-256、原生 ARGB CRC32 與移除像素數。六個 .ocp 總計 1,373,438 bytes（約 1.31 MiB），只展開目前角色，最大一款原始 ARGB 影格快取約 3.68 MiB；這不是整體程序記憶體實測。

## 動畫及封裝

海豚保留原始待機節奏，鯨鯊以原節奏 60% 速度播放。開心／睡醒可略加速；眨眼／睡眠直接用各款原有閉眼影格，睡眠停止游動並慢呼吸。撫摸／開心使用既有愛心，生氣用輕微抖動與符號；拖曳／回彈及提醒跳躍沿用既有共用程式，不新增貓狗四肢，不主動桌面移動。

主程式只以最近鄰取樣海洋像素；原自訂 PNG／GIF 繪製方式不變。資源 201..206 僅連結進 DesktopPet.exe，不連進 Updater.exe。既有五檔自動更新白名單及兩種 ZIP 結構不變，沒有新解碼器或常駐服務。
