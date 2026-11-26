# DesktopPet 1.5.0

Windows 11 x64 原生 Win32 桌面寵物。C++17、像素繪製、完全無聲，沒有帳號、遙測、廣告或常駐服務。除了可選的 GitHub 更新，不使用網路。開發者 Meow，© 2026 Meow。軟體與原創素材採 [MIT](LICENSE)；匯入第三方素材另依其授權，見 [元件與素材聲明](THIRD_PARTY_NOTICES.md)。

## 功能

- 橘貓、三花、白貓、黑貓、虎斑；赤柴、黑柴、白柴、柯基、米色垂耳犬。同時一隻，可命名、切換。
- 貓、狗、鸚鵡、熊貓已重製為原創日系像素圖集，16 種外觀及既有毛色索引全部保留；透明背景與最近鄰縮放；v1.5.0 只修正五種貓咪哈欠／翻肚並各加 12 幀循環，正常姿勢及狗／鳥／熊貓素材保留。128 段陸生 GIF 見 [素材文件](docs/RENEWED_PETS.md)。海豚／鯨鯊保留六款造型、舊來源及設定索引，另新增 60 段動作 GIF。
- 待機、睡眠呼吸、睡醒、哈欠、伸展、翻肚、開心、害羞、生氣、驚訝、撫摸愛心、拖曳伸縮、彈簧回彈及提醒跳躍。
- 繁體中文／English／日本語離線介面與對話、倒數、循環提醒、單次／每日／指定星期鬧鐘，自訂提醒與延後。只維持一個計時任務。
- 透明無邊框、不占工作列、系統匣、大小 50～300%、透明度 20～100%、置頂、穿透、隱藏、全螢幕自動隱藏、每螢幕 DPI。
- 原子 JSON 設定，優先可攜資料夾、不能寫入時改用 `%LOCALAPPDATA%\DesktopPet\config.json`。正常結束停止任務；更新重啟恢復任務。
- WinHTTP 背景檢查、ECDSA P-256 簽章、SHA-256、嚴格 ZIP 白名單、備份及啟動失敗回復。未設定儲存庫或公開金鑰時停用更新。

## 下載與執行

從 [GitHub Releases](https://github.com/nyqvori1268/DesktopPet/releases) 優先下載 `DesktopPet_v{version}.zip`（例如 `DesktopPet_v1.5.0.zip`），完整解壓到可寫入的資料夾，執行同名根資料夾 `DesktopPet_v{version}/` 內的 `DesktopPet.exe`。`DesktopPet-windows-x64.zip` 是自動更新專用包，保留原本平坦的五檔結構；一般手動下載請選版本命名包。`SHA256SUMS.txt` 列出兩種 ZIP 的 SHA-256。不要只取出 EXE；保留 `Updater.exe` 與 `data/dialogue.json`。無需安裝或管理員權限，MSVC Runtime 靜態連結。

**v1.5.0 開發交付。** 此修改包不含 Windows EXE，未建立 Tag 或發布 Release；本次已執行與待驗證項目見 [v1.5.0 驗證紀錄](docs/V150_TESTING.md)，正式編譯、簽署及雙 ZIP 打包沿用 [發布流程](docs/RELEASE.md)。

## 語言、對話與關於

右鍵「語言」即時切換繁體中文、英文或日文，不需重啟。新設定依 Windows 顯示語言選擇，不支援則英文；既有設定缺少語言欄位時保留繁體中文。內建寵物的「寵物對話語言」可跟隨介面，或獨立選擇三種語言；自訂寵物依介面語言選擇獨立台詞庫；使用者名稱及自訂提醒不會翻譯。

貓、狗、虎皮鸚鵡、玄鳳鸚鵡、熊貓、海豚及鯨鯊各有八種狀態的三語台詞，同物種毛色共用。缺漏僅使用通用台詞，絕不套用其他物種。「顯示一般對話」可關閉閒聊，計時提醒仍會出現。提醒內容優先使用你設定的文字；文字留空時才使用提醒台詞。

三組自訂寵物各有「編輯台詞」：先選一般、開心、撫摸、生氣、拖曳、睡覺、睡醒或時間提醒，再切換「繁體中文／English／日本語」分頁，逐行新增、修改、清空或預覽，各語言獨立儲存。未設定的語言／狀態使用同語言通用台詞，不借用其他語言。舊版台詞無損保留在繁中，英文及日文預設空白；使用者原文不翻譯。各語言各狀態最多 24 句，每句 360 UTF-8 位元組，總設定受既有 1 MiB 安全界限保護（預留其他設定空間）。可獨立關閉一般對話，提醒不受影響。設定、名稱及素材在重啟、切換及更新後保留；清除整組才刪除該組全部語言。詳見 [自訂寵物文件](docs/CUSTOM_PETS.md)。

「關於」為原生可調整視窗，顯示版本／架構、Meow、Copyright、MIT 全文、實際元件與素材聲明，以及專案／問題／版本紀錄連結與檢查更新。版號由 CMake 生成，不需修改資源檔。

## 海洋寵物（v1.3.0）

「更換寵物」新增海豚、鯨鯊；「更換毛色」可選經典藍／粉紅／灰藍海豚，以及深藍／淺藍／灰藍鯨鯊。保留原有十六外觀與三組自訂寵物，共七種物種、二十二種內建外觀。物種台詞隔離，八個互動狀態都有三語多句台詞。

六款指定原創 GIF 的舊 OCP／來源紀錄完整保留，v1.5.0 依各款原造型與調色盤新增待機、眨眼、固定閉眼睡眠、撒嬌、生氣、愛心、提醒、轉圈／溫和翻身、吐泡泡及俏皮搖擺／濾食。每段 12 幀、透明 128×128 px，單檔均低於 100 KiB；鯨鯊與睡眠播放較慢。胸鰭、尾鰭與腹部使用局部連續動作，轉圈同時保留鰭尾動作；不主動桌面移動。特殊動作透過既有隨機待機觸發且間隔至少 90 秒；提醒 1.8 秒後停留最後影格，氣泡保留到確認。

新海洋 OCP 合計 2,790,169 bytes，與陸生素材合計 4,182,658 bytes，比 v1.4.0 多 2,267,876 bytes；這是內嵌來源資料大小，不是 EXE 或 ZIP 實測。僅展開目前角色，海洋 ARGB 快取為 7.5 MiB、貓咪 1.875 MiB，其他陸生仍 0.375 MiB；不等於整個程序記憶體。新影格沿用 OCP1／RCDATA、原資源 ID 及五檔更新白名單，不新增 GIF 解碼器或 Runtime。轉換、逐幀檢視與可播放預覽方式見 [v1.5.0 動畫說明](docs/V150_ANIMATIONS.md)。

## 自訂寵物 PNG／GIF

右鍵選單 →「自訂寵物 1／2／3」→「匯入待機／更換角色…」選擇本地檔並輸入名稱；二十二種內建外觀保留，可從「更換寵物」切回。重新匯入待機檔會替換目前自訂角色並清除其動作設定，目前提供三個獨立設定檔，各組可命名、匯入七種動作、清除及切換；舊資料自動遷移第一組，只快取使用中的一組。

各組選單可指定待機、眨眼、睡覺、撒嬌、生氣、愛心、提醒七個 PNG／GIF。保留原畫、透明背景，依所有影格的 alpha 聯集裁切，平滑縮放；沒有動作素材時使用呼吸、短暫壓縮、伸縮、搖動、愛心／睡眠符號及既有提醒跳躍。**缺少眨眼原畫時只做短暫壓縮，不會猜測並重繪角色眼睛。**

素材複製至 `%LOCALAPPDATA%\DesktopPet\custom-pets`，完全離線播放；更新不覆蓋該資料夾。每檔上限 8 MiB／2048×2048／120 影格，完整解碼畫布總量限 16 MiB，每個動作快取限約 2 MiB（最多邊長 256）；較大素材會在裁切後縮小。僅裁掉透明留白，不自動去除不透明背景。

小桃（モモンガ）的官方預覽只作本地測試，未取得再散布許可，不包含於原始碼或發布 ZIP。授權來源、限制及測試方式見 [自訂素材文件](docs/CUSTOM_PETS.md)。

## 操作

| 操作 | 效果 |
|---|---|
| 左鍵點一下 | 隨機動作、4 秒對話；睡眠時叫醒 |
| 快速連點四下 | 鬧脾氣 |
| 游標來回移動 | 五次方向變化觸發撫摸愛心 |
| 按住拖曳、放手 | 隨速度與方向伸縮，彈簧阻尼回彈 |
| 提醒時點寵物或氣泡 | 確認提醒 |
| 右鍵寵物、系統匣圖示 | 完整選單、計時狀態、設定、更新、結束 |
| 系統匣 → 滑鼠穿透 | 解除穿透；透明像素本來就不攔截滑鼠 |
| 關閉寵物視窗（Alt+F4） | 隱藏，計時仍繼續；選單「結束程式」才退出 |

計時視窗採原生數字滾輪、柔和卡片與醒目時間數字，可調整大小、垂直捲動，跟隨 Windows 應用程式深／淺色及高對比設定。滾輪／上下箭頭／Page Up、Page Down／Home、End 與原生上下按鈕可調整數字，Tab 切換欄位，也可直接輸入。倒數及循環提供 5／15／25／30／60 分鐘捷徑；倒數上限維持七天，循環為 1 秒至 24 小時，24 小時時分、秒自動歸零。鬧鐘為 1～12 時、AM／PM、0～59 分；12 AM 是午夜、12 PM 是中午。星期一至日獨立勾選，提供單次／每天／平日／週末捷徑，下方顯示時間及星期摘要。全部取消星期即為單次；內部排程仍用原有 24 小時及星期位元格式。

視窗內顯示目前任務、剩餘時間及啟用／暫停／停止／待確認狀態；按「開始計時／設定鬧鐘」才替換目前任務，可繼續檢視、暫停／繼續、停止或延後 5／10／15 分鐘。自訂延後及重設沿用右鍵選單。編輯尚未套用前，原任務仍繼續。循環延後不改變原週期，重疊輪次合併於延後截止時提醒，睡眠錯過多個週期只通知一次。提醒不搶焦點，氣泡保留到確認；選單或設定視窗開啟期間先保留，關閉後顯示。

預設橘貓「小橘」、96×96 邏輯像素、主螢幕工作區右下、完全不透明、置頂、全螢幕隱藏、自動檢查更新開啟；穿透及開機啟動關閉。五分鐘未互動睡眠，循環 30 分鐘、預設延後 5 分鐘，計時器未啟動。

## 原始碼編譯

安裝 Visual Studio 2022 的「使用 C++ 的桌面開發」、Windows 11 SDK、CMake 3.21 以上。使用 Developer PowerShell：

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
pwsh -File tools/Package.ps1
```

輸出 `dist/DesktopPet-windows-x64.zip`；`/MT` 不依賴額外安裝 VC Runtime。CI 另檢查 EXE 匯入表與封裝格式，正式發布要求簽章。這不取代乾淨 Windows 電腦上的執行驗證。

Linux 僅測試可攜核心：

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -Isrc tests/core_tests.cpp -o core_tests
./core_tests
```

可用 `tools/export_assets.cpp` 重建原創 PNG 圖集與 ICO，僅開發時需要編譯此工具；程式執行與發布不需要 Python 或其他 Runtime。圖集五欄為毛色，十四列依 `Action` 列舉；實際動畫由原生程式連續計算。

## GitHub Repository、金鑰與發布

設定集中於 CMake：`DESKTOPPET_OWNER`、`DESKTOPPET_REPOSITORY`、`DESKTOPPET_PUBLIC_KEY`。CI 從目前儲存庫取得前兩者，從 Repository Variable 取得公開金鑰。

第一次發布先建立 ECDSA P-256 簽署金鑰，將公開點設定為 Repository Variable `DESKTOPPET_PUBLIC_KEY`、私鑰內容設定為 Secret `UPDATE_SIGNING_KEY`。**私鑰不得進入 Git。** 詳細指令、本地測試與後續版本步驟請見 [發布文件](docs/RELEASE.md)。

Tag `vX.Y.Z` 必須與 CMake 版本相同；CI 建置、測試、打包、驗證簽章，再建立正式 Release。沒有正確簽署設定時會失敗，不會發布未簽署更新。一般 Push／PR 只產生未簽署的人工下載測試包，客戶端不接受它作為更新。

更新視窗預設約 640×480 邏輯像素，支援調整大小與捲動；只顯示本版說明，下載／驗證／交接與安裝階段均有提示。舊版更新器失效請參考 [安全手動遷移](docs/MANUAL_MIGRATION.md)。主要及備援可信公開金鑰、離線輪替／撤銷請參考 [金鑰文件](docs/UPDATE_KEYS.md)；本次不生成正式金鑰或發布。完整驗證狀態見 [v1.1.1 測試紀錄](docs/V111_TESTING.md)。

## 已知限制與待驗證

- Windows Release、GUI、跨螢幕 DPI、睡眠喚醒、開機啟動、更新回復與實際資源使用，均需 Windows 實測。尚不宣稱達成 30 MB／1% CPU 目標。
- 全螢幕判斷使用前景視窗是否覆蓋所在螢幕；工作列與桌面排除，採一秒檢查。偽裝為全螢幕的無邊框視窗也會隱藏寵物。
- 更新僅支援本工具產生的未壓縮 STORE ZIP，以及五個明確允許的發布檔；這用來限制解壓風險。設定檔永不加入更新包。
- 安裝目錄不可寫、含 reparse point／符號連結，或檔案被安全軟體鎖定時，更新會拒絕或回復；不要求提升權限。
- 更新健康檢查為初始化完成事件加三秒存活檢查，無法判斷啟動後很久才發生的故障。金鑰輪替須沿既有授權鏈，舊單金鑰客戶端與失去全部可信金鑰的事故需要人工遷移。
- 突然斷電後以交易日誌回復，仍需磁碟及舊版備份完整；詳細中斷測試尚待實機驗證。
- 程式正常退出後不會在背景提醒；手動隱藏／關閉寵物視窗不等於退出。


安全、隱私、更新故障處理與疑似防毒誤判回報見 [SECURITY.md](SECURITY.md)。v1.5.0 沿用 v1.4.0 安全強化與正式金鑰；舊未受保護交易紀錄需要人工核對，不會自動信任備份。


v1.4.0 動畫重製：[16 種外觀預覽](docs/renewed-pets.png)／[動作預覽](docs/renewed-actions.png)。保留全部花色及海洋寵物；v1.5.0 另修正五貓哈欠／翻肚並擴充海洋動畫，正常素材保留；Windows 顯示與編譯結果依實際執行紀錄判定。

## Code signing policy

DesktopPet is applying for free code signing provided by [SignPath.io](https://signpath.io/), with certificates provided by the [SignPath Foundation](https://signpath.org/).

### Team roles

- Authors: [nyqvori1268](https://github.com/nyqvori1268)
- Reviewers: [nyqvori1268](https://github.com/nyqvori1268)
- Approvers: [nyqvori1268](https://github.com/nyqvori1268)

### Release security

- Official releases are built from the public source repository using GitHub Actions.
- Release builds must pass automated security and regression tests.
- Code signing requests will require manual approval.
- Automatic updates are verified using ECDSA P-256 signatures and SHA-256 hashes.

### Privacy policy

DesktopPet does not intentionally collect telemetry, analytics, or personal information. Application settings are stored locally.

The application connects to GitHub to check for and download software updates. GitHub's privacy policy applies to these network requests.

[GitHub Privacy Statement](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement)
