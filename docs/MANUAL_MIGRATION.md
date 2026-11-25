# 舊版更新器失效時的安全手動遷移

v1.0.0／v1.1.0 使用「主程式立刻退出、更新器驗證後才按 PID 查路徑」的舊流程。程序可能已結束卻仍有控制代碼存在，路徑查詢失敗會被誤判成「程序不符」；PID 重用也沒有建立時間證明。v1.1.1 改用受限、明確列出的繼承控制代碼，驗證 PID、建立時間及實體檔案識別；驗證後回報就緒，主程式再存設定正常退出，更新器等程序 kernel handle 結束後才替換。

程式路徑不再使用有競態的延後 PID 重開與 `filesystem::equivalent` 查詢。取消、準備逾時 60 秒、等待正常結束逾時 90 秒均不授權覆寫；不強制殺掉父程序。舊版失效更新器無法靠包內新版 Updater 自行修好，因為包裡的新版尚未被安裝，必須安全手動遷移一次。

## 有簽署 Release 時

使用 PowerShell 7.2 以上，下載同一正式版本的三個檔案：`DesktopPet-windows-x64.zip`、`update.json`、`update.sig`。從你**先前已信任**的 Repository Variable／離線紀錄取得主要公開點；若已配置備援，也使用先前核對的備援公開點，不要信任下載包自行聲稱的新根。

```powershell
pwsh -File tools/Manual-Migrate.ps1 `
  -Target C:/Apps/DesktopPet `
  -Zip C:/Downloads/DesktopPet-windows-x64.zip `
  -Manifest C:/Downloads/update.json `
  -Signature C:/Downloads/update.sig `
  -PublicKey '既有主要公開點128hex' `
  -BackupPublicKey '既有備援公開點128hex' `
  -ConfigPath C:/Apps/DesktopPet/config.json
```

尚無備援時省略 `-BackupPublicKey`。設定在 LocalAppData 時把 `ConfigPath` 改為該實際位置，避免誤用另一份舊設定。腳本來自你檢閱的原始碼，不是從未驗證 ZIP 執行任意腳本。

腳本會：

1. 驗證原始 manifest 的 ECDSA P-256 簽章、既有授權的政策鏈、撤銷及版本下限，再以同一份 ZIP bytes 驗證 SHA-256。
2. 僅允許原本五個發布路徑，拒絕符號連結、路徑穿越、重複名稱、超限資料；確認兩個 EXE 產品版本與簽署版本相同。
3. 保存設定／計時快照，再請你從系統匣選「結束程式」。請保持設定不變；不會終止任何既有程序。
4. 確認此安裝位置的程序已關閉後，建立原格式交易日誌及備份，逐檔原子替換，再還原快照。
5. 以 `--updated` 重啟，等待初始化事件及三秒存活，提交已授權信任狀態，成功才清理日誌與備份；失敗則只停止它自己啟動的新程序並回復檔案。

對 v1.1.1 以上，腳本還會請**現有安裝的可信客戶端**驗證更新，以使用它的 DPAPI 撤銷／最高版本快取。舊客戶端若同時留下新版信任快取，腳本會拒絕推測其狀態，不能刪快取略過。

失敗回復後若主程式尚未執行，可執行原資料夾的 `DesktopPet.exe --updated` 恢復保存的計時。若中途斷電，保留 `.desktoppet-transaction.json` 及 `.desktoppet-backup`，正常啟動進入回復流程；不要先刪備份。磁碟故障或回復再失敗時先複製完整資料夾作保全，別重複覆蓋唯一備份。

## 沒有正式簽署包時

本次只交付程式修改，不建立 Tag／Release。若目前只有未簽署 CI 測試 ZIP，不能用它假裝已通過簽章更新。可自行從已檢閱原始碼編譯、關閉舊程式並保留設定／custom-pets 備份後人工部署測試；這是原始碼測試流程，不是略過自動更新驗證的正式發布方式。

不要只換 DesktopPet.exe 而留下舊 Updater.exe；兩者、資料與版本須一起一致部署。所有正式流程繼續使用既有簽署金鑰與五檔白名單，詳見 [金鑰備援](UPDATE_KEYS.md)。PowerShell 與 Windows 原生交接在本次 Linux 環境未執行，須先在測試資料夾與 Windows CI 驗證。
