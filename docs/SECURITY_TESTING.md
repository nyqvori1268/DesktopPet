# v1.3.0 安全強化驗證（2026-11-26）

## 已實際執行

- Linux／GCC C++17 Release：`-O2 -DNDEBUG -Wall -Wextra -Werror -pedantic`，既有核心 **5,157,760** 項檢查通過（含所有 22 外觀／7 物種與三語完整性）。
- 新增 `security_tests`：**4,141** 項檢查通過；相同測試在 AddressSanitizer／UBSan 下通過。LeakSanitizer 因本環境限制停用，不宣稱漏洩檢查通過。
- 中斷測試直接使用產品的 `recoverTransaction` 與 `classifyTrustCommit`，以記憶體後端模擬持久化檔案／雜湊／收據；覆蓋每個替換前綴、各復原 I/O 完成後斷電／程序消失、清理中斷後重試、回滾冪等、信任提交前後分流、錯誤收據、損壞備份及外來檔案拒絕。這是故障注入模型，**不是實體斷電實測**。
- `python tests/security_tool_tests.py` 通過：私鑰／Token／副檔名正負案例、ZIP 設定檔拒絕、錯誤僅含路徑、所有 Actions 完整 SHA、來源證明與 Tag gate。
- `tools/Check-Sensitive.py` 掃描可存取專案來源，未檢出指定私鑰／Token／設定檔模式；workflow YAML 與 job 權限分離檢查通過。工作區無 .git 歷史，未掃遠端歷史或 Secrets；模式掃描不是沒有秘密的完整證明。增量 ZIP 另於交付時掃描與驗證 CRC。
- 比對確認 src/sprite.h、六款海洋資源／來源記錄、dialogue.json、信任政策授權邏輯、ECDSA／ZIP 協定、既有父程序握手及正式金鑰建置欄位未變。版本仍為 1.3.0，沒有產生金鑰、Tag 或 Release。
- binary Patch 在基準副本先 `git apply --check`、實際套用並逐檔位元比對；沒有新增寵物素材或執行檔。

## 已新增、尚未執行的 Windows 驗證

目前沒有 MSVC、Windows SDK、CMake、PowerShell、Windows 桌面或實際 DesktopPet.exe／Updater.exe，因此尚未確認產物防護或 Windows 編譯通過。

- CTest `windows_native`：真實子程序在 sealed prepared 交易後 `ExitProcess(23)`，確認 DPAPI 紀錄及五檔備份可讀；篡改 ciphertext 拒絕、複製至其他目標拒絕、目錄鎖阻止 rename、NTFS 硬連結拒絕。測試用 manifest／全零假簽章只用於本地紀錄格式，沒有略過產品的 manifest／簽章驗證。
- CTest `dll_search_policy`：僅建置測試用 benign DLL，確認絕對路徑載入成功，再確認工作目錄同一 DLL 不能由相對名稱載入；該 DLL 不安裝或打包。
- `tools/Test-PE.ps1` 直接檢查兩個 MSVC Release PE 的 x64、ASLR／DEP／高熵／CFG、relocation、GS cookie、CFG check／FID、System32 DependentLoadFlags 與 DLL 清單；腳本在 CI 及 Package.ps1 執行，缺漏失敗。尚未在此環境跑過，不能宣稱實際 EXE 已有這些防護。
- PowerShell 手動遷移：commit 嘗試後的例外保留新檔／紀錄／備份，不冒險還原已撤銷版本；使用原驗證與金鑰。中斷於舊明文紀錄者需人工核對。
- Actions 的 OIDC／Sigstore 來源證明、artifact ID 交接、來源 SHA／Tag／workflow 驗證、Secret 缺漏及故意篡改產物拒絕；尚未呼叫正式 Release 或實際跑 workflow。
- 完整簽署更新／健康檢查／取消／Windows 檔案占用／信任提交前後 fault injection／回滾、真實 OS 重啟與實體斷電、GUI／DPI／計時／無聲、性能皆待實機。

## Actions 固定來源核對

完整 SHA 從官方 GitHub 儲存庫的 commit／release 頁核對；不是本工作區自行生成的值，也沒有聲稱離線驗過 GitHub GPG 簽章。直接 GitHub API 讀取受環境網路限制，改用官方頁面來源。

| Action | 固定版本 | 完整 SHA／官方來源 |
| --- | --- | --- |
| checkout | v4.4.0 | [11d5960a326750d5838078e36cf38b85af677262](https://github.com/actions/checkout/commit/11d5960a326750d5838078e36cf38b85af677262) |
| upload-artifact | v4.6.2 | [ea165f8d65b6e75b540449e92b4886f43607fa02](https://github.com/actions/upload-artifact/commit/ea165f8d65b6e75b540449e92b4886f43607fa02) |
| download-artifact | v4.3.0 | [d3f86a106a0bac45b974a628896c90dbdf5c8093](https://github.com/actions/download-artifact/commit/d3f86a106a0bac45b974a628896c90dbdf5c8093) |
| attest-build-provenance | v2.4.0 | [e8998f949152b193b063cb0ec769d69d929409be](https://github.com/actions/attest-build-provenance/commit/e8998f949152b193b063cb0ec769d69d929409be) |

固定版本不等於永遠無漏洞；日後更新需重新檢查來源與完整 SHA。來源證明需支援該功能的 GitHub 儲存庫／方案，不支援時安全失敗，不以關閉驗證發布。

參考官方規格：[DLL 搜尋政策](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories)、[靜態相依 DLL 搜尋](https://learn.microsoft.com/en-us/cpp/build/reference/dependentloadflag)、[CFG](https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard)、[gh attestation verify](https://cli.github.com/manual/gh_attestation_verify)。
