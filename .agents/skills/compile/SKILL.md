---
name: compile
description: These are the skills used when compiling this project.
allowed-tools: Bash(git *)
---

# Simutrans OTRP コンパイルスキル

## 環境

- **コンパイラ**: `/c/msys64/mingw64/bin/g++.exe` (GCC 13.2.0, MinGW-w64)
- **make**: `/c/msys64/usr/bin/make.exe`
- **ビルドディレクトリ**: `build/default/`

## 重要: stderr キャプチャの制限

**MinGW の g++ は Windows コンソール (CONOUT$) に直接書き込むため、Bash ツールの `2>&1` でエラーメッセージがキャプチャできない。**

- `g++ ... 2>&1` → 常に空出力（エラーがあっても見えない）
- `exit=$?` で終了コードは取れる（0=成功, 1=失敗）が、エラー内容は見えない
- PowerShell の `Process.StandardError` でもキャプチャ不可

## コンパイル確認の正しい方法

### 単一ファイルのビルド（エラーメッセージを見る）

```bash
/c/msys64/usr/bin/bash.exe -c "
  cd '/d/system/documents/GitHub/simutrans/simutrans' &&
  TEMP='C:/Users/aihara/AppData/Local/Temp' \
  TMP='C:/Users/aihara/AppData/Local/Temp' \
  PATH='/c/msys64/mingw64/bin:/c/msys64/usr/bin:\$PATH' \
  make build/default/gui/player_frame_t.o 2>&1
  echo exit=\$?
"
```

この方法なら **エラーメッセージがキャプチャされる**。

### フルビルド

```bash
/c/msys64/usr/bin/bash.exe -c "
  cd '/d/system/documents/GitHub/simutrans/simutrans' &&
  TEMP='C:/Users/aihara/AppData/Local/Temp' \
  TMP='C:/Users/aihara/AppData/Local/Temp' \
  PATH='/c/msys64/mingw64/bin:/c/msys64/usr/bin:\$PATH' \
  make -j4 2>&1 | grep -E 'error:|Error'
  echo exit=\$?
"
```

### エラーチェックのみ（ビルドなし）

```bash
/c/msys64/usr/bin/make.exe -n build/default/some_file.o 2>&1
```

dry-run でビルドコマンドを確認できる（ファイルが up-to-date の場合は `touch` で強制）。

## 実際のコンパイルフラグ（参考）

make が使う実際のフラグ：

```
g++ -DPNG_STATIC -DZLIB_STATIC -static -Wno-deprecated-copy
    -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DWINVER=0x0501 -D_WIN32_IE=0x0500
    -O1 -DNDEBUG -DMULTI_THREAD
    -DREVISION=211212 -DUNOFFICIAL_REVISION=67 "-DKUTA_REVISION=2021/12/12"
    -Wall -Wextra -Wcast-qual -Wpointer-arith -Wcast-align
    -DREVISION="834195" -Wconversion -DCOLOUR_DEPTH=16
    -c -MMD -o build/default/<path>.o <source>.cc
```

## TEMP エラーへの対処

make 経由で g++ を呼ぶと `Cannot create temporary file in C:\WINDOWS\: Permission denied` が出ることがある。

**解決策**: `TEMP` と `TMP` を明示的に渡す（上記コマンドに含めてある）。

## ビルド対象のパスの指定

ソースファイルのパスからビルドターゲットを導出：

| ソース | ターゲット |
|--------|-----------|
| `gui/player_frame_t.cc` | `build/default/gui/player_frame_t.o` |
| `simline.cc` | `build/default/simline.o` |
| `display/simgraph16.cc` | `build/default/display/simgraph16.o` |
| `player/simplay.cc` | `build/default/player/simplay.o` |

## 依存関係の確認

各 `.o` ファイルと同名の `.d` ファイルに依存するヘッダ一覧がある：

```bash
cat build/default/gui/player_frame_t.d | head -5
```
