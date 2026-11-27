# 驗證紀錄與 Windows 驗收

環境：2026-11-25，Linux x64，GCC 14.2.0。沒有 CMake、MSVC、Windows SDK、Windows GUI 或 PowerShell。本次沒有建立／執行 Windows EXE，也沒有執行 GitHub Actions。以下將已執行結果與待執行項目分開記錄。

## 本次已執行

- GCC C++17，`-Wall -Wextra -Werror -pedantic` 編譯核心測試成功。
- 核心測試通過（323,381 次檢查；Debug 與 `-O2 -DNDEBUG` 均執行）。計數涵蓋大量逐像素／ZIP 截斷檢查，**不是三十多萬個獨立功能案例**。
- AddressSanitizer + UndefinedBehaviorSanitizer 核心測試通過（`ASAN_OPTIONS=detect_leaks=0`）。本環境 LeakSanitizer 因 ptrace 限制無法運作；未宣稱已通過記憶體洩漏檢查。
- `tools/export_assets.cpp` 編譯並匯出原創圖集及 ICO 成功。
- JSON 對話與 Windows manifest XML 語法可解析；PNG／ICO 可由 Pillow 解碼。

核心檢查包括 JSON 格式／大小／深度／重複 key／Unicode、設定預設及界限、更新狀態 roundtrip、一次倒數／暫停／恢復、循環時間相位、睡眠跨越多週期、延後不改週期、當地鬧鐘／每日／指定星期、嚴格正式版本比較、十種角色與貓狗剪影、各動作 alpha／像素範圍、彈簧收斂、CRC32、ZIP 正常白名單／路徑穿越／重複／截斷／符號連結拒絕，以及固定亂數 JSON 輸入。

## 已建立、尚未執行的 Windows 自動檢查

`ctest -C Release` 包含核心測試及 `windows_tests`：

- BCrypt SHA-256 標準向量。
- 產生暫時測試金鑰，驗證 P-256 正確簽章、遭改訊息／簽章／長度拒絕。
- Unicode 轉換、Windows 程序參數 quoting、UTF-8 設定原子寫入。
- 未簽署 manifest 拒絕。

`tools/Package.ps1` 另呼叫原生 ZIP 白名單／CRC 檢查；`-Sign` 加上 .NET 簽署 → BCrypt 驗證，驗證發布端與客戶端格式一致。Actions Release 建置使用 `/MT` 並檢查額外 VC Runtime DLL 依賴。這些只有在 Windows CI 成功後才能標記通過。

## Windows 11 x64 實機待測

| 項目 | 步驟／預期 | 狀態 |
|---|---|---|
| Release 與免 Runtime | 在無 VS／VC Runtime 的乾淨電腦完整解壓 ZIP，直接執行兩個 EXE | 待測 |
| 十種角色、原創美術 | 切換貓狗與五種毛色；比較圖集及桌面顯示，計時不中斷 | 待測 |
| 動畫 | 各動作、五分鐘睡眠、呼吸、眨眼、尾巴、睡醒、提醒跳躍 | 待測 |
| 點擊與撫摸 | 單點對話四秒，四連點生氣，來回游標愛心，避免同句連續 | 待測 |
| 拖曳 | 慢快、斜向、多螢幕拖曳，放手回彈；拖曳途中觸發提醒後才顯示 | 待測 |
| 三種時間模式 | 秒級倒數、循環、單次／每日／指定星期鬧鐘，開始／暫停／繼續／重設／停止 | 待測 |
| 延後與休眠 | 5／10／15／自訂分鐘；跨越多週期睡眠僅補一次，維持原相位 | 待測 |
| 提醒焦點 | 其他程式輸入中提醒，焦點不移動；跳躍後持續氣泡，點擊確認 | 待測 |
| 隱藏與提醒 | 手動隱藏／Alt+F4／全螢幕仍計時，系統匣可讀全文確認，恢復時呈現 | 待測 |
| DPI／多螢幕 | 100／150／200% 混用、負座標、拔螢幕、改解析度，不永久移出工作區 | 待測 |
| 穿透與置頂 | 透明像素可點後窗，整體穿透可從匣解除，置頂及氣泡同步 | 待測 |
| 設定 | 重啟保留；缺漏／損毀／新版本 schema 使用預設；唯讀目錄 fallback | 待測 |
| 開機啟動／單一程序 | HKCU Run 啟用停用、不需管理員；第二次開啟不新增寵物 | 待測 |
| 更新通知 | 啟動／24 小時／手動；停用、稍後、容量、內容 | 待測 |
| 下載 | 慢網路、離線、API 失敗、取消、逾時，UI 可操作 | 待測 |
| 更新驗證 | 正常、錯 key、改簽章／manifest／ZIP、降版、先行版、架構不符、大小超限 | 待測 |
| 回復 | 新版啟動失敗；備份中／替換中／就緒前殺掉更新器，重啟回復 | 待測 |
| 更新自身及計時 | Updater.exe 一起換新版；倒數／暫停／循環／鬧鐘／待確認／延後跨更新恢復 | 待測 |
| 權限／故障 | 寫入拒絕、檔案鎖定、磁碟不足、reparse 路徑拒絕並保留可回復資料 | 待測 |
| 無聲與網路 | 無任何音訊模組；關閉更新後以網路監測確認零主動連線 | 待測 |
| CPU／記憶體／容量 | 乾淨系統量測待機、睡眠、拖曳、隱藏至少五分鐘；記錄 CPU 與 Working Set | 待測 |

效能目標尚未實測，不能宣稱達標。打包腳本會拒絕超過 15 MiB 的 ZIP，但目前並無已成功建置的 Windows 發布容量數據。


## v1.5.2

新增 `v152_modes_editors`（模擬時鐘並行／佇列、三語草稿、動作隔離及透明度／螢幕邊界）、`v152_native_ui`（Windows 控制／預覽／捲動及兩 EXE Icon）與可用 Python 開發環境下的 `v152_brand_sources`。保留全部原 CTest／安全檢查；執行情況及 Windows 實機清單見 [V152_TESTING.md](V152_TESTING.md)。
