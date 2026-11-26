# DesktopPet 安全與隱私

本次強化維持 v1.3.0；沒有變更正式金鑰、簽署協定、版本、Tag 或發布 Release。沒有混淆、掃描規避、關閉防毒或降低驗證。

## 網路、權限與本地資料

- 主程式僅為可選 GitHub 更新連線，使用 WinHTTP／HTTPS，啟動、每 24 小時或手動檢查公開 Releases；確認後才下載。不需使用者 Token，不上傳台詞、圖片、計時、名稱或個人資料。
- 更新關閉／未設可信公開金鑰時可完全離線使用。無帳號、廣告、遙測、音訊或背景常駐服務，無需管理員權限。
- config.json 優先存程式資料夾，無寫入權限時為 `%LOCALAPPDATA%\DesktopPet`；自訂圖片、台詞與信任檔均為本地資料。設定可能含使用者文字，回報時請先去識別化。
- 開機啟動預設關閉，只在使用者開啟後寫入 HKCU\Software\Microsoft\Windows\CurrentVersion\Run\DesktopPet；關閉會刪除該值，不使用服務或系統排程。
- CI 的來源證明另使用 GitHub OIDC／Sigstore，僅在建置／發布環境連線；不改變桌面程式網路行為，也不替換使用者的 ECDSA 金鑰。

## 更新驗證與復原

Release 維持 ECDSA P-256／SHA-256、既有主要與離線備援可信根、授權鏈／撤銷／防降版、正式 SemVer、架構與相容性檢查及固定五檔 ZIP 白名單。新金鑰仍須既有可信根授權，沒有略過旗標。程式及素材以原五檔包更新，config／自訂素材／私鑰不在白名單。

新本地交易紀錄以 Windows 同一使用者 DPAPI 保護，綁定目標路徑、簽署資訊、舊／新版本、五檔存在狀態與前後 SHA-256。拒絕缺欄位、非布林存在狀態、未知階段及異常檔案。讀取不跟隨 reparse point，拒絕硬連結，操作期間用目錄控制代碼阻止祖先被替換；只操作固定檔名，不依紀錄提供任意路徑。

所有備份完整驗證後才開始替換；主程式正常退出並通過原有 PID／建立時間／檔案身分握手後才動檔。備份持續保留到恢復或提交完成。新程式就緒並存活後，先持久化 committing 階段，再原子提交信任鏈／最高版本與 manifest 收據。

重啟時依收據判斷：尚未提交信任才可回滾；已提交且新檔完整時只完成清理，絕不還原舊版。紀錄最後才刪除，回滾與清理可重試；信任衝突、備份損壞、異常檔案或無法停止本更新器建立的子程序時停止並保留現場，不強殺無關程序。未知備份／暫存樹不遞迴刪除。

**相容性界線：** Releases／簽章／ZIP／交接參數與舊客戶端相容，既有設定與信任 schema 仍可讀。舊客戶端可正常安裝新版；但舊版明文交易紀錄沒有可驗證的備份雜湊與來源，不能當成刪檔／執行依據。遇到舊紀錄或 PowerShell 手動遷移中斷時，強化版會安全停止，保留紀錄與備份，需人工核對可信來源；不會自動信任或「轉換」未驗證備份。

若復原失敗：

1. 關閉程式，保留 `.desktoppet-transaction.json`、`.desktoppet-backup` 與 `%LOCALAPPDATA%\DesktopPet\update-trust.bin` 的副本。不要修改紀錄、刪除信任狀態或執行未知備份中的 EXE。
2. 回報目前／目標版本、錯誤階段及公開 Release URL。備份可能含舊程式；config／台詞／私鑰不應公開。DPAPI 紀錄綁定原 Windows 帳戶，換電腦不能直接解密，請保留原環境。
3. 由維護者以原可信根、簽署資訊與完整 Release 重新核對檔案／信任收據，再進行完整可信版本修復。信任已提交或狀態不明時，不自行降版或將舊 backup 覆蓋回去。缺少可信完整來源時保持停止，不以關閉驗證解決。

實體斷電、SSD／檔案系統失效、同一帳戶已遭惡意程式控制、管理員／核心層攻擊，不是 DPAPI 或目錄鎖能完全防禦的範圍。備份校驗失敗時安全停止，不宣稱一定可自動復原。

## DLL 與編譯產物

MSVC 明確使用 `/GS`、編譯及連結 `/guard:cf`、`/DYNAMICBASE`、`/HIGHENTROPYVA`、`/NXCOMPAT`、`/DEPENDENTLOADFLAG:0x800`。靜態相依 DLL 的搜尋限制於 System32；兩個 EXE 的進入點另設定 System32 的預設 DLL 搜尋並排除目前工作目錄。系統 COM／絕對路徑載入、已載入模組或已被控制的帳戶仍有不同邊界，不能只靠搜尋設定宣稱完全防護。

`tools/Test-PE.ps1` 直接解析兩個實際 PE，檢查 x64、ASLR／DEP／高熵／CFG 位元、relocation、GS cookie、CFG check／FID table、DependentLoadFlags 與系統 DLL 匯入清單；缺漏使打包／CI 失敗。GS cookie 存在不代表每個函式都有插入檢查，仍需同時檢閱編譯參數。此交付環境無 MSVC／Windows，**尚未確認實際 Windows 產物通過**。

## 發布來源與敏感資料

Actions 以官方儲存庫已核對的完整 40 位 Commit SHA 固定，checkout 不保留憑證；PR 不取得簽署私鑰／OIDC／發布權限。公開發布只由 Push Tag 進入，建置、來源證明、發布分開權限。以 immutable artifact ID 交接；發布前驗證每個產物的來源證明，綁定此儲存庫／工作流程／Tag／source SHA 並拒絕自行架設 runner。證明失敗則不發布。OIDC 與 attestations:write 只給來源證明 job，contents:write 只給發布 job。

建議維護者設定受保護分支與 Tag／Rulesets、必要審查及 Secrets 權限；這些 GitHub 帳戶設定不會由本增量自行修改。來源證明不是程式碼安全審查或防毒掃描的替代品。

`tools/Check-Sensitive.py` 檢查 tracked source（無 Git 時掃可存取來源）及 ZIP 內檔，拒絕常見私鑰、PKCS#8、GitHub Token、設定檔與敏感副檔名，只顯示路徑、不輸出值。發布仍採明確白名單，忽略檔另排除本地交易／信任／備份資料。模式掃描不是完全保證；本工作區無 Git 歷史，未掃遠端提交歷史或 GitHub Secrets。若發現曾提交真實私鑰，依 [金鑰事故文件](docs/UPDATE_KEYS.md) 撤銷／輪替並處理歷史，不能只刪目前檔案。

## 弱點與疑似防毒誤判回報

安全弱點優先使用儲存庫 Private vulnerability reporting（若維護者已啟用）：https://github.com/nyqvori1268/DesktopPet/security/advisories/new 。未啟用時，可先在 Issues 請求安全聯絡方式，避免公開可利用細節或敏感檔；目前未宣告私人聯絡信箱或回應 SLA。

疑似誤判請提供：版本、官方下載網址、ZIP／EXE SHA-256、防毒產品與引擎版本、偵測名稱、時間及經遮蔽個資的畫面。先核對官方 Release／雜湊／來源證明，保持防毒啟用與正常隔離；由作者向廠商送交樣本判定，例如 [Microsoft 安全樣本提交](https://www.microsoft.com/en-us/wdsi/filesubmission)。避免未經同意上傳含個資／私有素材的檔案至公開多引擎服務。不要求白名單、關閉防毒、規避掃描或混淆程式；沒有購買或宣稱 Authenticode 簽署憑證。
