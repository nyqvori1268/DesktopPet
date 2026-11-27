# 品牌 Icon

`pet.ico` 為使用者附件 DesktopPet-icon-assets.zip 中 DesktopPet.ico 的原始位元組，未重繪、縮放或改色。包含 16／20／24／32／40／48／64／128／256 px 九份 RGBA PNG；SHA-256：`9c988a3f90eeaf4a7aa7c9d0d9d18cd04216becaccce0f5ea011bfcf62627c10`。

兩個 EXE 透過既有資源 ID 1 內嵌，Windows／關於／系統匣依 DPI 選尺寸。不必安裝額外 PNG 或 Icon 檔；原始 1254／1024 PNG 與重複獨立尺寸 PNG 不加入發布包。export_assets 不再覆寫品牌 ICO。`tests/v152_asset_tests.py` 驗證完整雜湊、尺寸及 PNG CRC／透明度格式；Windows v152_native_ui 另比對兩個實際 EXE 內每個圖示影像與原 ICO。
