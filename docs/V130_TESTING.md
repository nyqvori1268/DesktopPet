# v1.3.0 驗證紀錄（2026-11-26）

## 已實際執行

環境為 Linux x64／GCC，沒有 MSVC、Windows SDK、CMake、PowerShell 或 Windows 桌面。本次不建立／發布 Tag、Release 或正式金鑰。

- C++17 核心以 `-O2 -DNDEBUG -Wall -Wextra -Werror -pedantic` 編譯成功，`PASS: 5157716 checks`。
- 同一組測試以 AddressSanitizer＋UndefinedBehaviorSanitizer 執行通過；已知此環境不支援 LeakSanitizer，使用 `ASAN_OPTIONS=detect_leaks=0`，不宣稱漏洩檢查通過。
- 全部六款指定 GIF、36 影格轉換後的 RGBA SHA-256 與清理後原圖相同，保留原解析度／配色／透明度；只裁切共同留白，實際移除孤立像素數皆為零。原生解碼結果另以逐影格 ARGB CRC32 比對。
- 開發轉換器重跑，六款資源及 provenance 可逐位元重現。內嵌資料總計 1,373,438 bytes；不等於實際 Windows Release 包容量。
- 六款素材維度／時間／透明像素／各款閉眼影格、鯨鯊慢節奏、最近鄰取樣、未使用快取釋放；拒絕截斷、錯誤尺寸／影格數／調色盤／索引／alpha／多餘資料。
- 二十二外觀實際點陣資料均不同；七物種與三語八狀態路由，海洋台詞不借用其他物種，缺漏只用當地語言通用文字。
- 新舊角色 ID 與設定往返、舊三組自訂台詞、語言切換、計時／延後與桌面設定在角色切換後保留；既有計時、JSON、ZIP、信任／撤銷政策等核心回歸通過。
- 原 src/sprite.h、所有舊物種台詞逐位元／資料比對保留；原更新白名單、下載／簽章／程序交接／交易／回滾／雙 ZIP 腳本未變。版號由同一 CMake 來源 1.3.0 產生，manifest 替換版號後 XML 可解析。
- 交付時驗證 binary Patch 的 git apply --check、實際套用及所有修改檔逐位元一致性；整合 ZIP CRC 檢查。

總檢查數含大量逐像素與截斷資料斷言，不代表五百多萬個獨立使用情境。

## 已加入但尚未在 Windows 執行

CTest 的 `ocean_embedded` 以資料方式載入實際 DesktopPet.exe，確認資源 201..206 存在、可原生解碼且與六個來源 .ocp 的 SHA-256 相同。既有 Windows 測試保留。此測試不將 EXE 當程式執行。

## Windows／Actions 待驗證

- MSVC x64 Release／RC 編譯、靜態 Runtime、ocean_embedded 及全部原生 CTest。
- 七種物種／二十二外觀的選單名稱、三語、原始比例、透明邊緣、眨眼／睡眠／睡醒、點擊／撫摸／生氣、拖曳回彈及提醒跳躍；確認無貓狗肢體、無自主移動、選單不被氣泡遮擋。
- 多 DPI／多螢幕、縮放、透明度、置頂／穿透、隱藏時切換與恢復、睡眠喚醒及無聲。
- Package.ps1 產出的兩種 ZIP、版本命名根目錄、SHA256SUMS、內嵌素材的實際可執行檔載入；自動包仍為原五檔清單。
- 使用既有金鑰的 v1.2.0 → v1.3.0 簽署更新／失敗回滾／設定與計時恢復；未在本環境執行正式端對端更新或 Actions。
- 完整 Release 的 15 MB 目標、CPU、程序記憶體與動作流暢度須實機量測，尚未宣稱達標。
