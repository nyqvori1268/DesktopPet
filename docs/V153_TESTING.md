# v1.5.3 選單層級修正

## 基準

從 GitHub 重新 clone 最新 main，執行 `git fetch --prune origin`，確認 HEAD、origin/main、正式 v1.5.2 Tag 同為 `c6a79a89b74b5ada36fcc384c353d8d6a776e49b`，初始工作區乾淨，已讀 AGENTS.md 及發布文件。未使用先前工作區或 v1.5.2 增量包。

包含 v1.5.2 功能提交（1a0f8a4）及後續人工提交：3cf12ef（計時視窗品牌編譯修正）、dc045ae（外層捲軸操作）、c6a79a8（新容器原生測試）。`src/timer_window.h`、`src/form.h`、`tests/v152_windows_tests.cpp` 與本基準逐位元組相同。

## 原碼根因及處理

TrackPopupMenu 的巢狀訊息迴圈會繼續派送 WM_TIMER 及前景 WinEvent 所投遞的訊息。v1.5.2 的 maintainZOrder(true) 沒有選單保護，可在主／子選單存在時再次把 layered 寵物視窗插到 HWND_TOPMOST；每秒 visibility() 又會經由維護／顯示路徑干擾層級。只停止動畫計時器或隱藏氣泡／名牌不能阻擋這些呼叫。這是可由原碼確認的衝突；本環境無 Windows，尚未實機重現截圖中的 OS Z-order。

- `MenuLayer` 在原生追蹤前暫時將寵物降出 topmost，不改 Config.top、不儲存暫態狀態。scope 涵蓋整個 TrackPopupMenuEx，WM_ENTERMENULOOP／WM_EXITMENULOOP 使用同一狀態；退出子層或收到最後 EXIT 訊息時，外層 scope 仍阻擋提前恢復。
- maintainZOrder 的強制前景補正與每秒補正共用 guard；visibility 在此期間只繪製，不呼叫 ShowWindow 或重新判斷前景全螢幕；DPI／螢幕搬移使用 SWP_NOZORDER，名牌及氣泡沿用隱藏機制。沒有對寵物呼叫 SW_HIDE，原本手動／全螢幕已隱藏的狀態也不強制顯示。
- 動畫沿用原計時器／FPS 繼續渲染，既有短動作自然結束後回到待機；新互動、隨機動作與一般對話仍暫停。計時及提醒佇列繼續運作，選單結束再呈現待確認提醒。
- TrackPopupMenuEx 返回後，scope 以目前 Config.top 一次恢復層級，所有 SetWindowPos 均使用 SWP_NOACTIVATE。清理發生在執行所選功能之前，功能若更改置頂即套用新偏好；不會將原本關閉置頂的寵物強制置頂。
- 保留 Win32 系統匣選單所需的明確使用者操作 foreground owner／WM_NULL 流程。新增條件式歸還：只有選單結束後 foreground 仍是寵物 owner，才回到開啟選單前視窗；外部點擊已改 foreground 則不碰焦點。不新增背景 SetForegroundWindow、AttachThreadInput 或 SetFocus 迴圈。

Win32 選單 foreground／外部點擊行為依 [TrackPopupMenu 官方文件](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenu)。不是刪除 foreground handshake 來換取錯誤的選單取消行為。

scope 狀態固定大小、無 heap／GDI 配置，沒有新增 timer、WinEvent hook、輪詢、素材或依動物分支；同一主視窗路徑適用 22 款內建及三組自訂。CPU／RAM／GDI 實際數據仍需 Windows 驗證，不能由結構分析宣稱達標。

## 測試

原 15 個 CTest 名稱及設定全保留，另加 v153_menu_state、v153_name_persistence（可跨平台）及 v153_native_menu（Windows），共 18 項（原可選 Python 測試仍需找到 Python）。原 v152 品牌測試只同步檢查的新程式版本為 1.5.3，Icon／CRC／安全／其他斷言不變。

Linux：以 GCC C++17、-O2 -Wall -Wextra -Werror -pedantic 編譯執行九組 C++ 測試（既有七組＋v153 選單及名稱），執行原品牌／安全工具／GIF 與來源雜湊驗證。v153 另執行 ASan／UBSan，detect_leaks=0，不宣稱洩漏檢查。最終檢查數量及打包驗證見 APPLY.md。

新增 portable 測試：原生通知與 tracking scope 的巢狀狀態、重入拒絕、未配對退出／失敗清理、十萬次重複操作；production 的前景／顯示／DPI 保護、動畫持續及自然完成、焦點歸還條件；原 15 項測試註冊與多螢幕判斷不退化。

新增 Windows 測試已註冊但本環境未執行：實際 layered owner 播放原海洋 OCP，主選單及兩層子選單的原生 Z-order、owned badge、動畫持續與維護抑制、ESC／命令／activation-cancel、關閉子選單仍維持 guard、原置頂開／關還原、例外與期間偏好變更清理、焦點歸還及重複開關 GDI 物件不累積。activation-cancel 不代替真正滑鼠點擊外部的人工驗收。

## Windows 待驗證

本環境沒有 MSVC、Windows SDK、CMake、PowerShell、Wine 或 Windows 桌面，未編譯 Windows EXE、未執行原生 GUI／多螢幕測試，也未執行 GitHub Actions。

1. Windows Release＋完整 18 項 CTest，確認原 15 項（包含人工修正的捲軸／控制項）皆保持通過；Test-PE.ps1 驗證實際產物防護。
2. 寵物／系統匣右鍵主選單、多層子選單、子層開關、ESC、點空白／其他程式、功能選擇、反覆開關，確認文字不被 GIF 或名牌遮住且無閃爍／殘影。
3. 置頂開及關、選單中改顯示設定／DPI／螢幕、選單結束前景切換、非本程式輸入焦點不被搶回。需確認原問題軟體的實際 Z-order。
4. 全 22 款／三組自訂 GIF 在選單中持續播放，短動作自然結束；名牌、氣泡、拖曳／回彈、透明度、穿透、多 DPI／負座標、多螢幕最大化／真正全螢幕、手動隱藏後系統匣選單。
5. 三模式並行、選單中同時到期、延後／確認及更新後恢復；所有設定與自訂素材不變。
6. 重複選單操作前後測量 CPU、working/private bytes 與 GDI／USER handles，不增加高頻置頂／重繪，不累積資源。原生 GDI 回歸未執行，不宣稱已確認無洩漏。

所有寵物／品牌素材、更新協定、ECDSA／SHA-256／DPAPI／DPJ3／信任／回滾、DLL 防護、MSVC 安全旗標及 build.yml／provenance／EXE SHA 輸出保持不變。未針對防毒偵測調整程式特徵，未建立 Commit、Tag 或 Release。


## 追加：自訂名稱更新後遺失

再次 fetch 核對 origin/main，仍為上述 c6a79a8，保留同一份選單修正，未重設工作區。

確認範圍：builtinIdentity 的 22 個索引自 v1.5.1 至最新 main 相同；Updater 只安裝簽章驗證後的白名單五檔，config.json 不在其中，交易／回滾未寫入它。因此沒有把問題歸因於更新器覆蓋設定。正常且完整的 builtinNames roundtrip 原測試可通過。

找到的兩個讀取／遷移缺口：

- 原 settingsPath 只根據本次程式目錄可否寫入決定讀取位置，不看既有設定；可寫的新版本資料夾缺 config 時，即使 LocalAppData 有先前設定，也會用預設值重新存檔。原本只存於舊可攜資料夾的設定，搬資料夾時也沒有穩定復原副本。這是原碼確認的讀取來源問題，並非 updater 安裝白名單變更。
- 舊設定只要帶 builtinNames 陣列，即使為空或缺當前外觀欄位，load 都不再看既有 name／builtinName。Linux 最小重現：粉紅海豚的 name=「粉粉」、builtinNames=[]，舊碼載成「桃泡」，修正後保留「粉粉」。

本次修正：configPaths 分離讀取來源與可寫目的地，考慮既有 portable／LocalAppData 檔與修改時間；readConfig 先在候選副本完整解析，失敗嘗試另一份，不部分改動目前 Config。可寫目錄仍建立可攜 config，saveConfig 同步原子復原副本（主檔先成功，再寫副本），啟動 checkpoint、正常關閉與更新 handoff 都使用同一流程。備份失敗仍回報原儲存錯誤，不宣稱磁碟故障時可保證成功。

索引名稱與三組 profile 維持原格式／識別 ID；缺舊索引時只把已知目前外觀的非預設別名補回，不覆蓋既有獨立名稱。新增 nameSchema=1 名稱欄位標記（主 schema 仍為 1），使新設定的空槽明確表示預設／主動重設，防止舊別名復活。舊版已明確設定、即使字面等於預設名的索引 override 仍完整保留；缺索引但看似三語預設的舊投影不誤當自訂。pre-index 格式維持既有相容遷移。使用者舊長名保持原儲存上限內原文，新改名仍驗證 12 單位。沒有改動 IME、名稱編輯器、名牌或素材。

新增 v153_name_tests：所有 22＋3 名稱、每個外觀作為活動身分的正常／更新重啟、三語切換、空／短／完整舊陣列遷移、明確重設與字面預設自訂名、索引優先於舊別名、其他桌面／台詞／素材欄位不變、設定來源／寫入權限組合。Linux 579 checks 及 ASan＋UBSan 均通過（detect_leaks=0）。原全部測試保留；Windows v153_native_menu 另加入隔離暫存目錄的真實原子主檔／副本、讀取恢復、重設及新版本資料夾測試，尚未執行。

Windows 待測追加：真實權限及檔案時間選擇、portable ↔ LocalAppData、新版本解壓目錄、正常關閉／更新重啟後全部名稱、IME 與名牌、主檔損毀 fallback、復原副本寫入失敗及三模式更新狀態。原碼缺口已確認，但使用者個別電腦先前讀取哪個檔案、是否仍有舊名稱，仍需對照實際設定。過去已覆寫的名稱若沒有任何副本不能推算；既有舊可攜檔第一次搬資料夾仍應一併攜帶，此後新版會同步復原副本。不擅自掃描其他資料夾。
