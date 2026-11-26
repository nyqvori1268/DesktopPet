# 建置、簽署與發布

v1.1.1 以上沿用既有正式金鑰，新增 [備援／輪替文件](UPDATE_KEYS.md) 與 [舊版安全手動遷移](MANUAL_MIGRATION.md)。下方「第一次發布」只適用尚無正式金鑰的新專案，升級時不要重新生成現有金鑰。

## 第一次發布

1. 將此目錄作為 GitHub 儲存庫根目錄，保留 `.github/workflows/build.yml`。
2. 安裝 VS 2022 C++、Windows SDK、CMake、PowerShell 7；金鑰腳本使用 PowerShell 7／.NET 的 P-256 與 P1363 簽章格式。
3. 在**儲存庫以外**產生私鑰：

```powershell
pwsh -File tools/New-SigningKey.ps1 -PrivateKeyPath "$env:USERPROFILE/DesktopPet.private.key"
```

輸出 `DESKTOPPET_PUBLIC_KEY=...` 是 128 位十六進位字元的 X||Y，不是 PEM。保護私鑰檔案 ACL，只允許發布者存取，另行離線備份。私鑰若外洩，既有客戶端需人工更換可信根，不能依賴已外洩的金鑰自行修復信任。

4. GitHub → Settings → Secrets and variables → Actions：

| 種類 | 名稱 | 值 |
|---|---|---|
| Variable | `DESKTOPPET_PUBLIC_KEY` | 上述 X||Y 公開點 |
| Secret | `UPDATE_SIGNING_KEY` | 私鑰檔中的 PKCS#8 Base64 單行內容 |

5. 本地正式設定與打包：

```powershell
$env:DESKTOPPET_PUBLIC_KEY = '填入你的128字元公開點'
$env:UPDATE_SIGNING_KEY = Get-Content "$env:USERPROFILE/DesktopPet.private.key" -Raw
cmake -S . -B build -A x64 -DDESKTOPPET_OWNER=你的帳號 -DDESKTOPPET_REPOSITORY=你的儲存庫 "-DDESKTOPPET_PUBLIC_KEY=$env:DESKTOPPET_PUBLIC_KEY"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
./tools/Package.ps1 -Sign -Tag v1.0.0
Remove-Item Env:UPDATE_SIGNING_KEY
```

簽署腳本會檢查 CMakeCache 內的公開金鑰相同，並使用原生測試程式驗證 .NET 產生的簽章。不可加入測試時略過簽章的參數。

6. 實際完成 `docs/TESTING.md` 的 Windows 驗收後，提交程式碼、推送 `v1.0.0`。Actions 必須全綠才視為發布成功：

```sh
git add .
git commit -m "Release DesktopPet 1.0.0"
git tag v1.0.0
git push origin HEAD
git push origin v1.0.0
```

Release 有四個 Assets：ZIP、SHA256SUMS.txt、update.json、update.sig。工作流程只有 release job 具有 `contents: write`；PR 不讀私鑰，標籤缺私鑰、公開金鑰或版本不符時安全失敗。

## 更新協定

`update.json` 為無 BOM 的 UTF-8 原始位元組，ECDSA P-256／SHA-256，`update.sig` 為 64-byte IEEE P1363 `r||s`。驗證原始 bytes，**不可重新排版再驗證**。

```json
{"schema":1,"version":"1.0.1","minVersion":"1.0.0","arch":"windows-x64","asset":"DesktopPet-windows-x64.zip","size":123456,"sha256":"64個小寫十六進位字元","notes":"更新內容"}
```

以上為格式說明，不是可用簽章或實際下載。`size` 上限 15 MiB，更新資訊 64 KiB，簽章必須恰好 64 bytes。僅接受嚴格三段正式版本，不接受先行版、build metadata 或降版。使用 HTTPS，僅允許 GitHub API／Release 及兩個 GitHub 資產主機；最多五次跳轉。逾時為解析／連線 4 秒、傳送／接收 5 秒，API 僅重試一次，資產失敗由使用者重試，取消可在下一次讀取或逾時後完成。

ZIP 僅接受五個允許路徑：`DesktopPet.exe`、`Updater.exe`、`data/dialogue.json`、`README.md`、`CHANGELOG.md`。拒絕額外檔案、目錄記錄、壓縮、重複名稱、加密、符號連結、資料描述器、ZIP64、路徑穿越或錯誤 CRC。全部 ZIP 的 SHA-256 受簽章保護。

更新流程：背景下載 → 驗證 → 啟動目前可信更新器的暫存副本 → 等候主程式結束 → 寫交易日誌、備份五個檔案 → 逐檔原子替換（含 Updater.exe）→ 新程式以保存的計時狀態啟動 → 就緒事件及三秒存活 → 刪日誌、清備份。失敗停止新程序、回復備份並重啟舊版。設定不在白名單，不會被替換。

`.desktoppet-transaction.json` 存在時，正常啟動會先使用暫存更新器執行回復。備份尚未完成的 backing 階段沒有修改程式檔，可安全清理；prepared／installed 階段回復備份。回復失敗會保留日誌及備份，請勿手動刪除。非交易期間清理超過七天的暫存下載與更新器副本。

## 本地安全更新測試

請在獨立資料夾以測試用公開點與正式相同的簽署規格測試，不把測試金鑰放入正式客戶端。v1.1.1 已改成父程序繼承控制代碼握手，舊版手動傳 PID／0 的命令已移除，不得拿它略過身分驗證。

Windows CTest 加入實際 handle 繼承、程序身分、準備就緒／取消及更新器提前結束的測試。完整更新仍需在公開測試庫或你已簽署的沙盒版本測試：正常退出／慢關閉、下載取消、逾時、程序提前退出、簽章／雜湊錯誤、回滾、計時恢復。舊版遷移使用 [Manual-Migrate.ps1](MANUAL_MIGRATION.md) 並保留所有簽章驗證。

## 後續版本

修改 CMake 版本、`src/pet.rc` 檔案與產品版本、CHANGELOG。使用同一組可信金鑰、完成建置與驗收，再推送新的 `vX.Y.Z` Tag。不得覆寫已發布版本或重用標籤。


## v1.2.0 的兩種 ZIP

`tools/Package.ps1` 從 CMake 讀取版號，自動產生：

- **手動下載：`DesktopPet_v{version}.zip`**。每個 entry 都在同名 `DesktopPet_v{version}/` 根資料夾，七檔為兩個 EXE、`data/dialogue.json`、README、CHANGELOG、LICENSE、THIRD_PARTY_NOTICES。
- **自動更新：`DesktopPet-windows-x64.zip`**。維持既有五個平坦路徑、不加入 LICENSE 路徑；README 附完整 MIT 與元件聲明。`update.json` 的 asset／size／sha256 僅對應此包，不指向手動包。

兩包均採明確檔案清單，不包含 config、自訂素材、私鑰或暫存資料。打包時驗證完整 entry 清單、根路徑及逐檔 SHA-256 與 staging 一致；自動包另由原本原生安全解析器驗證。`SHA256SUMS.txt` 含兩包；手動包沒有額外簽章協定，不可把它交給自動更新器。

Actions 的一般 Push／PR 產物含兩包與雜湊；正式 Tag 通過原有簽署檢查後，上傳兩包、雜湊、update.json／update.sig，使用自動產生的 RELEASE_NOTES.md 優先引導下載版本命名包。本次不建立 Tag 或 Release。

v1.1.1 → v1.2.0 仍下載同名五檔自動包，ECDSA P-256、原有可信金鑰、SHA-256、交易／回滾與設定位置不變。新欄位均可選，三組既有角色與計時資料保留。新版 Updater 接受舊八參數交接；新版配對元件可額外傳入介面語言，原有程序身分檢查不變。簽署更新、故障回滾及兩版端對端升級仍需 Windows 實測。


## v1.3.0 海洋素材的發布相容性

新增的 assets/ocean/*.ocp 由生成的 ocean.rc 僅內嵌 DesktopPet.exe。建置時需完整六檔；不把它們以額外 entry 加進 ZIP。兩種 ZIP 都已包含 DesktopPet.exe，因此仍沿用原路徑清單、update.json asset 名稱、SHA-256、ECDSA P-256、主要／備援金鑰與交易／回滾協定。v1.2.0 客戶端可使用相同五檔平坦包更新，免修改舊白名單。手動包自動命名為 DesktopPet_v1.3.0.zip 並含同名根資料夾。

CTest 新增 ocean_embedded：以 Windows 的資料資源載入方式（不執行 EXE）檢查發行主程式實際含六款正確素材。核心測試另外檢查所有影格、資源異常及七物種對話。不要將來源 GIF、自訂寵物、config 或開發工具增列至更新包；發布體積及 v1.2.0 → v1.3.0 簽署更新仍需 Windows 實測。

## v1.3.0 安全強化（不變更版本）

新增檢查：`security_tests`、`python tests/security_tool_tests.py`、`python tools/Check-Sensitive.py`、Windows `dll_search_policy`／原生交易中斷測試、`./tools/Test-PE.ps1`。CMake 與 Package.ps1 保持原有金鑰、版本來源與雙 ZIP；SECURITY.md 全文追加至 staging README，不新增更新包白名單路徑。

Actions 的官方 commit 來源見 [安全驗證紀錄](SECURITY_TESTING.md)。發行僅 Push 正式 Tag：同一建置的 immutable artifact ID → 分權限產生 GitHub artifact attestation → 發布 job 逐檔驗證儲存庫／workflow／Tag／source SHA／hosted runner → gh release --verify-tag。PR 沒有 OIDC、簽署 Secret 或發布權限。請在儲存庫設定維護分支／Tag 規則，定期審查固定 Actions 版本。

本地交易 v2 使用同使用者 DPAPI，簽署 Release 的 schema／ECDSA／ZIP 與舊客戶端保持不變。新信任檔仍為 schema 1，多出可選 receipt 欄位供提交一致性判斷，舊客戶端忽略該欄位。舊明文交易／遷移中斷的安全限制及人工處理方式見 [SECURITY.md](../SECURITY.md)。不要手改紀錄或刪除 update-trust.bin 來降版。

本次未改 1.3.0 版號、未產生 Windows EXE／Tag／Release。相同版號不會被既有防降版規則當成更新；請先本地重建／驗證，日後正式發布時再由維護者決定下一個版本，不能略過 SemVer 驗證發布同版覆蓋更新。


## v1.4.0 發布準備

以 main `4f8ddb205a7cf9008775143e582ac8f1dd4ba2e0` 為基準，將 `CMakeLists.txt` 的單一版本來源更新為 1.4.0。主程式、Updater、關於介面、Windows VERSIONINFO 及 manifest 均沿用既有版本範本自動產生，勿另行硬編碼。

本版納入貓／狗／鸚鵡／熊貓動畫重製，保留 16 款既有花色與海洋六款，合計 22 外觀；素材與安全／更新／provenance 程式碼未在此次版本準備中修改。CHANGELOG 的 1.4.0 區段供既有腳本擷取本版更新說明。

Windows 建置及測試後，既有 Package.ps1 會自動產生 `DesktopPet_v1.4.0.zip`（含同名根資料夾）與 `DesktopPet-windows-x64.zip`（原五檔自動更新包），SHA256SUMS.txt 涵蓋兩包；update.json 仍指向自動更新專用包，既有金鑰／簽章／來源證明及 build.yml 不變。

此階段只交付必要的版本／文件修改，未執行建置、測試、簽署打包，未建立 v1.4.0 Tag、提交 Commit 或發布 Release。正式發布前請依上述流程由維護者確認實際 Windows 建置、測試、素材顯示、更新／回滾與 provenance 結果，不能將過去驗證紀錄當成本次建置通過。


## v1.5.0 多語台詞及計時介面

開發基準：正式 v1.4.0 Tag 與當時最新 main 皆為 `d9c510071e9db3520654a0e9089d40dedb3fc0d0`。單一版本來源 CMake 更新為 1.5.0，RC、manifest、Updater 及關於版本沿用生成範本。

本次不建立 Tag 或 Release。保留完整最新 build.yml（含兩個 EXE 的 SHA-256 輸出）、完整 SHA 固定 Actions、provenance 產生與驗證、主要／備援正式金鑰及五檔 ZIP 白名單。兩種 ZIP 命名及結構、update.json、ECDSA、DPJ3／DPAPI、交易／回滾均不變，新增 UI、翻譯與動畫資源透過 EXE 更新，不加散裝資源。視覺功能僅增加 Windows 系統 DLL `dwmapi.dll`、`uxtheme.dll`，PE 檢查精確加入這兩項；ASLR／DEP／CFG／GS 與額外 Runtime 拒絕檢查均維持。

發布前依 [本版驗證紀錄](V150_TESTING.md) 完成 Windows Release／CTest、原生控制與 IME／DPI／深淺色實測、v1.4.0 設定遷移與簽署更新回滾、PE 防護及雙 ZIP 敏感資料檢查。此開發增量包不是自動更新包，不能直接掛到 update.json。

本版動畫追加：ocean.rc.in 使用六個 marine-*.ocp；renewed.rc.in 資源 ID／路徑不變，五貓檔含修正姿勢與新增影格。原始來源與 GIF 是開發／人工驗收檔，既有 install／Package.ps1 不將它們加進正式 ZIP，只內嵌主程式。兩種正式 ZIP 都帶同一個 DesktopPet.exe，因此不用修改五檔更新白名單。確認 `v150_animations`、`renewed_embedded`、`ocean_embedded` 及既有所有安全測試後，再依原流程簽署／驗證／打包；本次不執行發布。
