# _legacy Agent Notes

`_legacy/` 是舊架構程式碼與舊格式 level 資料，可作參考，但目前 executable 的 `files.cmake` 不使用這裡的 `src/`、`include/` 或 `levels/`。

- 不要把 `_legacy/` 當作 runtime source of truth。
- 若要復用舊邏輯，先對照目前 `src/`、`include/` 與 `Resources/data/` 的資料流。
- 除非任務明確要求整理舊版程式，通常不要修改這個目錄。
