# v1.5.0 動畫修正、擴充與人工驗收

與三語自訂台詞、12 小時鬧鐘及計時 UI 合併，基準仍為正式 v1.4.0／main `d9c510071e9db3520654a0e9089d40dedb3fc0d0`。不另升版、不建立 Tag／Release。無線上素材或執行期網路／GIF 解碼依賴。

## 貓咪：只修正兩個問題姿勢

根因在原始 cats.png 的圖樣，並非解碼或透明疊圖殘影。舊哈欠同時畫了抬起與落地的兩組前腳；翻肚少了一個清楚的後腳輪廓。

新增 `assets/renewed/source/cat-corrections.png`（5 花色×2 姿勢），使用既有原創圖樣為參考，內建圖像生成工具只修正這兩類姿勢，再以最近鄰及原款調色盤轉為 128px。橘、三花、白、黑、虎斑全部修正。哈欠為兩抬起前腳＋兩後腳；翻肚呈現四個肉球腳，保留腹部與單一尾巴。循環加入局部爪／尾／耳與呼吸位移，不留下舊肢體層。

| OCP 影格 | 用途 | 相容處理 |
|---|---|---|
| 0、1、2、4 | 原待機、眨眼、睡眠、伸展 | 像素 CRC 與 v1.4.0 完全相同 |
| 3、5 | 修正後哈欠、翻肚基礎姿勢 | 同角色／花色索引及資源 ID |
| 6～17、18～29 | 兩段各 12 幀循環 | 100 ms／幀，原生路由及實際 GIF 使用相同像素 |

僅改五個貓 OCP、十個對應 GIF 與清單；其他 118 個正常陸生 GIF、11 款狗／鳥／熊貓 OCP、舊來源圖集不變。144 個未修改素材的 SHA-256 鎖定於測試基準檔。

## 六款海洋動畫

保留六款造型、pet=5／6、coat=0..2 及資源 ID 201..206。原 ocean-*.ocp／provenance.json 逐位元保留；新增 marine-*.ocp 供目前主程式內嵌。來源為以各款既有原創角色為參考的 12 key poses，按原款調色盤處理；沒有下載其他角色美術。規格及來源雜湊見 `assets/ocean/source/prompts.json`、`preparation.json`、`animations.json`。

| 區段 | GIF | 原生動作 | 動畫重點 |
|---|---|---|---|
| 0～11 | idle | Idle／Drag | 鰭尾局部游動、腹部呼吸、間歇眨眼 |
| 12～23 | blink | Blink | 閉眼／微笑後睜眼，保留原身體及斑點 |
| 24～35 | sleep | Sleep | 全程固定閉眼睡姿，慢鰭尾與呼吸；不返回清醒姿勢 |
| 36～47 | cuddle | Happy／Shy | 抱腹／害羞、局部揮鰭及愛心 |
| 48～59 | angry | Angry | 鼓臉、眉眼及強力鰭尾動作 |
| 60～71 | heart | Pet | 開心微笑、揮動胸鰭與愛心 |
| 72～83 | remind | Remind／Surprise | 張嘴、鰭尾、泡泡；原生 1.8 秒連跳後停在最後影格待確認 |
| 84～95 | roll | Yawn | 海豚轉圈，同時擺鰭尾；鯨鯊溫和翻身 |
| 96～107 | bubbles | Stretch | O 嘴、泡泡及局部鰭尾 |
| 108～119 | special | Belly | 海豚俏皮翻搖，鯨鯊濾食／泡泡 |

每段實際 GIF 為 128×128、12 幀、無限循環；一般海豚 100 ms、鯨鯊 150 ms，睡眠分別 400／600 ms，提醒 150 ms。最大檔 68,604 bytes，均低於 100 KiB。動作包含胸鰭／尾鰭／腹部的局部連續變化，未僅以整張平移／旋轉代替；轉圈是在局部鰭尾動作之外增加旋轉。

海洋動作按片段完整時長播放，避免新眨眼片段被原先 250 ms 的短動作時限截斷。特殊動作沿用原隨機待機選擇，另加 90～150 秒冷卻，仍須等既有待機觸發；無桌面自主移動。拖曳／回彈、提醒優先及隱藏／選單的待提醒機制保留。

## 人工逐幀檢視與預覽

已將全部 188 段來源 GIF 展開為每段 12 個等時播放位置（涵蓋全部影格），並檢視 22 張序列表；另檢視 188 段 C++ 離屏序列的 22 張序列表。五貓兩問題姿勢皆呈現四肢；狗／熊貓保留兩前肢兩後肢，鳥保留兩腳兩翼及尾羽。睡眠／伸展的遠側肢體可自然被遮擋，未為了顯示數量而補畫多餘肢體。海豚保留胸鰭、單一背鰭、水平尾鰭造型；鯨鯊為胸鰭、單一背鰭、垂直尾鰭。檢視斑紋、輪廓、透明邊界及首尾銜接，正常動物沒有重製。

預覽包 `DesktopPet-1.5.0-animation-preview.zip`：解壓開啟 `index.html`，可離線直接播放 188 段來源及 188 段離屏原生 GIF，包含全部五貓修正與六海洋十動作，另附逐幀表。不是只有靜態拼圖。

`native-sampler` 使用與主程式相同的 `blitNativePet`／最近鄰取樣及真實動作影格路由，按預設 96 邏輯像素／144 畫布、呼吸幾何、一般效果與提醒跳躍輸出。它是隔離動作的離屏檢視，沒有模擬完整視窗狀態機，也不是 Windows 桌面／DPI／透明度／效能的實機驗收。來源 GIF 的獨立循環與原生 Blink 等短暫狀態的播放方式可能不同；實際互動切換仍須 Windows 檢查。

## 容量及效能範圍

| 內嵌 OCP 來源 bytes | v1.4.0 | v1.5.0 |
|---|---:|---:|
| 六款海洋 | 1,373,438 | 2,790,169 |
| 十六款陸生 | 541,344 | 1,392,489 |
| 合計 | 1,914,782 | 4,182,658 |

原生資源資料增加 2,267,876 bytes（約 2.16 MiB）；EXE／ZIP 還有程式碼、圖示、對齊及文件，不把這個數字當作實際發布體積。原始 PNG／GIF／開發工具不納入正式雙 ZIP，仍經 DesktopPet.exe 內嵌。完整發布小於 15 MB 的目標須等 Windows 建置／打包量測。

只解碼目前使用的一款，不同角色快取會清除。貓咪 30 幀 ARGB=1,966,080 bytes（1.875 MiB），原為 393,216 bytes；海洋 120 幀=7,864,320 bytes（7.5 MiB），舊各款約 2.45～3.68 MiB。這是像素陣列精確計算，不是整體程序 RSS／工作集或峰值實測。OCP1 仍限制編碼檔 4 MiB；新影格數只接受 128px／30 或 120 幀，解碼上限 8 MiB，舊六幀維持原 6 MiB 限制。

Linux GCC `-O2` 的離屏取樣微基準，各款各 5,000 幀（144 畫布、96 寵物、實際像素取樣與清除）：舊資源每幀 CPU 約 32.7～45.7 µs，新資源約 44.7～51.9 µs。此數字只涵蓋取樣內核，並非 Win32 合成／UI／整個程式 CPU，也不宣稱達到 Windows 的 CPU／記憶體目標。執行 `animation_tests --benchmark` 可重測；隱藏停止繪製及現有繪製頻率保留。

## 重建與測試

```sh
python3 tools/Build-V150-Animations.py
python3 tests/animation_asset_tests.py
python3 tests/renewed_gif_tests.py
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/animation_tests.cpp -o build-local/animation_tests
./build-local/animation_tests
./build-local/animation_tests --export build-local/native-review
python3 tools/Export-Animation-Review.py build-local/native-review build-local/animation-preview
./build-local/animation_tests --benchmark
```

從既有已標準化 source PNG 可重現相同 OCP／GIF，只寫入本次指定素材；Pillow 僅為開發工具，不是程式依賴。原圖像生成的完整原始輸出仍在本地生成工作區，Git 收錄清理後、可直接維護的來源圖集及原始生成 SHA／提示詞。

已通過 C++ 動作／像素／邊界／畸形資料／正常姿勢回歸及 ASan／UBSan、70 GIF／840 影格原生像素一致性、144 正常素材雜湊、原核心／安全／三語計時／128 陸生 GIF 回歸。測試不能辨識肢體意義，已另行進行上述視覺檢視。

Windows MSVC Release、真正的 layered window／高 DPI／多螢幕／縮放、互動交接、簽署更新與失敗回滾、EXE 內嵌資源測試、總程序 CPU／RSS／大小仍待實機驗證。ECDSA／DPJ3／DPAPI／SHA-256、信任鏈、更新白名單、最新 build.yml 的 EXE 雜湊／provenance 及原安全測試均保留。
