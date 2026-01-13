# Advanced C Final Project - Virtual File System

一個基於 C 語言實作的虛擬檔案系統（VFS），模擬 Unix-like 檔案系統的階層式目錄結構，支援加密磁碟映像持久化儲存與整合的 vi 風格文字編輯器。

## 特性

- **INode 架構**：基於 inode 的檔案管理系統
- **區塊分配**：使用 bitmap 進行區塊管理（1024 bytes/block）
- **加密儲存**：XOR 加密的磁碟映像持久化
- **內建編輯器**：vi 風格的文字編輯器
- **Unix-like 命令**：支援 ls、cd、mkdir、rm、cat 等常見指令

## 快速開始

### 編譯專案

```bash
make              # 編譯專案（產生 'command' 執行檔）
make clean        # 清除編譯產物
```

### 啟動系統

```bash
./command
```

啟動後會看到兩個選項：

**選項 1：載入現有的磁碟映像**
```
options:
1, loads from file
2. create new partition in memory
1

Input password for encryption:
[輸入密碼]
```

**選項 2：建立新分區**
```
options:
1, loads from file
2. create new partition in memory
2

Input size of a new partition (example 102400 2048000)
partition size = 2048000

Make new partition successful !
```

### 基本使用範例

```bash
/ $ mkdir projects              # 建立目錄
/ $ cd projects                 # 切換目錄
/projects/ $ put hello.c        # 從主機匯入檔案
/projects/ $ ls                 # 列出檔案
hello.c
/projects/ $ cat hello.c        # 顯示檔案內容
/projects/ $ vi newfile.txt     # 使用 vi 編輯器
/projects/ $ get hello.c        # 匯出檔案到主機
/projects/ $ status             # 查看系統狀態
/projects/ $ cd ..              # 返回上層目錄
/ $ exit                        # 儲存並離開（需輸入密碼）
```

## 系統架構

### 記憶體佈局

```
┌─────────────┐
│ SuperBlock  │ ← 分區元資料（大小、inode/block 統計、掛載時間）
├─────────────┤
│ INode Table │ ← 檔案/目錄元資料陣列
├─────────────┤
│Block Bitmap │ ← 區塊使用狀態追蹤
├─────────────┤
│ Data Block 0│ ← 1024 bytes
│ Data Block 1│
│     ...     │
│ Data Block N│
└─────────────┘
```

### 核心數據結構

**SuperBlock** (`space.h:9-21`)
- 分區大小、inode/block 總數與已用數量
- 區塊大小（固定 1024 bytes）
- 掛載時間戳

**INode** (`space.h:29-39`)
- 檔名（32 字元，儲存完整路徑，如 "/test/file.txt"）
- 10 個直接區塊指標 + 1 個間接區塊指標
- 檔案類型（0=一般檔案，1=目錄）
- 時間戳（建立/修改時間）
- 大小、權限、使用狀態

**記憶體分配比例**
- `INODE_RATIO = 16`：分區空間的 1/16 保留給 inode
- 最大約 221 個 inode（依分區大小而定）

### 模組架構

```
main.c          ← 程式進入點，處理啟動選項
├── space.c/h   ← 虛擬磁碟初始化、inode/block 分配管理、bitmap 操作
├── commands.c/h← 所有檔案系統命令實作、加密/解密、磁碟映像持久化
└── vi.c/h      ← 互動式行編輯器
```

**全域狀態**
- `virtualDisk`：整個虛擬磁碟的記憶體指標
- `sb`：SuperBlock 指標
- `currentPath`：當前工作目錄路徑字串

## 命令參考

### 檔案系統操作

| 命令 | 說明 | 範例 |
|------|------|------|
| `ls` | 列出目前目錄內容 | `ls` |
| `cd <dir>` | 切換目錄（支援 `..`） | `cd test` 或 `cd ..` |
| `mkdir <dir>` | 建立目錄 | `mkdir projects` |
| `rmdir <dir>` | 刪除空目錄 | `rmdir test` |
| `rm <file>` | 刪除檔案 | `rm hello.c` |
| `cat <file>` | 顯示檔案內容 | `cat hello.c` |
| `put <file>` | 從主機匯入檔案 | `put hello.c` |
| `get <file>` | 匯出檔案到主機（會重建目錄結構） | `get hello.c` |
| `status` | 顯示分區使用狀態 | `status` |
| `help` | 顯示命令列表 | `help` |
| `exit` | 儲存加密映像並離開 | `exit` |

### vi 編輯器命令

啟動編輯器：`vi <filename>`

| 命令 | 說明 |
|------|------|
| `:i <n>` | 在第 n 行後插入文字（輸入 `.` 結束） |
| `:d <n>` | 刪除第 n 行 |
| `:w` | 寫入變更到磁碟 |
| `:wq` | 寫入並離開 |
| `:q` | 離開（未儲存的變更會遺失） |
| `:h` 或 `:help` | 顯示幫助 |
| 純文字（無 `:` 前綴） | 附加到檔案末尾 |

## 技術細節

### 路徑表示方式

**關鍵設計**：所有檔案/目錄在 inode 的 `fileName` 欄位儲存**完整路徑**（例如 "/test/file.txt"）

這意味著：
- 沒有父目錄指標
- 目錄列表需掃描所有 inode 並過濾路徑前綴
- 路徑拼接操作至關重要（`commands.c:296-306`, `commands.c:406-418`）

### 加密機制

**XOR 加密**：
- `EncryptVirtualDisk()`：每個 byte 與 `password[i % passwordLength]` 進行 XOR
- `DecryptVirtualDisk()`：相同操作（XOR 對稱性）
- 應用於整個虛擬磁碟
- 密碼不儲存於任何地方（使用者必須記住）

實作位置：`commands.c:773-806`

### 區塊分配策略

**Bitmap-based 分配**（`space.c`）：
- Bitmap 位於 inode table 之後
- `allocateBlock()`：找到第一個空閒位元，標記為已使用
- `freeBlock()`：清除 bitmap 位元，更新計數器
- 線性搜尋（O(n)）- 無雜湊表或 B-tree 優化

### 檔案匯入/匯出

**put 命令** (`commands.c:288-379`)：
- 從主機檔案系統讀取檔案（`fopen()`）
- 在 VFS 建立 inode（儲存完整路徑）
- 分配區塊並複製資料（處理直接 + 間接區塊）

**get 命令** (`commands.c:381-495`)：
- 匯出到主機檔案系統
- **重建目錄結構**：若 VFS 路徑為 "/a/b/file.txt"，會在主機建立 "./a/b/"
- 使用 `mkdir()` 建立目錄（權限：`S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH`）

