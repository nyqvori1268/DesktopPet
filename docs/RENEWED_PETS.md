# v1.4.0 貓、狗、鸚鵡、熊貓動畫重製

原素材重製以 v1.3.0 後的 main `c758eeba3f1eaddf80ec64fdab3cce193cedfc37` 為基準；本次從最新 main `4f8ddb205a7cf9008775143e582ac8f1dd4ba2e0` 準備整合為 v1.4.0。僅更新版本與發布文件，來源圖集、GIF、內嵌資源、花色索引及程式邏輯不變；未提交 Commit／Tag 或發布 Release。

## 外觀與花色相容

| pet 設定 ID | coat 索引（保持原順序） | 新圖集／資源 |
| --- | --- | --- |
| 0 貓 | 0 橘貓、1 三花、2 白貓、3 黑貓、4 虎斑 | source/cats.png → pet-0-0..4.ocp |
| 1 狗 | 0 赤柴、1 黑柴、2 白柴、3 柯基、4 米色垂耳犬 | source/dogs.png → pet-1-0..4.ocp |
| 3 鸚鵡 | 0 綠虎皮、1 藍虎皮、2 黃虎皮、3 灰玄鳳、4 奶黃玄鳳 | source/birds.png → pet-3-0..4.ocp |
| 4 熊貓 | 0 黑白熊貓；原本無額外毛色 | source/panda.png → pet-4-0.ocp |

原本是 `src/sprite.h` 依 `pet/coat` 索引選擇配色及形狀，並非載入既有 GIF。重製後仍以同一索引選擇完整外觀，**不以單一造型假裝支援全部花色**；花色、斑紋及柯基／垂耳犬、虎皮／玄鳳輪廓均有獨立素材。未改 Config、schema、選單命令／名稱、語言及台詞路由。既有設定無需遷移。

pet=2 的三組自訂素材及台詞、pet=5／6 海豚／鯨鯊，以及所有海洋來源檔保持不變。DPJ3、目錄鎖、竄改／備份保留測試、簽署／回滾與修正後 provenance workflow 均以最新 main 原樣保留。

## 動畫與檔案

- `assets/renewed/source/`：四張原創生成 PNG 與生成規格；並非網路圖片或第三方角色。美術由內建圖片生成工具製作，沒有 API CLI 或新增執行時服務。圖集每款含待機、眨眼、睡眠、哈欠、伸展、翻肚六個姿勢；鳥第六姿勢為拍翅／歪頭，不會翻肚或長出哺乳類四肢。
- `assets/renewed/gifs/pet-{pet}-{coat}/`：每款八段透明循環 GIF，`idle`、`blink`、`sleep`、`yawn`、`stretch`、`belly`、`happy`、`pet`，共 128 檔。鳥的 belly 檔沿用 Action::Belly 識別但內容為拍翅。每段以十二個呼吸／擺動樣本輸出；GIF 合併相同樣本後的實際 frame 數可能少於十二，總時長仍為 1.2 秒（睡眠 2.4 秒）。
- `assets/renewed/pet-*.ocp`：同一組姿勢轉為既有、未修改的 OCP1 調色盤／PackBits 格式，復用已存在的有界原生解碼器。十六檔共 541,344 bytes，約 0.52 MiB。
- `manifest.json`：保留索引、資源 ID 301..316、來源 SHA-256、逐姿勢原生 ARGB CRC32。海洋資源 201..206 沒有改動或衝突。
- [96 邏輯像素外觀預覽](renewed-pets.png)、[各物種動作預覽](renewed-actions.png)；預覽背景僅供閱讀，GIF／資源背景透明。

每個姿勢固定 128×128 畫布、共同縮放比與腳底基線，以完整連通輪廓擷取跨格耳朵／尾巴／頭冠，清除低 alpha 邊緣及孤立碎片，128 色量化／最近鄰縮放保留像素邊緣。姿勢不逐格自動放大，避免呼吸／眨眼時大小跳動。

執行時使用同一組六姿勢，依既有 Action 播放，沿用 8–12 FPS／拖曳／睡眠排程，以輕微呼吸與擺動、眨眼、睡醒順序及原生共用愛心／生氣／提醒跳躍補足互動。熊貓既有竹子效果保留。GIF 是可直接使用／預覽的動畫輸出；桌面程式不是逐檔播放 GIF，而是以原生共用時序繪製同一姿勢，沒有新增 GIF 解碼依賴。拖曳、彈簧回彈、氣泡及計時機制不變。

主程式只快取目前一款（六姿勢約 384 KiB）；切換（包含隱藏期間切換）時清除未使用的舊角色快取。資源只內嵌 DesktopPet.exe，兩種既有發布 ZIP 隨 EXE 攜帶素材，**不新增更新白名單路徑或改簽章協定**。來源 PNG／GIF 不另外塞入可攜式 Release。

## 重建與驗證

一般 CMake／MSVC 建置直接使用已轉換的 `.ocp`，不需 Python Runtime、Pillow、來源重新生成或網路。選擇性開發工具：

```sh
python tools/Build-Renewed-Pets.py
python tests/renewed_gif_tests.py
g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Werror -pedantic -Isrc tests/renewed_tests.cpp -o renewed_tests
./renewed_tests
```

轉換／GIF 檢查需要開發環境 Pillow；不增加桌面程式相依。CMake 增加 `renewed_art`，Windows CTest 增加 `renewed_embedded` 驗證實際 EXE 的全部十六資源及來源雜湊，原有測試保留。

素材重製階段既有的實際 Linux 結果（本次版本準備未重跑）：新增素材核心 **1,598,110** 項檢查通過，ASan／UBSan 同數通過（此環境不支援 LeakSanitizer，未宣稱漏洩檢查通過）；既有核心 **5,157,782** 項與安全 **4,141** 項通過；128 段 GIF 的透明、循環、時長、邊界及來源雜湊檢查通過。總數包含大量逐像素斷言，不等於獨立情境數。最新安全檔、海洋素材與設定／花色核心逐位元比對保留。

本環境無 CMake、MSVC、Windows SDK、PowerShell 或 Windows 桌面，尚未編譯 Windows EXE／RC 或執行 Windows GUI。實際選單切色、各 DPI 小尺寸、眨眼／睡眠轉換、拖曳／穿透、素材載入／內嵌資源 CTest、更新／回滾及效能仍待 Windows／Actions 驗證；沒有宣稱達到 CPU、記憶體或 15 MB 完整發布目標。


## v1.5.0 貓咪局部修正

舊原始 cats.png 的哈欠姿勢已重複畫出兩隻落地前腳，翻肚則只有三個清楚肉球；不是 Win32 合成殘影。新 source/cat-corrections.png 只提供五花色×兩姿勢，正常原圖集不覆寫。各 cat OCP 的第 0／1／2／4 姿勢像素與 v1.4.0 完全一致，第 3／5 姿勢替換為四肢合理版本；後接兩段 12 幀哈欠／翻肚動畫（總 30 幀）。其餘 11 款陸生 OCP、118 段正常 GIF 與全部舊來源雜湊未變。詳細動畫及驗證見 [V150_ANIMATIONS.md](V150_ANIMATIONS.md)。

重新建置本次修正只需 `python tools/Build-V150-Animations.py`，不會寫入狗、鳥、熊貓或正常貓咪 GIF。若維護者刻意重新跑舊完整素材生成器，須接著執行本工具套用修正，再跑素材回歸，不能發布重新帶回六腳的舊姿勢。
