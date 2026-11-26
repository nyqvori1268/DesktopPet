# DesktopPet
原生 Windows 11 x64、C++17、Win32；介面及文件用繁體中文。無音訊、無遙測；僅更新可連網。不加入大型依賴，軟體及原創素材依使用者指定採 MIT；匯入素材權利不屬 MIT。
版本與開發者集中在 CMakeLists.txt，資源／manifest 由 .in 自動生成；更新 CHANGELOG。翻譯集中於 src/i18n.h，勿改動已儲存名稱／提醒原文。
建置：`cmake -S . -B build -A x64`，`cmake --build build --config Release`。
測試：`ctest --test-dir build -C Release --output-on-failure`。
Linux 核心測試：`g++ -std=c++17 -Wall -Wextra -pedantic -Isrc tests/core_tests.cpp -o core_tests && ./core_tests`。
發布前必讀 docs/RELEASE.md；更新不得略過簽章、雜湊、版本及封裝檢查。私鑰不可提交。
Windows GUI 與效能須實機測量，不將未執行驗收寫為通過。
