# v1.5.1 修正與驗證紀錄

基準：main／正式 v1.5.0 Tag `522856486320e504669ebe060882bb55dcee519a`。全新 clone，先讀 AGENTS.md；不採用先前舊工作區，也沒有修改任何 GIF／OCP／來源素材。版本從 CMake 單一來源設為 1.5.1。未建立 Commit、Tag 或 Release。

## 實作與根因

- 分頁原本固定 528 寬、每項 168，再加原生內距，容易出現 overflow。新版以實際客戶區計算等寬項目、清除多餘 padding 並使用 multiline tab style 防止水平箭頭；正常最小大小下保持一列。編輯器採 WS_OVERLAPPEDWINDOW，可縮放／最大化；說明放上方、編輯區與頁尾依客戶區布局，保存正常客戶區邏輯大小（480～1600 × 320～1200），最大化不覆寫正常尺寸。
- 計時視窗原本 WM_ERASEBKGND 直接回傳已處理、父視窗未裁切子控制項；逐個搬移時可複製舊像素，又只 invalidate 父視窗，導致背景與字形不同步。現採 WS_CLIPCHILDREN／CLIPSIBLINGS、DeferWindowPos、SWP_NOCOPYBITS／NOREDRAW，移動完成才一次重繪父與子；背景在 WM_PAINT 使用記憶體 DC／Bitmap 雙緩衝，還原及釋放 GDI 選入物件。DPI 更新先暫停中間布局，最後統一更新字型／重繪。
- WM_VSCROLL 的子控制項通知（lParam 非零）不再當作視窗捲軸；NumberWheel 吞掉自身滾輪事件，視窗保留高解析度滾輪餘數。既有 1 秒狀態計時器不新增頻率，文字相同不重設，重繪只由捲動、縮放、DPI、主題及真正內容變化觸發。
- 名牌為 owned layered popup，使用既有 GDI+ 的 PARGB 文字與圓角，NOACTIVATE／TRANSPARENT／HTTRANSPARENT，不攔截寵物或後方視窗輸入。位置以固定 canvas 為錨點、與動畫影格無關；優先下方並保留底部空間，再依工作區嘗試其他外側位置，必要時避開氣泡或暫藏。DPI、透明度、縮放、置頂跟隨寵物；文字／字型／主題／尺寸未變時不重建像素，不新增名牌計時器。
- 滑桿採原生 Trackbar、50～300 與數字雙向驗證，增加 ICC_BAR_CLASSES 初始化，沒有新增 small 識別字。輸入每批最多 33 ms 一次預覽；不是反覆重新啟動的 debounce，連續拖曳仍會更新。確定時 flush 最後值，取消還原原大小／位置；背景計時 checkpoint 也保存原預覽前大小，不把暫存預覽當設定。

## 設定相容

schema 仍為 1。新增 `builtinNames` 長度 22 的字串陣列，依既有 species／coat 身分索引；空字串表示跟隨三語預設名。`name`／`builtinName` 等舊欄位仍輸出供舊版讀取。

舊格式沒有每外觀名稱，因此只將已知目前名稱綁定該款；若目前為自訂寵物且舊格式無上一款內建身分，舊 builtinName 保留在橘貓。其餘外觀各用預設名稱，避免新的切換功能再次將同名覆蓋所有外觀。三組自訂檔保留原 name，另以 `nameDefault` 區分預設／使用者輸入；舊有明確名字一律視為使用者原文，不猜測或翻譯。恢復預設才改為跟隨語言。即使使用者手動輸入與預設完全相同的字串，也保留為使用者名稱。

新增 `showNameplate` 預設 true、`dialogueEditorWidth`／`dialogueEditorHeight` 邏輯尺寸；缺漏／異常皆有預設與界限。全部台詞、素材槽、桌面與計時資料保留。舊版可以讀既有欄位，但舊版另存會捨棄不認識的新欄位，降版前另備份設定。

## 已執行（Linux／GCC C++17）

以 `-O2 -Wall -Wextra -Werror -pedantic -Isrc` 編譯並實際執行：

| 測試 | 結果 |
|---|---|
| core_tests | 5,159,036 次檢查通過 |
| v150_tests | 6,416 次檢查通過，原三語／台詞／計時相容 |
| v151_tests | 2,284 次檢查通過；22＋3 獨立名稱、三語／遷移／序列化、分頁／編輯布局、負座標工作區／氣泡避讓與 50～300% 邊界 |
| renewed_tests | 1,598,110 次檢查通過，22 外觀／正常姿勢及計時相容 |
| animation_tests | 11,890,825 次檢查通過，素材路由／像素／安全邊界 |
| security_tests | 4,141 次檢查通過，交易中斷／信任提交／冪等回滾／備份及無關檔案拒絕 |
| security_tool_tests.py | 敏感資料掃描器正反例／Actions SHA／provenance 閘門通過 |
| animation_asset_tests.py | 144 份原素材 SHA、70 GIF／840 播放位置像素一致及透明／銜接通過 |
| renewed_gif_tests.py | 原 128 段陸生 GIF 驗證通過 |

新 v151_tests 的 ASan＋UBSan 已通過相同 2,284 次檢查。`ASAN_OPTIONS=detect_leaks=0`，不宣稱完成洩漏檢查。檢查次數包含逐像素／資料組合，不等於獨立功能案例。既有兩處「所有外觀共用名字」斷言改為「原外觀保留名字、其他各自預設」，其餘條件與檢查不刪除，另加入完整 22 款命名回歸。

CMake 註冊 v151_settings_layout 與 v151_native_ui，原全部 CTest／安全測試保留。版本、素材及安全檔案比對、Patch 真正套用及交付包掃描於打包時執行。

## 新 Windows 原生測試（未在本環境執行）

`v151_windows_tests`：

- 實際三語 Tab 一列／無溢出箭頭、拖大與最大化、Unicode 文字不遺失、分頁通知。
- 實際鬧鐘窗口快速上下捲動 60 次，每次立即比較自然結果與完整清除重畫的像素，檢查 GDI 物件不持續增加；檢查子控制項捲動與數字滾輪不改視窗捲軸。
- 實際 Trackbar／Edit 雙向同步、無效值拒絕、100% 重設及確定前最後預覽 flush。
- 實際名牌建立／放置、WS_EX_TRANSPARENT／HTTRANSPARENT 與隱藏。

## Windows 實機待驗證

本環境沒有 CMake、MSVC、Windows SDK、PowerShell 或 Windows 桌面，沒有建立／執行 Windows EXE 或 GitHub Actions。不能把 Linux 布局與資料測試當作 Windows GUI 已通過。

1. Windows Release＋完整 CTest；SDK 的 small 巨集與 Common Controls 編譯／初始化、Test-PE.ps1 的實際 ASLR／DEP／CFG／GS／System32 DLL 與靜態 Runtime。
2. 編輯器 3 語、480×320 至最大化與還原、視窗尺寸記憶、長台詞、切頁暫存／預覽／取消／儲存、日文／繁中 IME 組字過程中縮放與切頁。
3. 倒數／循環／鬧鐘快速滾動、拖 scrollbar／觸控板高解析度滾輪、數字 UpDown 滾輪與鍵盤；三語、深淺色、高對比、視窗縮放及 100／150／200／300% DPI／跨螢幕後無殘影／重疊／閃爍。
4. 22＋3 名稱、預設／自訂／恢復／重啟、改介面語言不翻譯自訂名；名牌拖曳／縮放／透明度／置頂／穿透／全螢幕隱藏／氣泡與提醒避讓、邊界／負座標／螢幕移除。極端無法容納外側名牌時以暫藏避免重疊。
5. 滑桿拖曳／鍵盤／手動輸入同步，確定保存與取消還原（含預覽中觸發提醒或更新 checkpoint），監看設定檔不隨滑動反覆寫入；CPU／RSS／GDI 及顯示品質實測。
6. v1.5.0 → v1.5.1 以既有正式金鑰簽署更新、設定／台詞／名稱／計時恢復、故障回滾及雙 ZIP／EXE 雜湊／provenance 驗證；不繞過任何安全檢查。
