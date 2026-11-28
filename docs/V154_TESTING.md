# v1.5.4 名牌顯示及驗證

## 新基準

重新從 GitHub clone 並執行 `git fetch --prune origin`；HEAD、origin/main 與正式 v1.5.3 Tag 同為 `24feff1fe93798bb9d8f99c8d412c8ab79c1f532`。初始工作區乾淨，先讀 AGENTS.md／發布文件。此提交包含 v1.5.3 選單及名稱永久保存修正，並包含原人工修正；未使用舊工作區或舊增量包作為來源。

## 陰影：可確認與待確認

原碼檢查結果：Nameplate 的 WNDCLASS 初始 style=0，沒有啟用 CS_DROPSHADOW；自繪 DIB 先清成透明，只有半透明圓角、細邊線及字，不存在陰影／模糊／外部光暈繪圖。原 UITheme.sync(window) 卻對名牌套用一般視窗的沉浸式深色與 DWM 系統圓角偏好，而名牌本來就以 per-pixel alpha 自行畫圓角，未明確排除系統非用戶區裝飾。

因此已確認共用 DWM 裝飾套用於名牌的路徑，並排除程式自畫陰影及 CS_DROPSHADOW 是原始來源；但本環境沒有 Windows DWM，不能斷言使用者機器的大灰影一定由 DWM 產生，也不能宣稱陰影已實機消失。

本次僅對名牌：

- UITheme.sync(window,false) 只取色彩／字型主題，跳過一般視窗 DWM 裝飾；其他視窗呼叫保留 nativeFrame=true，行為不變。
- DwmSetWindowAttribute 的 DWMWA_NCRENDERING_POLICY 設為 DWMNCRP_DISABLED，系統圓角 preference 設為 DONOTROUND（值 1），停用該窗過場；保留由 GDI+ 畫出的圓角。
- 類別 style 明確為 0，若 CS_DROPSHADOW 被設上則只清除此名牌類別的旗標；建立、主題／DWM composition 變更與必要重建時重新套用，不新增輪詢。
- 無 Windows 全域 SPI 視覺效果寫入，沒有關閉全域 DWM，不改主程式／其他設定窗／GIF／OCP。沒有新增使用者開關。

API 依據：[DWM 非用戶區渲染政策](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmncrenderingpolicy)、[DWM 組合控制](https://learn.microsoft.com/en-us/windows/win32/dwm/composition-ovw)、[系統圓角政策](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners)。政策關閉不等於在此已量測 OS 合成結果。

## 字體與布局

字型家族維持原設定：繁中 Microsoft JhengHei UI、英文 Segoe UI、日文 Yu Gothic UI；Regular、GDI+ UnitPixel／AntiAliasGridFit 不變。

| 項目 | 原本 | 本次 |
|---|---|---|
| 100%／96 DPI 字體 | 12 px（9 pt） | 14 px（10.5 pt，+16.7%） |
| 最小邏輯字體 | 9 px | 10 px |
| 字體計算 | 先截斷 12×scale/100，再做 DPI 取整 | round(max(10,14×scale/100)×dpi/96)，只最後取整 |
| 左右內距 | max(6,8×scale/100) 邏輯 px | max(round(6×dpi/96),round(fontPx×0.65)) |
| 上下內距 | 舊固定／名義高度 | max(round(3×dpi/96),round(fontPx×0.30)) |

採使用者建議的 14 px，而非誤報為增加 20～25%。例：100%／144 DPI =21 px，100%／192 DPI =28 px，200%／96 DPI =28 px，300%／192 DPI =84 px；50%／96 DPI 受最小值限制為 10 px。

W=ceil(實測文字寬)+2×(padX+1)，H=max(fontPx,ceil(實測文字高))+2×(padY+1)。1 為實體邊界餘量。圓角直徑為 max(round(8×dpi/96),fontPx)，再限制在實際圓角框內。以同一 GDI+ Font／StringFormat 量測並繪字，PointF 原點補償實測 X／Y，將完整實測範圍水平及垂直置中；不使用省略號或固定寬度。

保留極窄工作區／舊長名稱的逐級縮字全文適配，縮字時同步重算內距，不浪費空間；一般可容納情形均使用上述最小字體。物理工作區極小而無法同時滿足可讀性與完整寬度，仍需實機檢視，不宣稱所有不可能幾何皆可滿足。位置沿用目前動畫可見聯集與約 5 px（按大小／DPI）間距，並避開寵物、氣泡與工作區；不依每幀輪廓上下跳動。

文字、DPI、寵物比例、語言、工作區寬度及主題未改時，prepare 直接返回，字體建立與量測不進入每幀路徑。透明度只重用 bitmap 變更合成 alpha，沒有新增 timer 或置頂操作。主視窗底部空間以保守的字體行高估計預留；實際名牌高度仍依真實量測。

## 測試

原 18 個 CTest 名稱保留，新增 v154_nameplate_layout 及 v154_native_nameplate，共 20 項（Python 項沿用原可選環境需求）。舊 v151 原生寬度驗證改量測新的實際字體與內距，沒有刪除斷言；v152 來源測試只同步程式版本為 1.5.4。

Linux GCC C++17，-O2 -Wall -Wextra -Werror -pedantic：原九組及新 v154_tests 共十組實際編譯執行通過。新名牌測試 56,260 checks，涵蓋 50～300% 每個百分比、多種 DPI、字體最小值／單次取整／單調性、實測框完整與置中內距、擬合重算、螢幕底部預留、合法最長繁中／英／日名稱、負座標定位、DWM scope／快取及原測試註冊。

v154_tests ASan＋UBSan 通過（detect_leaks=0，未宣稱洩漏驗證）。原名字 579 checks、選單 100,126 checks 及計時／安全／素材回歸通過；品牌／來源雜湊、70 GIF／840 播放位置、128 原陸生 GIF、安全掃描器／Actions SHA／provenance 閘門通過。各組最終數量見增量包 APPLY.md；逐像素與參數組合不等同獨立功能數。

## 新 Windows 原生測試（未執行）

- 實際 GDI+ 三語、最長合法中文／日文／12 個 W 與 Blue Dolphin，六種寵物比例 × 六種 DPI；比較實際 layered window 的寬高與完整量測＋內距，並確認不遮擋寵物。
- 反覆相同動畫幀的名牌位置／尺寸不變；font fitting 沿用實際工作區，無省略號。
- 查詢該 HWND 的 DWM 非用戶區渲染 disabled、系統圓角 DONOTROUND、類別 drop-shadow 關閉；主題及 composition 訊息後仍保持。
- 建立測試自己的明／暗不透明背景窗，不更改 Windows 主題或全域設定。擷取真實 compositor 顯示前／後影像，逐像素比較名牌視窗外 24 px 區域完全相同，同時確認名牌內確實顯示；可抓到 OS 陰影、光暈或外部殘影，而非只檢查程式旗標。
- 500 次相同幀及 30 次重建的 GDI 物件數不累積，保留 HTTRANSPARENT、透明／隱藏功能。

此項 CTest 設為 RUN_SERIAL，避免與其他原生視窗測試互相遮擋。需要可擷取畫面的 Windows 桌面及 DWM；原生測試背景窗只用於隔離測量，不是修改使用者桌布或全域視覺效果。

## 待驗證

本環境沒有 MSVC、Windows SDK、CMake、PowerShell、Wine 或 Windows 桌面；未編譯 Windows EXE、未執行 Windows 原生／DWM／多螢幕測試或 GitHub Actions，不能把 source／數學布局檢查當作「陰影確實消失」。

1. Windows Release＋完整 20 項 CTest／Test-PE，確認實際 ASLR／DEP／CFG／GS／DLL 防護及靜態 Runtime；SDK 小巨集衝突亦需實際編譯確認。
2. 在原出現灰影的 Windows 環境，實際確認開／關名牌、明暗桌面及主題、DWM composition／RDP 變化，外部陰影／光暈消失而半透明背景、圓角及文字正常。若仍有灰影，需依實際外圍像素與 DWM 狀態進一步定位，不能宣稱根因已完全實機證實。
3. 全 22 款及三組自訂，50～300%、各 DPI／多螢幕／負座標、最長名稱／Unicode／IME，檢查完整文字、間距、底部邊界與動畫穩定度。
4. v1.5.3 名稱持久化／舊設定遷移／更新恢復、主／子選單 Z-order、置頂／全螢幕避讓／穿透、大小／透明度、拖曳回彈、三模式與集中編輯器回歸。
5. CPU／RAM／GDI／USER handles、EXE／雙 ZIP 容量與視覺品質實測，未量測前不宣稱達標或無洩漏。

所有素材、core／設定來源及保存、menu_layer／main.cpp、更新／交易／金鑰／驗證／回滾／平台安全實作、build.yml 與 provenance／EXE SHA 輸出皆不變。UITheme 新參數預設保留一般視窗行為，只給 Nameplate 傳 false。未建立 Commit、Tag 或 Release。
