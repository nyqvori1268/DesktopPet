# v1.5.0 驗證紀錄

日期：2026-11-26。全新 clone 的 main／正式 v1.4.0 Tag 同為 `d9c510071e9db3520654a0e9089d40dedb3fc0d0`；完成前再次 fetch 確認未變。先讀 AGENTS.md，沒有將其他舊工作區覆蓋到此基準。版本只從 CMake 更新為 1.5.0，沒有 Commit、Tag、Release 或正式金鑰變更。

## 已執行

Linux x64，GCC C++17，`-O2 -Wall -Wextra -Werror -pedantic`：

| 測試 | 實際結果 |
|---|---|
| core_tests | 5,158,464 次檢查通過；設定、三種計時／延後、語言、物種隔離、22 外觀、JSON／ZIP／版本／信任規則 |
| v150_tests | 6,234 次檢查通過；三組×三語×八狀態、無損遷移／重啟／切換／同語言回退、翻譯完整性、AM／PM、全部 127 非空星期遮罩、跨日跨週、一次／每日／指定星期、快捷時間及模式上下限、暫停／延後剩餘時間 |
| renewed_tests | 1,598,110 次檢查通過；16 重製花色與六海洋外觀、時序及設定／計時相容 |
| security_tests | 4,141 次檢查通過；持久階段／信任收據、逐 I/O 中斷、重試回滾／清理、錯誤備份及無關檔案拒絕 |
| v150_tests ASan＋UBSan | 6,234 次檢查通過；`ASAN_OPTIONS=detect_leaks=0`，不宣稱完成洩漏檢查 |
| security_tool_tests.py | 敏感資料掃描器正反例／ZIP 路徑及遮蔽、Actions 固定 SHA／provenance 閘門通過 |
| renewed_gif_tests.py | 128 段既有 GIF 透明／循環／影格範圍／時序／花色／來源雜湊通過；沒有生成或修改素材 |

檢查次數包含逐像素及資料組合，不等於獨立功能案例數。所有時間測試均使用模擬秒數或本地 `mktime`，沒有實際等待鬧鐘。

`git diff --check` 通過；與基準逐位元比對確認144 個正常素材雜湊、data、build.yml、ECDSA／DPJ3／DPAPI／交易／信任／下載／ZIP／程序交接、原有安全測試及雙 ZIP 腳本未變。Windows 原生測試只更新兩處素材幀數／來源檔斷言，DPJ3 竄改／備份保留及其他安全檢查原樣保留。build.yml 的兩個 EXE 雜湊輸出仍在。只因新增原生 UI 使用系統 DLL，在 Test-PE.ps1 精確加入 dwmapi、uxtheme，原有防護及非允許 DLL 拒絕規則保留。敏感資料與交付包掃描為模式檢查，不是私鑰不存在的數學證明。

## 可重現指令

在儲存庫根目錄執行：

```sh
mkdir -p build-local
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/core_tests.cpp -o build-local/core_tests
./build-local/core_tests
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/v150_tests.cpp -o build-local/v150_tests
./build-local/v150_tests
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/renewed_tests.cpp -o build-local/renewed_tests
./build-local/renewed_tests
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -Isrc tests/security_tests.cpp -o build-local/security_tests
./build-local/security_tests
python3 tests/security_tool_tests.py
python3 tests/renewed_gif_tests.py
python3 tools/Check-Sensitive.py
```

GIF 額外開發檢查使用 Pillow，程式執行及 CMake 不依賴 Python／Pillow。CMake 現已註冊 v150_dialogue_timer、v150_animations 與 Windows 專用 v150_native_controls，既有 CTest 項目全部保留。

## 尚需 Windows 驗證

本環境沒有 CMake、MSVC、Windows SDK、PowerShell 或 Windows 桌面。未建立／執行 Windows EXE，未執行 GitHub Actions、正式簽署、PE 檢查、端對端更新或效能量測。

1. Windows Release 建置及完整 CTest（含既有 DPJ3 竄改／備份保留、安全與嵌入素材測試）。新增 v150_native_controls 驗證實際原生數字控制鍵盤／滑鼠循環、非法輸入、Unicode 編輯與設定超限時保留原檔；已加入但本環境未執行。
2. 三組各八狀態三語分頁：切頁未儲存緩衝、清空／預覽／儲存／取消、繁中／日文 IME 組字完成、重啟／角色切換、跟隨介面語言、空白語言通用回退、舊設定及一般對話開關；提醒自訂原文優先。
3. 實際 AM／PM、上下按鈕按住加速、滑鼠滾輪、方向／Page Up／Page Down／Home／End／Tab、直接輸入、每天／平日／週末／單次及星期摘要；24 小時自動歸零分秒、七天倒數上限、零秒／非法貼上拒絕。
4. 原任務在未套用編輯時持續，啟用後替換單一任務，暫停／繼續／停止／延後與自訂延後、睡眠喚醒及選單／設定關閉後待提醒正確呈現。
5. 三語長文字在縮放／調整視窗／捲動下可見；Windows 深淺色與高對比即時切換；100／150／200／300% DPI、不同 DPI 螢幕與負座標、置頂及穿透。原生選單與系統捲軸保留 Windows 自身繪製，不使用未公開的深色 API。
6. v1.4.0→v1.5.0 正式金鑰簽署更新、設定／三語台詞與計時恢復、故障回滾及兩種 ZIP／provenance／敏感資料檢查。Test-PE.ps1 確認實際 EXE 的 ASLR／DEP／CFG／GS／System32 載入政策及靜態 Runtime。體積、CPU、記憶體與完全無聲需實測，不宣稱已達標。


## 合併動畫追加驗證

- 新 animation_tests：11,890,825 次檢查通過，含 720 個海洋影格 CRC／透明邊界／動作路由／睡眠隔離／提醒停止跳躍、120 貓動畫影格路由、正常貓姿勢原 CRC、截斷／幀數／尺寸拒絕及資源釋放。ASan／UBSan 亦通過相同檢查（不含洩漏測試）。
- animation_asset_tests.py：144 個正常素材 SHA-256 不變；70 段新增／修正 GIF 的 840 個播放位置逐像素等於 OCP 解碼影格，透明邊界／循環銜接通過，最大 GIF 68,604 bytes。
- 已逐幀視覺檢視 188 段來源 GIF 的播放序列與 188 段 C++ 共用取樣離屏序列（每段 12 個時間點），涵蓋 22 外觀；修正五貓兩姿勢，其他素材未發現需要修改的同類重複肢體。允許自然透視遮擋，未把自動測試當作肢體結構驗收。
- 離屏原生取樣包含當前動作路由、最近鄰取樣、預設 96／144 幾何、一般效果及提醒跳躍；未執行 Windows 視窗／DPI／透明度／完整互動狀態機。
- 容量及 Linux 取樣 CPU 微基準記錄見 [V150_ANIMATIONS.md](V150_ANIMATIONS.md)。Windows EXE／雙 ZIP 大小、總程序 RSS／CPU 與 GUI 仍未驗證。
