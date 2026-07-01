---
name: build
description: These are the skills used when building this project.
allowed-tools: Bash(git *)
---

# Simutrans OTRP ビルドスキル

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
  make -j32 2>&1 | grep -E 'error:|Error'
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

## git checkout 直後の全ファイル再ビルドと windres エラー

`git checkout`/ブランチ切り替えは全ファイルの mtime を更新するため、その直後に `make` を実行すると（実際のソース変更が無くても）全オブジェクトが再ビルド対象になる。この際、`simres.rc`（Windows リソース）のビルドが以下のエラーで失敗することがある：

```
windres.exe: can't popen `"g++ -E -xc -DRC_INVOKED -MMD -MT build/default/simres.o" -DREVISION=... simres.rc': No error
```

これは `simhalt.cc` などの実際のソース変更とは無関係な環境要因（windres が内部で g++ をプリプロセッサとして呼び出す際に失敗する）。原因は未特定。`build/default/simres.o` が既に存在し `simres.rc` 自体は変更されていない場合、`touch build/default/simres.o build/default/simres.d` で該当ファイルの再ビルドをスキップできる可能性があるが、**これはビルド成果物への直接操作であり、実行前に必ずユーザーに確認すること**。

## ビルド後、リンクまで走ったか確認する

`make build/default/sim.exe` のようにリンク成果物のパスを直接指定すると、依存関係の再評価が行われず `Nothing to be done for 'build/default/sim.exe'` と表示されてリンクがスキップされることがある（`.o` が実際には新しくても）。**必ずデフォルトターゲット（`make -j32` のみ、ターゲット名なし）でビルドし**、`build/default/sim.exe` が変更した `.o` より新しいことを確認してからテストに進むこと。

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
