# 建置、簽署與發布

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

## 本地安全更新測試（不需要公開私鑰）

- 產生獨立的本地測試金鑰；用它編譯 1.0.0 舊版與 1.0.1 新版。不要在正式客戶端放入測試私鑰。
- 複製程式目錄作為沙盒，在舊版啟動計時。新版提高 CMake 版本、同步 `src/pet.rc`，執行 `Package.ps1 -Sign -Tag v1.0.1`。
- 將新版 ZIP 改名 `package.zip`，與 `update.json`、`update.sig` 放進同一個 stage 目錄。先關閉沙盒舊程式，再從**另一個暫存目錄**複製並執行舊版 Updater.exe：

```powershell
# third argument 0 means no still-running parent to wait for (manual sandbox only).
& C:/temp/updater-test/Updater.exe C:/temp/update-stage C:/temp/DesktopPet-sandbox 0
```

這仍須完整簽章、雜湊及版本驗證；不是略過驗證模式。計時恢復情境需由主程式正常「下載並安裝」測試；可在你自己的公開 GitHub 測試庫放入測試金鑰簽署的正式 Release，沙盒客戶端必須編入該庫及測試公開金鑰。

- 篡改 ZIP、簽章及 manifest、舊版本／錯架構／minVersion 過高均應拒絕。
- 在沙盒停用新版就緒回報或製造新版啟動失敗，確認備份回復；替換階段結束 Updater 後再開主程式，確認中斷回復。
- 正式金鑰只用於正式 Release。沒有金鑰時可正常離線使用，但不能測試成功的自動更新。

## 後續版本

修改 CMake 版本、`src/pet.rc` 檔案與產品版本、CHANGELOG。使用同一組可信金鑰、完成建置與驗收，再推送新的 `vX.Y.Z` Tag。不得覆寫已發布版本或重用標籤。
