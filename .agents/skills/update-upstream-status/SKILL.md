---
name: update-upstream-status
description: Update .agents/docs/upstream-merge-status.md (and its archive .agents/docs/upstream-merge-archive.md) - the documents tracking which OTRP-KUTAv6 (upstream) changes have been cherry-picked into develop-kasumi. Use when new upstream commits have landed, when something was merged into develop-kasumi, or when the merge status needs re-verification.
argument-hint: "[--scan-upstream | --recheck-status | --all] (default: --all)"
allowed-tools: Bash(git *), Read, Edit, Write, Grep, Glob
---

# 本家取り込み状況ドキュメントの更新

管理対象ファイル:

- `.agents/docs/upstream-merge-status.md` … 本体 (一覧表 + 未取り込み/取り込み不要 項目の詳細)
- `.agents/docs/upstream-merge-archive.md` … アーカイブ (`取り込み済み` 項目の詳細)

このドキュメントは、本家ブランチ `OTRP-KUTAv6` の変更のうち何を `develop-kasumi`
に取り込んだか/取り込んでいないかを、**意味ベースでグループ化した項目単位**で管理する。
まず対象ファイルを読み、冒頭の「追跡範囲」と「編集ルール」に従うこと。

**アーカイブ運用**: 状態が `取り込み済み` になった項目は、一覧表の行だけを本体に残し、
「詳細」セクションを `upstream-merge-archive.md` へ移設する。
既存項目を調べるときは本体とアーカイブの両方を対象にすること
(ある項目の詳細は必ずどちらか一方にだけ存在する)。

## モード

引数なし、または `--all` で両方を実行する。

- `--scan-upstream` … 本家の新規コミットを取り込んで項目を追加/追記する (下記 A)
- `--recheck-status` … 既存項目の `状態` を実際のコードと照合して更新する (下記 B)

---

## A. 本家の新規コミットを反映する

### A-1. 差分の洗い出し

```bash
git fetch origin OTRP-KUTAv6      # 必要に応じて upstream も
```

ドキュメントの「追跡範囲」表から `head` のハッシュを読み取り、そこからの新規コミットを列挙する:

```bash
git log --oneline <HEAD_IN_DOC>..origin/OTRP-KUTAv6 | cat
git log --format='%n===== %H%n%s%n%b%n----- files:' --stat <HEAD_IN_DOC>..origin/OTRP-KUTAv6 | cat
```

新規コミットが無ければ「更新なし」と報告して終了する。

### A-2. 各コミットの内容を確認する

コミットメッセージだけで判断しない。必ず実際の diff を読むこと:

```bash
git show <sha> -- <主要なファイル> | cat
```

特に注目する点:

- `dataobj/settings.{h,cc}` / `*::rdwr()` の変更 → **セーブ形式への影響**。
  `file->get_OTRP_version() >= NN` のゲートがあれば `NN` を控え、MISC-01 の依存関係リストに追記する
- `dataobj/schedule_entry.h` のフラグ追加 → スケジュール入出力 (develop-kasumi 独自機能) への影響
- `documentation/ja.OTRP.tab` の変更 → MISC-02 へ

### A-3. 分類とグループ化

編集ルール 1・4・5 に従う。判定順:

1. **既存項目の続きか?**
   同じ機能に対する後続バグ修正・機能拡張であれば、新規項目を作らず
   既存項目の「上流コミット」に `- \`<短縮sha>\` <件名> — 後続修正` の形式で追記し、
   必要なら「主な変更箇所」「詳細」も更新する。
   対象がアーカイブ済み (`取り込み済み`) の項目だった場合は、
   詳細セクションをアーカイブから本体へ戻したうえで追記し、状態を `一部取り込み` にする。
   判断材料: PR タイトル、変更ファイルの重なり、コミットメッセージ本文に含まれる
   元 PR のコミット列 (本家は squash merge でブランチ全体の履歴が本文に残る)。
2. **v58_3 より後に追加された機能に対するバグ修正か?**
   → その機能の項目 (`FEAT-nn`) に統合する。独立した `FIX-nn` にはしない。
3. **v58_3 時点から存在するコードのバグ修正か?** → 新規 `FIX-nn`
4. **新機能・機能拡張か?** → 新規 `FEAT-nn`
5. **バージョン番号更新のみ** → MISC-01 に追記
6. **翻訳ファイルのみ** → MISC-02 に追記
7. **上記いずれでもない** (CI 設定、リファクタ等) → 新規 `MISC-nn`

新規項目のIDは **既存の最大連番+1**。欠番があっても再利用しない。

### A-4. 記入

一覧表と詳細セクションの**両方**に追加する。詳細セクションの項目テンプレート:

```markdown
### FEAT-nn: <一言で表した機能名>

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: <追加/修正された機能の概要を一言で>
- **上流コミット**:
  - `<短縮sha>` <コミット件名>
- **主な変更箇所**:
  - `path/to/file.cc` — <その箇所で何を変えたか>
- **詳細**: <仕組み・計算式・条件など>
- **取り込み時の注意**: <セーブ形式への影響、develop-kasumi 独自改造との干渉など。無ければ —>
- **develop-kasumi 側コミット**: —
- **備考**: —
```

新規項目の `状態` は原則 `未取り込み`。A-1 の時点で develop-kasumi に既に入っている
ことが B の手順で確認できた場合のみ `取り込み済み` にする。

### A-5. 追跡範囲の更新

「追跡範囲」表の `追跡済み最新コミット (head)`、`対象コミット数`、`最終更新` を更新する。
`head` にはフルハッシュと、括弧内にコミット件名またはバージョンを書く。

---

## B. 状態の再確認

### B-1. cherry-pick 済みコミットの検出

本家は squash merge のため、cherry-pick してもハッシュは一致しない。件名で突き合わせる:

```bash
# develop-kasumi 側で、本家由来と思われるコミットを列挙
git log --oneline $(git merge-base develop-kasumi OTRP-KUTAv6)..develop-kasumi | cat
```

各項目の「上流コミット」の件名が develop-kasumi 側の履歴に (ほぼ) 同じ件名で
現れていれば取り込み済みの候補。

### B-2. コードでの裏取り (必須)

件名一致だけでは不十分 (手作業で移植された場合、件名が異なることがある)。
各項目の「主な変更箇所」から**その項目に固有の識別子**を1つ選び、実際に存在するか確認する:

```bash
git grep -n "<識別子>" develop-kasumi -- <該当ファイル>
```

識別子の例: 新規メンバ変数名、新規メソッド名、新しい enum 値、新設の設定キー名。
(例: FEAT-09 なら `highlighted_route_tiles`、FEAT-10 なら `bt_memo_filter`、
FEAT-07 なら `tile_length`)

### B-3. 状態の更新

- コードが存在し、対応するコミットが特定できた
  → `状態` を `取り込み済み` にし、`develop-kasumi 側コミット` に短縮sha を記入。
  そのうえで**詳細セクションを `upstream-merge-archive.md` へ移設する** (下記 B-4)
- 項目内の一部の上流コミットのみ取り込まれている
  → `一部取り込み` にし、`備考` に取り込み済み/未取り込みの内訳を書く
- コードが無い → `未取り込み` のまま
- ユーザーが「取り込まない」と判断した → `取り込み不要` + `備考` に理由

**一覧表と詳細セクションの両方を必ず同期させること。**

矛盾 (ドキュメントは `取り込み済み` だがコードが無い、等) を見つけた場合は、
勝手に書き換えず `備考` に `**要確認**` として記載し、ユーザーに報告する。

### B-4. 取り込み済み項目のアーカイブ

`取り込み済み` になった項目は、詳細を本体から切り離してアーカイブへ移す。

1. `upstream-merge-status.md` の「詳細」から、その項目の `### <ID>: ...` セクションを
   **本文そのまま (状態行や develop-kasumi 側コミットも含めて) 切り取る**。
2. `upstream-merge-archive.md` の「詳細 (取り込み済み)」へ、**ID順を保って**貼り付ける。
   内容は書き換えない。
3. `upstream-merge-status.md` の**一覧表の行はそのまま残す** (状態 = `取り込み済み`)。
   一覧表から削除してはいけない。

逆方向 (アーカイブ済み項目に本家の後続コミットが現れた場合) は、
詳細セクションを本体の「詳細」へID順で戻し、状態を `一部取り込み` にして内訳を `備考` に書く。

---

## 作業後

1. ドキュメント全体を読み直し、次を確認する:
   - 一覧表の項目数 = 本体の詳細セクション数 + アーカイブの詳細セクション数
   - 一覧表で `取り込み済み` の項目の詳細が**アーカイブ側にのみ**存在する
   - それ以外の状態の項目の詳細が**本体側にのみ**存在する
2. 変更内容 (追加した項目、状態を変えた項目、要確認事項) をユーザーに簡潔に報告する。
3. **コミットはしない。** `develop-kasumi` は直接コミット禁止ブランチ (`CLAUDE.md` 参照)。
   コミットが必要な場合はユーザーの明示的な指示を待つ。
