# 本家 (OTRP-KUTAv6) 取り込み状況管理表

`develop-kasumi` ブランチが本家ブランチ `OTRP-KUTAv6` の変更を
**どこまで取り込んだか / 意図的に取り込んでいないか** を管理する文書。

- `develop-kasumi` は私用安定版。本家の新機能は無条件には取り込まず、
  バグ修正と安定した機能のみを cherry-pick する運用。
- 本文書は本家コミットを **意味ベースでグループ化した「項目」** 単位で管理する。
  1コミット = 1項目とは限らない (関連する後続バグ修正は元の機能項目に統合する)。

## 追跡範囲

| キー | 値 |
| --- | --- |
| 起点コミット (base) | `d01a02edf26e0faf1c3542d4d854fe6ce52cc9f8` (v58_3) |
| 追跡済み最新コミット (head) | `5b254a9c285e913dc2987505d289a57853b08d76` (increment OTRP_VERSION_PATCH / v63_0_1) |
| 対象コミット数 | 148 |
| 最終更新 | 2026-11-04 |

> `head` より新しい本家コミットが増えた場合は skill `update-upstream-status` で追記する。

## 編集ルール (人間 / LLM 共通)

1. 項目IDは `FEAT-nn` (新機能追加) / `FIX-nn` (バグ修正) / `MISC-nn` (その他)。
   **一度振ったIDは変更・再利用しない。** 追加は必ず末尾の連番。
2. `状態` に使う値は次の4つのみ:
   - `未取り込み` … 本家にあるが `develop-kasumi` には入れていない (既定値)
   - `取り込み済み` … cherry-pick 済み。`develop-kasumi 側コミット` を必ず記入する
   - `取り込み不要` … 意図的に見送り。理由を `備考` に必ず書く
   - `一部取り込み` … 項目の一部のみ取り込み済み。内訳を `備考` に書く
3. 状態を変えたら **一覧表と詳細セクションの両方** を更新する。
4. v58_3 より後に追加された機能に対するバグ修正は、独立項目にせず
   **元の機能項目の「上流コミット」に追記** する。
5. 本家に新しい機能が入り、既存項目の続きと判断できる場合も同様に既存項目へ追記する。
   判断に迷う場合は新規項目にし、`備考` に関連項目IDを書く。
6. 状態が `取り込み済み` になった項目の**「詳細」セクションは
   [upstream-merge-archive.md](upstream-merge-archive.md) へ移設する**。
   一覧表の行は本文書に残したまま (状態は `取り込み済み`)、詳細のみを移す。
   移設した項目に新たな本家コミットが現れた場合はアーカイブから本文書へ戻し、
   状態を `一部取り込み` に直す。

---

## 一覧表

状態が `取り込み済み` の項目は、この一覧表にのみ残し、
詳細は [upstream-merge-archive.md](upstream-merge-archive.md) に退避している。

| ID | 分類 | 概要 | 状態 |
| --- | --- | --- | --- |
| FEAT-01 | 新機能追加 | プレイヤー(会社)数上限を15社→63社に拡張 | 未取り込み |
| FEAT-02 | 新機能追加 | 他編成との発車待ち合わせ (発車許可の授受) | 未取り込み |
| FEAT-03 | 新機能追加 | 水上車両(船)の編成連結対応 | 未取り込み |
| FEAT-04 | 新機能追加 | 停滞課税 (`penalty_wait_for_two_month`) | 未取り込み |
| FEAT-05 | 新機能追加 | 駅からの収入 (`base_revenue_from_halt`) | 未取り込み |
| FEAT-06 | 新機能追加 | ツールバーのスクロールバー対応 | 未取り込み |
| FEAT-07 | 新機能追加 | タイル長設定 (`tile_length`) と実距離(m)統計 | 未取り込み |
| FEAT-08 | 新機能追加 | 道路車両の方向転換(reverse)対応 | 未取り込み |
| FEAT-09 | 新機能追加 | 経路表示の拡充 (ミニマップ表示・スケジュール全経路・所要時間) | 未取り込み |
| FEAT-10 | 新機能追加 | 路線一覧のメモ欄をフィルタ対象に追加 | 未取り込み |
| FEAT-11 | 新機能追加 | 財務グラフの収入内訳 (旅客/郵便/貨物) と輸送量内訳 | 未取り込み |
| FEAT-12 | 新機能追加 | 送金ダイアログの小数点以下入力対応 | 未取り込み |
| FEAT-13 | 新機能追加 | 一方通行標識の詳細設定に矢印図とコピー/貼り付け | 未取り込み |
| FEAT-14 | 新機能追加 | MCP サーバに `get_tile_info` ツールを追加 | 未取り込み |
| FEAT-15 | 新機能追加 | 道路車両の編成連結対応と誘導標識 (guide signal) | 未取り込み |
| FEAT-16 | 新機能追加 | 1タイルに2本の way を斜めで共存させる (異なる waytype / 同一 waytype、2本の脚への停留所建設を含む) | 未取り込み |
| FEAT-17 | 新機能追加 | 保守費・走行費の倍率設定 (`maintenance_cost_multiplier_*` 等) | 未取り込み |
| FEAT-18 | 新機能追加 | 航送 (編成を他の編成に積載して輸送する) | 未取り込み |
| FEAT-19 | 新機能追加 | 無閉塞運行 (`drive_without_reservation`) | 未取り込み |
| FEAT-20 | 新機能追加 | choose 標識の「長さを無視」設定 (`ignore_length`) | 未取り込み |
| FEAT-21 | 新機能追加 | 停留所の私有化で道・架線の所有権も移転し、費用を精算する | 未取り込み |
| FEAT-22 | 新機能追加 | 車両オフセット付き線路で対向2編成が同一タイルに進入できる | 未取り込み |
| FEAT-23 | 新機能追加 | 追越モードに `exclusive_area` / `passing_lane_stop_only` を追加 | 未取り込み |
| FEAT-24 | 新機能追加 | ミニマップ全体の PNG 出力 (ツール・MCP・Squirrel API からの出力を含む) | 取り込み済み |
| FEAT-25 | 新機能追加 | マップエディタの建物等をサムネイルで選択する | 取り込み済み |
| FEAT-26 | 新機能追加 | スケジュール編集で選択した停留所を先頭にする (Make first) | 取り込み不要 |
| FEAT-27 | 新機能追加 | 市街建築物の選択状態をプリセットとして保存/読込する | 未取り込み |
| FEAT-28 | 新機能追加 | makeobj 60.12 形式の pak 読み込み (建物 v12・標識の追加画像リスト・近接斜め画像) | 未取り込み |
| FEAT-29 | 新機能追加 | 建物の高さ (`height_clearance`) を使った高架・橋・建物配置の建設判定 | 未取り込み |
| FEAT-30 | 新機能追加 | 着陸待ちの航空機が空いている別の滑走路を選ぶ (choose runway) | 未取り込み |
| FEAT-31 | 新機能追加 | 誘導路・滑走路 (`air_wt`) で私有道路標識を有効にする | 未取り込み |
| FEAT-32 | 新機能追加 | 表示設定ダイアログにキー設定タブを追加 (`menuconf.tab` と同期) | 未取り込み |
| FEAT-33 | 新機能追加 | サーバー接続ダイアログのアドレス入力欄の内容をローカルに保存する | 未取り込み |
| FEAT-34 | 新機能追加 | 信号情報ウィンドウで「信号を撤去」ボタンを区切り線の下・右寄せに配置 | 未取り込み |
| FEAT-35 | 新機能追加 | 産業の情報ウィンドウで基本生産量・出荷単位・自動拡張の禁止を設定する | 未取り込み |
| FEAT-36 | 新機能追加 | Simutrans standard 125.0.1 までのセーブデータを読み込む | 未取り込み |
| FEAT-37 | 新機能追加 | choose 標識で同じ停留所に複数の編成を進入させる (`allow_multi_convoys`) | 未取り込み |
| FEAT-38 | 新機能追加 | 駅の集客範囲を waytype ごと・海タイル用に設定する (`station_coverage[...]` / `station_coverage_water`) | 未取り込み |
| FEAT-39 | 新機能追加 | 需要がなければ区間ごと飛ばす停車設定、連結待ちの判定更新、貨物の単一目的地積載 | 未取り込み |
| FIX-01 | バグ修正 | choose 標識/choose 区間終端が経路上にある場合の判定 | 取り込み済み |
| FIX-02 | バグ修正 | pak の `clip_below` 設定の読み込み順序 | 取り込み済み |
| FIX-03 | バグ修正 | 一方通行詳細設定の waytype 判定漏れと経路喪失時の異常終了 | 取り込み済み |
| FIX-04 | バグ修正 | 積込中に「次の停留所へ」を押した際の停車時間等の未更新 | 取り込み済み |
| FIX-05 | バグ修正 | 経路予約解除ツールが連結子編成を解放してしまう | 取り込み済み |
| FIX-06 | バグ修正 | 橋脚が滑走路 (`air_wt`) タイル上に建つ | 取り込み済み |
| FIX-07 | バグ修正 | Windows GDI の DPI スケーリング時のリサイズ処理 | 取り込み済み |
| FIX-08 | バグ修正 | 整地ツールが基礎 (`fundament`) タイルで高低差制限を無視する | 取り込み済み |
| FIX-09 | バグ修正 | 誘導標識を使わない軌道系連結で、ホーム上の待機編成を見つけられない | 取り込み済み |
| FIX-10 | バグ修正 | 道路が無いタイルでの道路参照による異常終了と、連結/解放時の `route_index` 不整合 | 取り込み済み |
| FIX-11 | バグ修正 | 一方通行詳細設定の標識がある道路で、一方通行モード (`oneway_mode`) の進行方向制限が経路判定に効かない | 取り込み済み |
| FIX-12 | バグ修正 | 市街建築物を複数タイルの建物に建て替える際の異常終了 | 取り込み済み |
| FIX-13 | バグ修正 | `-load net:` で起動したときに pakset のチェックサムを照合しない | 取り込み済み |
| FIX-14 | バグ修正 | 編成の譲渡・会社統合で会社ごとの編成数統計が更新されない (旧セーブの再集計を含む) | 取り込み済み |
| FIX-15 | バグ修正 | 車庫の車両名フィルタ文字列のコピー長の誤り | 取り込み済み |
| FIX-16 | バグ修正 | パスワード画面と会社ロック関連のネットワークコマンドの不具合 | 取り込み済み |
| FIX-17 | バグ修正 | シンプルツール番号のずれ (`TOOL_HALT_PERMISSION` を enum 末尾へ移動) | 取り込み済み |
| FIX-18 | バグ修正 | ズーム時・坂の境界で地面画像が隣接タイルの道を上書きする描画不具合 | 取り込み済み |
| FIX-19 | バグ修正 | 複数タイルの建物が道・停留所・他の建物と重なって建つ/壊れた断片が残る | 取り込み済み |
| FIX-20 | バグ修正 | 編成が載っているタイルを経路に補う処理 (`insert_route_convoy_on()`) が同じタイルを再探索して無限ループする | 取り込み済み |
| FIX-21 | バグ修正 | ロード時に記述子が見つからなかった建物を削除する際の異常終了 | 取り込み済み |
| FIX-22 | バグ修正 | 連結編成の発車時刻枠 (departure slot) の予約・解放の不整合 | 一部取り込み |
| FIX-23 | バグ修正 | 編成の停車駅一覧で車庫エントリに停車長を表示する/現在の停車駅番号が範囲外のとき異常終了する | 取り込み済み |
| FIX-24 | バグ修正 | 航空機が長い停留所から前進/横向きに出発しても押し戻し (pushback) 扱いになる | 取り込み済み |
| FIX-25 | バグ修正 | 発車条件を満たすまで待った編成が、到着時刻を基準にした過去の発車枠を予約してすぐ発車する | 取り込み済み |
| MISC-01 | その他 | 本家バージョン番号のインクリメント (v59〜v63_0_1) | 取り込み不要 |
| MISC-02 | その他 | 日本語訳 `ja.OTRP.tab` の更新 (v59〜v63分) | 未取り込み |
| MISC-03 | その他 | SDL3 バックエンド対応と CI のビルド構成更新 (Ubuntu 分は後に削除) | 未取り込み |
| MISC-04 | その他 | `obj_t::finish_rd()` にセーブの OTRP バージョンを渡す基盤変更 | 未取り込み |
| MISC-05 | その他 | OTRP 用アドオン pak (矢印・ダミー貨物) を `documentation/` に同梱 | 未取り込み |

---

## 詳細

`取り込み済み` の項目の詳細は [upstream-merge-archive.md](upstream-merge-archive.md) を参照。

### FEAT-01: プレイヤー(会社)数上限を15社→63社に拡張

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 1マップあたりの会社数上限を15社(+公共事業)から63社(+公共事業)へ拡張する。
- **上流コミット**:
  - `bd29d593f` player number 63 (#613) — 本体
  - `d272db352` BUG FIX:FIX some bugs of max_player=63 (#691) — 後続修正
  - `eb9dbf485` BUG FIX: alpha channel image fix (#692) — 後続修正
  - `cff33e0d2` BUG FIX: update the number of player_unowned (#693) — 後続修正
  - `03654a9dd` BUG FIX: player frame and player merge frame update (#704) — 後続修正
  - `7b8e2a174` BUG FIX: fix player color for large player_id (#773) — 後続修正
- **主な変更箇所**:
  - `simconst.h` — `MAX_PLAYER_COUNT` 16→64、`PLAYER_UNOWNED` 15→63
  - `obj/roadsign.{h,cc}`, `gui/privatesign_info.{h,cc}`, `simtool.cc` — 私有道路マスクを64社分へ拡張、既定値設定
  - `display/simgraph16.cc` — 会社色用の特殊色インデックス計算と画像データ確保の拡張、アルファチャンネル画像の描画修正 (`eb9dbf485`)
  - `network/network_cmd*.{h,cc}`, `network/network_socket_list.{h,cc}`, `nettools/nettool.cc` — 会社番号の型と範囲を拡張
  - `dataobj/loadsave.{h,cc}` — 会社番号 rdwr 用ヘルパーを追加 (`cff33e0d2`)
  - `boden/wege/weg.cc`, `obj/simobj.cc`, `simcity.cc`, `simconvoi.cc`, `simfab.cc`, `simhalt.cc` — `PLAYER_UNOWNED` 番号変更に伴う所有者判定の修正 (`cff33e0d2`)
  - `gui/loadsave_frame.cc` — 旧形式 (OTRP v59 未満) で保存する際の警告・会社統合フロー
  - `gui/player_frame_t.{h,cc}`, `gui/player_merge_frame.{h,cc}` — 会社一覧/会社統合ダイアログにスクロールバー追加、幅計算修正 (`03654a9dd`)
  - `dataobj/gameinfo.cc`, `dataobj/scenario.{h,cc}`, `dataobj/settings.cc`, `simworld.{h,cc}`, `simhalt.{h,cc}`, `gui/halt_info.cc`, `script/api/api_map_objects.cc`, `script/api_param.cc`, `vehicle/simvehicle.cc`
  - `tests/automated-tests/tests/test_player.nut` — 上限63社に合わせてテスト更新
  - `documentation/ja.OTRP.tab` — 旧形式保存時の警告文言を追加 (`d272db352`)
  - `dataobj/settings.cc` — `set_player_color_to_default()` の既定色を色系統数 (`SPECIAL_COLOR_COUNT / 8` = 28) で折り返す。
    ランダム色選択の「使用済みの色」除外処理を修正 (`7b8e2a174`)
  - `player/simplay.cc` — `player_t::rdwr()` の読み込み時、範囲外の会社色 (`>= SPECIAL_COLOR_COUNT`) を既定色に補正 (`7b8e2a174`)
- **詳細**:
  - 会社番号を扱う多数の箇所を uint8 前提で見直し、フラグ用ビットマスクを64社分に拡張している。
  - 旧セーブ形式で保存しようとすると、ネットワークモードでは「16社以上あるため現行形式で保存する」旨を通知、
    シングルプレイでは「会社0へ統合が必要。SHIFT+保存で確定」という2段階確認になる。
  - `7b8e2a174`: 既定の会社色は `player_nr` (色1) / `player_nr + 3` (色2) をそのまま色系統番号にしていたため、
    会社番号が大きいと色系統数 28 を超えて不正な色になっていた。これを `% 28` で折り返す。
    ランダム色選択 (`default_player_color_random`) の除外処理は、比較対象の会社を取り違え
    (`test_sp` ではなく `player` を参照) かつ `1<<(色/8)` という誤った値を除外していたのを修正し、
    候補が尽きた場合は既定色にフォールバックする。ロード時の会社色補正に OTRP バージョンによる分岐は無い。
    なおコミット本文には発車案内板など無関係な squash 前コミットの件名が並ぶが、実際の差分は上記2ファイルのみ。
- **取り込み時の注意**:
  - **セーブデータ形式が変わる** (本家では OTRP v59 相当)。取り込む場合は `simversion.h` の
    `OTRP_VERSION_MAJOR` の扱い (MISC-01) を必ず併せて検討すること。
  - 変更範囲が非常に広くネットワーク互換性にも影響する。単独 cherry-pick の難度は高い。
  - `7b8e2a174` のうち、ランダム色選択の除外処理の誤り (`settings.cc`) は v58_3 時点から存在し、
    develop-kasumi (16社) にも同じコードがある。会社数拡張とは独立に `settings.cc` 部分だけ取り込むことも可能。
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-02: 他編成との発車待ち合わせ (発車許可の授受)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: ある停留所で、指定した路線の編成が到着するまで発車を待つ / 逆に他編成へ発車許可を出すまで待つ、という待ち合わせをスケジュールに設定できる。
- **上流コミット**:
  - `9b07e0936` wait for other convoy arrive (#683) — 本体 (待つ側)
  - `6de4593ac` update allow other convoy departure (#694) — 許可を出す側の追加と修正
  - `5461ae2d2` rename non-selecter departure line (#707) — 後続修正
- **主な変更箇所**:
  - `dataobj/schedule_entry.h` — 停留所フラグ `WAIT_FOR_OTHER_CONVOY` (1<<20) と `WAIT_ALLOW_DEPARTURE` (1<<21)、
    待ち合わせ相手路線 `linehandle_t allow_depart_line` を追加。`operator==` の比較対象にも追加
  - `dataobj/schedule.cc` — スケジュール表示の属性文字に `O` / `o` を追加
  - `simconvoi.{h,cc}` — `waiting_for_departure_allowance_by_other_convoy`,
    `waiting_for_departure_make_another_convoy_depart`, `allow_other_convoy_to_depart(halthandle_t)`,
    `is_waiting_for_departure_allowance()` を追加
  - `gui/schedule_gui.{h,cc}` — `bt_wait_for_other_convoy`, `bt_wait_allow_convoy_depart`,
    `allow_depart_line_selector` (相手路線選択コンボボックス) と `init_allow_depart_line_selector()`
  - `vehicle/simvehicle.cc` — 停留所到着時に発車許可を発行する呼び出し
  - `documentation/ja.OTRP.tab` — 「発車許可待ち」「他編成の到着を待機」「他編成を発車させるまで待機」等 (MISC-02 参照)
- **詳細**:
  - 発車許可は1回の呼び出しにつき1編成のみに与えられる。自分自身および自分の連結子編成には許可を出さない。
  - `6de4593ac` で、通過駅 (pass stop) でも許可発行処理が呼ばれるよう修正され、
    「他編成に発車許可を出すまで待つ」側のフラグが分離された (通過駅には設定不可)。
  - `5461ae2d2` は `allow_depart_line_selector` の未選択時の表示を `<no line>` から
    「発車許可を出す路線を指定」(`Select Line Allow Departure`) に変更するだけの表記修正。
- **取り込み時の注意**:
  - `schedule_entry_t` にフラグとフィールドが増えるため、スケジュールのセーブ形式に影響する。
  - **FIX-22 の残りを合わせて適用すること。** FIX-22 は FEAT-02 依存部分を除いて取り込み済みで、
    `next_stop_button_pressed()` の `set_waiting_for_departure_make_another_convoy_depart(false)` が未適用
    (FEAT-18 も取り込む場合は `board_carrier()` 側にも1行ある)。詳細は FIX-22 の「未取り込み分」を参照。
  - **FIX-25 で除外したテストを戻すこと。** 本家 `1764fb965` の `test_departure_slot_allowance_timeout` (`loading_slot_timeout_case(0, 1 << 20)`) を `test_loading_departure_slot.nut` と `all_tests.nut` に追加する。
- **develop-kasumi 側コミット**: —
- **備考**: `develop-kasumi` にはスケジュールのインポート/エクスポート機能 (`dbe156c0b` ほか) があるため、
  `schedule_entry_t` の変更を取り込む場合は入出力側の追従が必要。

### FEAT-03: 水上車両(船)の編成連結対応

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: これまで対象外だった水上車両 (船) でも編成の連結・解放ができるようにする。
- **上流コミット**:
  - `7a3b9ad24` water vehicle convoy can couple (#690)
- **主な変更箇所**:
  - `gui/schedule_gui.cc` — 連結設定を表示する waytype 判定 (`coupling_waytype`) から `water_wt` を除外。
    船の場合は連結設定変更時に `reverse_convoi_coupling` を自動リセットしない
  - `simconvoi.{h,cc}` — `is_same_waterway(convoihandle_t)` を追加。
    同一タイル上でなくても同じ水域にいる編成を連結相手として探せるようにする。
    1つの待機編成に2編成以上が同時に連結しないよう制御
  - `vehicle/simvehicle.cc` — 水上車両の連結時処理を追加
- **詳細**: 連結停留所では逆向き編成の連結も許可される。水上編成は進行方向を連結順の決定に使わない。
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-04: 停滞課税 (`penalty_wait_for_two_month`)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 2か月以上「進行不能待ち」状態の編成に対し、積載旅客の運賃相当額をもとにした罰金を課す設定を追加する。
- **上流コミット**:
  - `e33ab3ccc` ADD Tax of Teitai (#571)
- **主な変更箇所**:
  - `dataobj/settings.{h,cc}` — `bool penalty_wait_for_two_month` を追加 (既定 false)。
    `simuconf.tab` パラメータ、および `rdwr()` で OTRP v59 以上のとき読み書き
  - `gui/settings_stats.cc` — 設定ダイアログ「経済」タブにチェックボックスを追加
  - `simconvoi.cc` — `convoi_t::new_month()` で `state == WAITING_FOR_CLEARANCE_TWO_MONTHS` の編成について、
    積載旅客数 × 運賃 から罰金額を算出し、公共事業へ通行料として支払わせる
    (`book_toll_received` / `book_toll_paid` / `CONVOI_WAYTOLL` / `CONVOI_PROFIT`)
- **詳細**: 罰金額は旅客運賃相当額に月の長さと速度係数を掛け、`>> 20` と `/3000` で正規化した値。waytype は先頭車両のものを使う。
- **取り込み時の注意**: `settings_t::rdwr()` が OTRP v59 でゲートされているためセーブ形式に影響する (MISC-01 参照)。
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-05: 駅からの収入 (`base_revenue_from_halt`)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 停留所で乗車した旅客数と駅施設の規模に応じて、駅の所有者に収入を計上する設定を追加する。
- **上流コミット**:
  - `7c358b21b` revenue from halt (#572)
- **主な変更箇所**:
  - `dataobj/settings.{h,cc}` — `sint32 base_revenue_from_halt` を追加 (0で無効)。`simuconf.tab` パラメータ、OTRP v59 以上で rdwr
  - `gui/settings_stats.cc` — 設定ダイアログ「経済」タブに数値入力を追加
  - `simhalt.{h,cc}` — `book_pax_boarding_revenue(uint16)` を追加。統計項目 `HALT_REVENUE` を追加し、
    `financial_history` の rdwr を「元の8項目 + OTRP v59 以上で `HALT_REVENUE`」という構成に変更
  - `simconvoi.cc` — `convoi_t::hat_gehalten()` で乗車旅客数 (`pax_boarded`) を集計して上記を呼ぶ
  - `gui/halt_info.cc` — 駅情報ダイアログに収入グラフを追加
- **詳細**:
  - 収入 = `base_revenue_from_halt × capacity_factor × 乗車人数 / 10000`。
  - `capacity_factor` は旅客扱い可能な駅施設の level から算出 (`min(lv/2+1, 2)` の総和、上限50)。
  - 旅客が混雑 (overcrowded) している駅では収入なし。実際に乗車した旅客のみが対象。
- **取り込み時の注意**: `haltestelle_t::rdwr()` の統計配列レイアウトが変わる。セーブ形式に影響する (MISC-01 参照)。
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-06: ツールバーのスクロールバー対応

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: メインメニューバー/ツールバーにアイコンが収まりきらない場合にスクロールバーを表示し、ドラッグやホイールでスクロールできるようにする。
- **上流コミット**:
  - `dfd7e39c6` update toolbar (#700) — 本体
  - `6cf6f5dd7` BIG FIX: scroll bar position when menubar not on the top (#706) — 後続修正
  - `fffd2145a` BUG FIX:re-render when move scroll of menubar (#708) — 後続修正
- **主な変更箇所**:
  - `dataobj/environment.{h,cc}` — `iconsize_set_by_pak` (menuconf.tab の `icon_height` をテーマ再読み込みで上書きさせない)、
    `menu_scrollbar_thickness` (=10px) を追加
  - `gui/tool_selector.{h,cc}` — `is_scrollbar_dragging`, `get_scrollbar_rect()`, `get_scroll_metrics()`,
    `get_tool_count()`, `get_icon_area_offset()` (`6cf6f5dd7`)、
    `last_tool_icon_disp_start` / `is_being_dragged()` (`fffd2145a`) を追加。
    1行のみのツールバーは水平、複数行のグリッドは行単位の垂直スクロール
  - `gui/simwin.{h,cc}` — `get_main_menu_scrollbar_extra()` を追加し、メニューバーの実占有量をウィンドウ側とマップ表示側で共有
  - `display/simview.cc`, `display/simgraph16.cc`, `gui/gui_theme.cc`, `gui/welt.cc`, `gui/enlarge_map_frame_t.cc`, `simmenu.cc`
  - `simmain.cc` — `tool_t::read_menu()` の呼び出しをテーマ読み込みの**後**に移動 (menuconf.tab の `icon_height` を最終値にするため)
- **詳細**:
  - スクロール不要なときはスクロールバーを描画しない。アイコン高さ0による除算を回避。テーマ再読み込み時もアイコンサイズを維持。
  - `6cf6f5dd7` は `env_t::menupos` がメニューバーを画面下端/右端に置いている場合に、スクロールバー帯の分だけアイコン領域を外側にずらす修正。
  - `fffd2145a` は (a) スクロール位置が変わったフレームでアイコン領域全体を再描画する
    (空アイコンのセルに前フレームの絵が残る問題)、(b) ドラッグ中は `is_hit()` を外れても
    `simwin.cc` 側がイベントをツールバーへ回し続ける (ドラッグが途中で止まる問題)、
    (c) 空メニューがある場合に最後のアイコンまで表示する、の3点の修正。
- **取り込み時の注意**: 描画・GUI レイアウト全般に影響するため、取り込み後は各 `menupos` 設定で表示確認が必要。
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-07: タイル長設定 (`tile_length`) と実距離(m)統計

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 1タイルの実長 (メートル) を設定できるようにし、編成/路線の走行距離をタイル数ではなく実距離 (m) でも記録・表示する。
- **上流コミット**:
  - `697437937` add and update tile length (#699)
- **主な変更箇所**:
  - `dataobj/settings.{h,cc}` — `sint32 tile_length` と `calc_default_tile_length()` を追加
  - `gui/settings_frame.cc`, `gui/settings_stats.cc` — 設定ダイアログ「一般」タブに `tile_length` 入力を追加。
    ネットワークモードでは編集不可 (途中変更で全統計が再スケールされるため)
  - `simconvoi.{h,cc}` — 統計項目 `CONVOI_DISTANCE_METERS` を追加。`add_running_cost()` で
    斜め移動は `pak_diagonal_multiplier` で補正して加算。OTRP v60 未満のセーブは
    `CONVOI_DISTANCE × tile_length` で移行
  - `simline.{h,cc}` — 統計項目 `LINE_DISTANCE_METERS` を追加 (同様の移行処理)
  - `simworld.{h,cc}` — `karte_t::recalc_distance_new_records(old, new)` を追加し、`tile_length` 変更時に既存統計を再スケール
  - `gui/convoi_info_t.cc`, `gui/schedule_list.cc` — 「走行距離 (m)」を表示
- **詳細**: —
- **取り込み時の注意**: 統計配列が拡張されるためセーブ形式に影響する (本家では OTRP v60。MISC-01 参照)。
- **develop-kasumi 側コミット**: —
- **備考**: —

### FEAT-08: 道路車両の方向転換(reverse)対応

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: これまで対象外だった道路 (`road_wt`) でも編成の方向転換 (reverse) を許可する。
- **上流コミット**:
  - `01f6e630e` allow reverse for road waytype (#684)
- **主な変更箇所**:
  - `dataobj/environment.h` — `env_t::reversible_waytype()` から `road_wt` の除外を削除
  - `simconvoi.{h,cc}` — `bool reversing_lane_hold` と `calc_reversing_lane_tiles()` を追加。
    物理的な方向転換で強制した車線を、編成長+1タイル分だけ車線リセット処理から保護する。
    `reversing_lane_hold` はセーブ対象
  - `vehicle/simvehicle.{h,cc}` — `road_vehicle_t::can_return_to_traffic_lane()` を追加。
    方向転換中の車線維持、側面画像の挿入チェック、`screen_offset` の修正、対向車チェック
  - `gui/schedule_gui.cc` — reverse 非対応 waytype では `bt_reverse_coupling` を無効化。スペーサ配置を調整
- **詳細**: —
- **取り込み時の注意**: OTRP の車線制御 (overtaking / lane keep) と密接に絡むため、道路交通の挙動全般に影響しうる。
- **develop-kasumi 側コミット**: —
- **備考**: `develop-kasumi` には車庫内コピーで編成反転状態を保持する独自改造 (`245bcfcc7`) があるため干渉に注意。
  また FEAT-15 (道路の編成連結) が本項目の `calc_reversing_lane_tiles()` を前提にしているため、
  FEAT-15 を取り込む場合は本項目が先行する。

### FEAT-09: 経路表示の拡充 (ミニマップ表示・スケジュール全経路・所要時間)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 経路のハイライト表示をミニマップにも広げ、さらに編成/スケジュールの
  **全区間の経路** をマップ上に表示し、走破に要する推定時間とタイル数を併記する。
- **上流コミット**:
  - `abb9c92a0` show route in minimap (#702) — 本体 (ミニマップ表示)
  - `3fa861f6a` show convoy and schedule route (#712) — 機能拡張 (スケジュール全経路 + 所要時間)
  - `a25a51a31` BUG FIX: the value of route time hours (#718) — 後続修正
- **主な変更箇所**:
  - `gui/minimap.{h,cc}` — `vector_tpl<koord3d> highlighted_route_tiles` と
    `set_highlighted_route()` / `clear_highlighted_route()` を追加。`draw()` で `COL_SOFT_BLUE` の矩形として描画。`init()` でクリア
  - `gui/convoi_info_t.cc` — `convoi_info_t::show_route()` からミニマップへ経路を設定/クリア。
    スケジュール全経路の表示中は編成経路を描かずボタンを無効化 (`3fa861f6a`)
  - `gui/schedule_list.cc` — `show_route_cache()` からも同様に設定/クリア。
    経路キャッシュ設定が無効でも、路線所属編成の実経路にフォールバックして表示する。
    ボタンの有効条件を「経路キャッシュ有効」から「路線に編成が1つ以上ある」に変更。
    `clear_route_tile_flags(route_t&)` を追加 (`3fa861f6a`)
  - `gui/route_display.{h,cc}` — `schedule_route_overlay_t` (show/hide/poll/route_ready) と
    `format_route_time_hours(uint32 ticks)` を追加 (`3fa861f6a`)
  - `simworld.{h,cc}` — `request_schedule_route()` / `clear_schedule_route()` / `step_schedule_route()` /
    `get_schedule_route()` / `is_schedule_route_complete()` / `is_schedule_route_active()` /
    `is_schedule_route_pending()` / `get_schedule_route_count()` (`a25a51a31`) を追加。
    経路は `karte_t::step()` から **1 step につき停留所間 1 区間ずつ** 計算する (`3fa861f6a`)
  - `simconvoi.{h,cc}` — `static convoi_t::calc_ticks_until_arrival(cnv, route_tiles, add_stop_time)` を追加 (`3fa861f6a`)
  - `gui/halt_info.cc` — 発車案内板の到着時刻推定を上記 `convoi_t::calc_ticks_until_arrival()` に移譲 (`3fa861f6a`)
  - `gui/schedule_gui.{h,cc}` — `bt_show_line_route`, `lb_route_time`, `route_overlay`,
    `get_route_reference_convoi()` (virtual) を追加 (`3fa861f6a`)
  - `gui/convoi_stops_list_t.{h,cc}` — `bt_show_whole_route`, `lb_route_time`, `route_overlay` を追加 (`3fa861f6a`)
  - `gui/line_management_gui.{h,cc}` — `get_route_reference_convoi()` を路線の先頭編成でオーバーライド (`3fa861f6a`)
- **詳細**:
  - スケジュール全経路はクライアントローカルな表示専用データで、セーブには残らない。
    `karte_t::step()` からのみ計算し、GUI からは経路探索を走らせない。
  - 航空 (`air_wt`) のスケジュールは `calc_route()` が経路を返さないため対象外 (ボタンを出さない)。
  - 所要時間は `calc_ticks_until_arrival()` の推定値。`a25a51a31` で
    `ticks × spacing_shift_divisor / ticks_per_world_month` (uint64 演算、0除算回避) に計算式を修正し、
    表示にタイル数を追加した。
- **取り込み時の注意**: `karte_t` に表示用の静的状態と `step()` からの呼び出しが増える。
  セーブ形式には影響しない。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/minimap.h` の `highlighted_route_tiles` (本体) と
  `simworld.h` の `request_schedule_route` (拡張分)。

### FEAT-10: 路線一覧のメモ欄をフィルタ対象に追加

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 路線一覧ダイアログの絞り込みで、路線名だけでなく路線メモの内容も検索対象にできるチェックボックスを追加する。
- **上流コミット**:
  - `b46764653` using memo as filter (#701)
- **主な変更箇所**:
  - `gui/schedule_list.h` — `button_t bt_memo_filter` を追加
  - `gui/schedule_list.cc` — `bt_memo_filter` (square_state) を生成・配置し、
    `build_line_list()` の絞り込み条件に「メモ欄との照合」を追加 (`utf8caseutf8(l->get_memo(), schedule_filter)`)
- **詳細**: —
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/schedule_list.h` の `bt_memo_filter`。
  なお路線メモ機能そのもの (`inp_memo` 等) は v58_3 時点で既に存在する。本項目はそれを
  フィルタ条件に加える差分のみ。

### FEAT-11: 財務グラフの収入内訳と輸送量内訳

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 会社の財務ダイアログに、収入の内訳 (旅客/郵便/貨物) と輸送量の内訳 (旅客/郵便/貨物) の
  行とグラフ曲線を追加する。
- **上流コミット**:
  - `bd3a24b48` add income goods category in finance graph (#717) — 収入内訳
  - `49ca28a97` add transported passenger,post,good graph (#719) — 輸送量内訳
- **主な変更箇所**:
  - `gui/money_frame.h` — `MAX_PLAYER_COST_BUTTON` 13 → 16 (`bd3a24b48`) → 19 (`49ca28a97`)
  - `gui/money_frame.cc` — `cost_type_name` / `cost_type_color` / `cost_type` / `label_type` /
    `cell_to_buttons` / `cell_to_moneylabel` に `ATV_REVENUE_PASSENGER` / `ATV_REVENUE_MAIL` /
    `ATV_REVENUE_GOOD` と `ATV_TRANSPORTED_PASSENGER` / `ATV_TRANSPORTED_MAIL` /
    `ATV_TRANSPORTED_GOOD` の行を追加。`MONEY_FRAME_ROWS` と
    `FIRST_REVENUE_BREAKDOWN_BUTTON` マクロを新設し、内訳ボタンは親行の下に右寄せで配置
  - `simcolor.h` — `COL_REVENUE_PAS` / `COL_REVENUE_MAIL` / `COL_REVENUE_GOOD` /
    `COL_TRANSPORTED_PAS` / `COL_TRANSPORTED_MAIL` / `COL_TRANSPORTED_GOOD` を追加
- **詳細**: 集計値そのものは既存の `finance_t` の統計 (`ATV_REVENUE_*` / `ATV_TRANSPORTED_*`) を
  使うだけで、新規の統計項目は追加していない。表示行が9行→15行に増える。
- **取り込み時の注意**: セーブ形式への影響なし。財務ダイアログの縦幅が大きくなる。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simcolor.h` の `COL_TRANSPORTED_PAS`。

### FEAT-12: 送金ダイアログの小数点以下入力対応

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 会社間送金の入力欄を「整数部 . 小数部」の2欄構成にし、円表記 (`show_yen`) の
  設定を `simuconf.tab` と設定ダイアログからも切り替えられるようにする。
- **上流コミット**:
  - `0536cff05` send money update (#709)
- **主な変更箇所**:
  - `gui/money_frame.{h,cc}` — `gui_numberinput_t write_money_cents` (0〜99) を追加。
    `get_balance_divisor()` (円表記なら10、そうでなければ1000) を新設し、上限計算と送金額計算を切り替え。
    送金後に入力欄を0にリセット
  - `dataobj/settings.cc` — `settings_t::parse_simuconf()` に `show_yen` を追加
  - `gui/settings_stats.cc` — 設定ダイアログ「一般」タブに `show_yen` のチェックボックスを追加
- **詳細**: `env_t::show_yen` が真のときは小数入力欄を隠し、入力値をそのまま最小単位として扱う。
- **取り込み時の注意**: `env_t::show_yen` 自体は v58_3 時点で既に存在する。セーブ形式への影響なし。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/money_frame.h` の `write_money_cents`。

### FEAT-13: 一方通行標識の詳細設定に矢印図とコピー/貼り付け

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 一方通行標識の詳細設定 (`detailed_oneway`) を、チェックボックス群だけでなく
  交差点の矢印図をクリックして切り替えられるようにし、設定を他の標識へコピー/貼り付けできるようにする。
- **上流コミット**:
  - `eaae6f8b2` add arrow and copy/paste buttons in one-way detail setting (#715)
- **主な変更箇所**:
  - `gui/onewaysign_info.h` — `gui_oneway_diagram_t` (クリック可能な交差点図コンポーネント) を新設。
    `onewaysign_info_t` に `diagram`, `lb_diagram`, `bt_copy`, `bt_paste`、
    静的クリップボード `clip_ns` / `clip_ow` / `clip_valid`、`apply_exit_toggle(row, col)` を追加
  - `gui/onewaysign_info.cc` — 矢印の描画とクリック判定、コピー/貼り付け時のツール発行
  - `documentation/ja.OTRP.tab` — 「通行方向の詳細設定をコピー/貼り付け」「通行可能な方向」等 (MISC-02 参照)
- **詳細**: 図の行は進入方向 (S / W / N / E)、列は退出方向 (`ribi_t::nesw` 順)。
  クリックすると `row*4 + col` の値でリスナが呼ばれ、該当の右左折可否ビットが反転する。
  コピー/貼り付けはツールの `n` / `e` パラメータと同じパック形式でクラス静的変数に保持する。
- **取り込み時の注意**: `detailed_oneway` 機能自体は v58_3 時点で既に存在する。セーブ形式への影響なし。
- **develop-kasumi 側コミット**: —
- **備考**: 同じ作業ブランチから派生した FIX-03 も参照。
  状態確認用の識別子は `gui/onewaysign_info.h` の `gui_oneway_diagram_t`。

### FEAT-14: MCP サーバに `get_tile_info` ツールを追加

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 内蔵 MCP サーバに、指定タイルの地形種別・傾斜・載っているオブジェクトを
  JSON で返すツール `get_tile_info` を追加する。
- **上流コミット**:
  - `02ebbec21` mcp server tool update (#711)
- **主な変更箇所**:
  - `network/mcp_tools.cc` — `tool_get_tile_info()` と補助関数
    `json_get_int()` / `obj_type_name()` / `ground_type_name()` / `append_slope_json()` を追加。
    `TOOL_DEFS[]` に `get_tile_info` の定義 (x, y 必須 / z 任意) を追加
- **詳細**: 返す内容は `ground_type`、`is_water` / `is_ground` / `is_bridge` / `is_tunnel` /
  `is_elevated` / `is_underground`、地面と way の `slope` (角の高さと建設可否フラグ)、
  タイル上のオブジェクト一覧 (index / type / type_name / name / owner)。
  `z` を省略すると地表 (kartenboden)、指定するとその高さのみを参照しフォールバックしない。
- **取り込み時の注意**: セーブ形式への影響なし。MCP サーバはデバッグ用途のみ。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `network/mcp_tools.cc` の `tool_get_tile_info`。

### FEAT-15: 道路車両の編成連結対応と誘導標識 (guide signal)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: これまで除外されていた道路 (`road_wt`) でも編成の連結・解放をできるようにし、
  併せて道路の choose 標識に「連結待ちの編成をここで止める」誘導標識 (guide signal) の設定を開放する。
- **上流コミット**:
  - `ef383b890` allow coupling for road vehicle (#726) — 本体
  - `0261ddf43` BUG FIX: wait-for-coupling just after end of the shipped (#729) — 後続修正 (下船直後の向き + `road_wt` の `end_of_guide` 開放)
- **主な変更箇所**:
  - `vehicle/simvehicle.{h,cc}` — `road_vehicle_t` に `can_couple()`, `is_coupling_partner()`,
    `is_coupling_target()`, `is_valid_coupling_partner()`, `guide_route()`,
    `is_seeking_coupling_partner()`, `is_on_coupling_approach()`, `is_in_guide_area()` (`0261ddf43`)、
    および `check_next_tile(bd, need_electric, find_route, coupling)` オーバーロードを追加。
    連結相手は「反転なしで後ろに付ける相手」= 同方向を向いた待機連結列の末尾のみ
  - `boden/wege/strasse.{h,cc}` — `get_reserver()` を追加。
    `reserve()` が拒否した際に、その予約主が許容できる相手 (連結相手) かどうかを呼び出し側で判定できるようにする
  - `simconvoi.{h,cc}` — `broadcast_lane_to_coupling_convois()` を追加。
    車線 (`tiles_overtaking`) は編成ごとに持つが決定するのは先頭編成だけなので、子編成へ伝搬させる
  - `gui/schedule_gui.cc` — `coupling_waytype` の除外を `air_wt` のみに縮小 (`road_wt` を連結可に)。
    道路は自動反転設定 (`bt_reverse_default`) を出さず、`bt_reverse_coupling` は連結可能 waytype で表示
  - `gui/onewaysign_info.{h,cc}` — 道路 choose 標識に `bt_guide_signal` を追加
  - `gui/end_of_choose_info.cc`, `simtool.cc` — `end of choose signal` / `try coupling convoy not enter here`
    の表示条件と `tool_change_roadsign_t` の `'c'` / `'g'` から `road_wt` の除外を削除 (`0261ddf43`)
  - `gui/depot_frame.cc` — 道路の車庫でも連結関連の表示を行う
- **詳細**:
  - 誘導標識は、連結相手を探している編成に対して「相手が目的の停留所に居るまで標識で待たせる」もの。
    `guide_route()` は `can_enter_tile()` からのみ呼ばれ、相手不在の間は false を返して標識で停止させる。
  - 連結接近中の経路は choose 標識による経路差し替えの対象外とする (`is_on_coupling_approach()`)。
  - `0261ddf43` は航送 (FEAT-18) で下船した編成が適当な方向を向いてしまい、連結相手として
    検出されない/正面衝突向きの相手として検出される問題の修正 (下船時に次の停留所へ向く向きを選ぶ)。
- **取り込み時の注意**:
  - `vehicle/simvehicle.cc` への変更が 470 行規模で、OTRP の車線制御・追い越し処理と同じ領域を触る。
    `develop-kasumi` の道路まわり独自改造との競合に注意。
  - FEAT-08 (道路の方向転換) で追加された `calc_reversing_lane_tiles()` を前提にしているため、
    **FEAT-08 を先に取り込む必要がある**。
  - 旧セーブの道路 choose 標識のフラグ互換処理は MISC-04 側にあるため、併せて検討すること。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simconvoi.h` の `broadcast_lane_to_coupling_convois`、
  または `boden/wege/strasse.h` の `get_reserver`。

### FEAT-16: 1タイルに2本の way を斜めで共存させる

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 1つのタイル上で2本の way が**互いに交わらない斜め (disjoint bend)** を描く場合、
  交差物 (`crossing_t`) を作らずに両方を敷設できるようにする。
  当初は異なる waytype 同士のみだったが、後続コミットで**同一 waytype 同士**
  (互いに接続されない2本の独立した線路/道路) にも拡張された。
- **上流コミット**:
  - `c2973aaec` build two different waytype in one tile as disjointed diagonal (#705) — 本体 (異なる waytype)
  - `58bb2b989` make two same waytype as different way (#738) — 機能拡張 (同一 waytype 同士)
  - `1588dba70` BUG FIX: load failed (#745) — 後続修正 (ロード時の予約処理)
  - `280ced3e8` BUG FIX: fix crash bug of v62 (#747) — 後続修正 (片脚が1方向のままのタイルのロード失敗等。**コミットの一部のみ**。残りは FIX-10)
  - `891191546` add grund_t::has_two_same_waytype_ways() and fix vehicle_t::get_ribi() (#751) — 後続修正 (経路探索・予約・`get_ribi()` の同一 waytype 2脚対応。**コミットの一部のみ**。残りは FIX-11)
  - `20710ff85` BUG FIX: merge two ways in same waytype (#780) — 後続修正 (2脚の統合条件・所有者/停留所/市道の扱い)
  - `c99642b40` BUG FIX: electricity check for two-way tile update (#787) — 後続修正 (2脚タイルの架線による電化の判定)
  - `b5703efaf` BUG FIX: diagonal images of wayobj (#789) — 後続修正 (2本の脚があるタイルの架線等の画像)
  - `5367977aa` allow build halt on closed diagonal way (#792) — 機能拡張 (2本の脚があるタイルに停留所を建てる)
- **主な変更箇所**:
  - `dataobj/ribi.h` — `ribi_t::are_disjoint_bends(x, y)` を新設
    (両方が2ビットのカーブで、方向ビットを共有しない = NW と SE、NE と SW の組)
  - `boden/wege/schiene.h` — 既存の `can_co_reserve_dirs()` を上記ヘルパーへ置き換え (挙動同一)
  - `boden/grund.cc` — `neuen_weg_bauen()` と `rdwr()` の読み込み時に、
    disjoint bend なら `needs_crossing()` が真でも `crossing_t` を作らない/直さない
  - `boden/wege/weg.cc` — `check_diagonal()` で、同タイルの別 waytype と disjoint bend の関係にあれば
    周囲のタイルによらず常に `IS_DIAGONAL` を立てる (対角の別々の角を通すため)
  - `bauer/wegbauer.cc` — `check_crossing()` の例外追加、および `bautyp==luft` (滑走路/誘導路) で
    「他 waytype の way があるタイルは不可」というルールに disjoint bend の例外を追加
  - `obj/crossing.cc` — `crossing_t::finish_rd()` で、両 way が disjoint bend になっていたら
    画像を `IMG_EMPTY` にして不活性化する (旧セーブに残った `crossing_t` 対策)
  - `tests/automated-tests/tests/test_diagonal_two_waytypes.nut` (新規), `tests/automated-tests/all_tests.nut`
  - **以下は `58bb2b989` (同一 waytype 拡張) の分**:
  - `boden/grund.h` — `get_weg(waytype_t, ribi_t::ribi dir)` と
    `get_weg_ribi_unmasked(waytype_t, ribi_t::ribi dir)` を新設。同一 waytype の2本が同居するタイルで、
    方向ビット `dir` を持つ側の way を選び分ける。`weg_erweitern()` / `neuen_weg_bauen()` に
    `new_desc` と `allow_same_waytype_dual_leg` 引数を追加
  - `boden/grund.cc` — `weg_erweitern()` で (a) 両脚にまたがる ribi が来たら2本を1本に統合、
    (b) どちらの脚を延長すべきかを「追加後も bend/single のままか」で判定、
    (c) disjoint な bend に対しては延長を拒否して呼び出し側に2本目を作らせる。
    `weg_entfernen()` で脚が single に退化したら2本を統合し直す (その際に停留所があれば撤去)。
    `get_neighbour()` を方向考慮の ribi 参照に変更
  - `bauer/wegbauer.{h,cc}` — `straight_route_mode` を追加。**CTRL 押下 (`calc_straight_route()`) の
    直線敷設時のみ** 同一 waytype の2本目を許可する。`check_crossing()` に同一 waytype 2本の例外を追加
  - `dataobj/route.{h,cc}` — `route_t::get_corner_set(index)` を新設
    (経路上のタイルで占有する2方向ビット = `schiene_t::reserve()` の予約方向)
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::get_current_corner_set()` を新設し、
    way 参照を `get_weg(waytype, corner_set)` に置き換え (`enter_tile()` / `leave_tile()` 含む)
  - `simconvoi.{h,cc}` — `get_reserved_tiles_corner_set(index)` を新設。予約/解除の way 参照を方向考慮に変更
  - `obj/wayobj.cc` — 架線が同一 waytype 2本のどちらにも架かるよう `finish_rd()` / `calc_image()` を修正
  - `boden/wege/weg.cc` — `check_diagonal()` に加え、ホーム長計算ループの無限ループ修正
    (`get_neighbour()` は失敗時に出力引数を書き換えないため、同変数を受け渡しに使うと抜けられない)。
    way 情報ウィンドウに「このタイルには同種の別の way がある」旨を表示
  - `simtool.cc` — way 検査ツール類の ribi 参照を方向考慮に変更
  - `tests/automated-tests/tests/test_diagonal_two_waytypes_same_desc.nut` (新規),
    `tests/automated-tests/tests/test_way_road.nut`
  - `simconvoi.cc` — `convoi_t::rdwr()` のロード時予約で `get_current_corner_set()` を使わず、
    ロード済みの経路から `route.get_corner_set(route_index-1)` を引く (`1588dba70`)
  - **以下は `280ced3e8` の分**:
  - `dataobj/ribi.h` — `ribi_t::are_disjoint_legs(x, y)` を新設。`are_disjoint_bends()` の緩和版で、
    片方が1方向 (single) でもよい (両方とも single の場合は不可 = 2本の行き止まりはタイル中心で出会う)
  - `boden/grund.cc` — `rdwr()` 読み込み時と `neuen_weg_bauen()` の判定を `are_disjoint_legs()` に変更。
    交差物が必要なのに `crossing_desc` が見つからない場合も `dbg->fatal()` せず、交差物なしでロードを続行する
  - `boden/wege/weg.cc` — `needs_crossing()` は同一 waytype 同士なら常に false。
    `check_diagonal()` を `are_disjoint_legs()` 基準にし、同一 waytype の相手も対象にする
  - `obj/crossing.cc` — `finish_rd()` で相手の way が無い場合は画像を空にして不活性化 (NULL 参照回避)。
    判定を `are_disjoint_legs()` に変更
  - `obj/roadsign.{h,cc}` — `roadsign_t::get_weg_here()` を新設。同一 waytype 2本のタイルで、
    標識自身の `dir` に沿った脚を選ぶ (`finish_rd()` / `set_dir()` / デストラクタで使用)
  - `simtool.cc` — 停留所建設時の disjoint 判定を `are_disjoint_legs()` に変更
  - `tests/automated-tests/tests/test_diagonal_two_waytypes{,_same_desc}.nut`, `all_tests.nut` —
    `*_partial_leg` テストを追加
  - **以下は `891191546` の分**:
  - `boden/grund.h` — `grund_t::has_two_same_waytype_ways()` を新設。`has_two_ways()` と異なり、
    路面電車+道路や踏切など**異なる waytype** の同居タイルでは偽になる
  - `dataobj/route.{h,cc}` — 経路探索の closed list を `route_closed_list_t` に置き換え、
    同一 waytype 2脚のタイルでは**脚 (`weg_t*`) 単位**で訪問済みを管理する (タイル単位だと
    環状経路が同じタイルの別の脚を後で通れない)。`find_route()` / `intern_calc_route()` の両方が対象。
    `set_start_heading()` と `start_heading` / `next_start_heading` を追加し、始点タイルで
    どちらの脚に乗っているかを指定できるようにする (未指定なら両脚を試す。次の `calc_route()` 1回限り有効)
  - `simconvoi.cc` — `drive_to()` で旧経路の進行方向から始点の脚を `set_start_heading()` で渡す
    (キャッシュ経路使用時はクリア)。経由地ごとの区間探索も前区間の到着方向を引き継ぐ。
    環状判定 (`looped`) は同一 waytype 2脚のタイルでは同じ脚を再訪した場合のみ真にする。
    `reserve_route()` で占有タイルの予約記録を位置検索ではなく `reserved_tiles` の先頭から外す。
    `get_reserved_tiles_corner_set()` / `get_reserved_tiles_travel_dir()` で隣接していない要素を
    方向計算に使わない (`neighbour_ribi()` を新設)。単独要素は自経路から脚を引く
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::get_ribi(gr, from_dir)` の脚選択を
    `has_two_same_waytype_ways()` の場合に限定し、詳細一方通行でなければ脚の**マスク済み** ribi
    (`leg->get_ribi()`) を返すよう修正 (従来は同居タイルなら異なる waytype でも脚の非マスク ribi を返し、
    標識・信号の方向制限を無視していた)。`hop_check()` / `check_next_tile()` (road/water/air に
    `prev` 付きオーバーロードを追加) / `can_enter_tile()` / `is_target()` / `check_longblock_signal()` の
    way 参照を方向考慮に変更。`block_reserver()` / `leave_tile()` で同一 waytype 2脚のもう一方の脚を
    予約・解除しない (互いに待ち合ってデッドロックするため)。`leave_tile()` で経路から脚が分からない
    場合 (車庫からの出庫等) は自編成が予約している方の脚を解放する。
    `air_vehicle_t::get_ribi(gr, from_dir)` / `get_ribi_of_way()` を新設
  - **以下は `20710ff85` の分**:
  - `boden/grund.{h,cc}` — `grund_t::would_merge_same_waytype_legs(wegtyp, ribi)` を新設。
    `weg_erweitern()` の2脚統合条件を「両脚にまたがる ribi」に加えて「片脚だけに触れるが、その脚が分岐 (bend/single でなくなる) になる ribi」にも広げ、
    統合時にタイル上の停留所を撤去する。延長する脚の選択で「既にその方向を持つ脚」を最優先にする
  - `bauer/wegbauer.{h,cc}` — `way_builder_t::check_dual_leg_merges()` を新設し、`calc_straight_route()` / `calc_route()` の後に呼ぶ。
    統合が起きるタイルで、どちらかの脚が他社所有、または停留所がある場合は経路を空にして建設を拒否する。
    `build_road()` / `build_track()` では、既にその方向を持つ脚があれば `weg_erweitern()` を呼ばず延長済み扱いにする
    (静的関数 `is_direction_already_built()`)
  - `simcity.cc` — 静的関数 `can_city_merge_road_legs()` を新設。市の道路建設 (`stadt_t::build_road()`) で、
    統合先の脚が市道化禁止 (`get_avoid_cityroad()` かつ分岐不可) または停留所がある場合は接続しない。
    隣接タイルへの ribi 追加を `ribi_add()` 直接呼び出しから `weg_erweitern()` 経由に変更 (2脚のどちらに足すかを正しく選ぶため)
  - **以下は `c99642b40` の分**:
  - `boden/grund.{h,cc}` — 2脚の統合処理を `grund_t::merge_same_waytype_legs(removed_leg)` に切り出し
    (`weg_erweitern()` と `weg_entfernen()` から使用)。統合後に架線の `update_electrification()` を呼ぶ。
    `weg_erweitern()` の電化済み way への ribi 追加で、`get_wayobj()` が NULL の場合の参照を回避
  - `obj/wayobj.{h,cc}` — `set_dir()` をインラインから実装へ移し、`update_electrification()` を新設。
    2脚タイルでは1つの架線が両脚にまたがるので、`dir` が届いている脚だけを電化する。
    デストラクタ・`finish_rd()` の電化/非電化処理を静的関数 `electrify_way()` / `unelectrify_way()` に整理
  - `vehicle/simvehicle.cc` — `vehicle_t::hop()` の速度制限参照と `rail_vehicle_t::is_target()` の way 参照を
    進入方向考慮 (`get_weg(waytype, entry_bit)`) に変更 (片方の脚だけ電化されている場合に対応)
  - **以下は `b5703efaf` (架線等の画像) の分**:
  - `boden/wege/weg.cc` — `calc_image()` / `check_diagonal()` / `check_close_diagonal()` で隣のタイルの way を引くときに
    方向を渡し、自分につながる脚だけを見る
  - `obj/wayobj.{h,cc}` — `two_legs` / `leg_diagonal` / `leg_dir_a` / `leg_dir_b` を追加し、1つの `wayobj_t` が両脚にまたがるときに
    脚ごとの画像を描き分ける。2本目の脚は `display()` / `display_after()` のオーバーライドから `display_extra_image()` で描く
  - `obj/simobj.h` — `obj_t::display()` を `virtual` にする
  - **以下は `5367977aa` (停留所の建設) の分**:
  - `obj/gebaeude.{h,cc}` — 2本目のホームの `tile2` と、`get_tile2()` / `set_tile2()` / `get_tile_toward()` / `set_tile_toward()` /
    `get_platform_count()` / `get_diagonal_layout_ribi()` を追加。`rdwr()` は **OTRP v63 以上**で `tile2` の番号を読み書きする
  - `bauer/hausbauer.{h,cc}` — `build_station_on_closed_diagonal()` と `calc_diagonal_station_layout()` を追加
  - `simtool.cc` — `tool_station_aux()` で両脚が停留所の waytype のカーブ、かつ建物のレイアウトが48以上なら両脚にホームを置く。
    後から2本目の脚ができた場合は同じ停留所を建て直すと新しいホームの分だけ課金して追加する。
    `tool_rotate_building_t` は2つのホームの前後の組み合わせ4通りを順に切り替える
  - `simhalt.cc` — 容量と所有権移転時の保守費を `get_platform_count()` 倍にする。2本の脚のタイルを路面電車の併用軌道と誤認しない
  - `bauer/wegbauer.cc` — `check_building()` で `tile2` の脚も接続方向に含める
- **詳細**:
  - 代表的な用途は「線路の NW カーブと誘導路の SE カーブを同じタイルに置く」ような配置。
    両者はタイル中心を共有しないため、物理的にも視覚的にも交差しない。
    `crossing_desc` が定義されていない組み合わせ (monorail+track、track+air など) も敷設可能になる。
  - `58bb2b989` は同じ仕組みを**同一 waytype** に広げたもの。同じタイルに互いに接続されない
    2本の線路 (道路) を置けるようになり、以降 way を引くコードは
    「waytype だけでは一意に決まらない」前提で `corner_set` (前タイル側ビット + 次タイル側ビット) を
    渡して選び分ける必要がある。両脚にまたがる ribi を敷設すると2本は1本に統合される。
  - `1588dba70` は、`convoi_t::rdwr()` のロード時予約で `vehicle_t::get_current_corner_set()` を
    呼んでいたのが原因のロード失敗の修正。この時点では車両がまだ編成に接続されていない
    (`set_convoi()` は `finish_rd()` で行われる) ため経路を参照できない。
  - `280ced3e8` (FEAT-16 分) は、2本目の脚を1タイルずつ敷設している途中 (まだ1方向だけ) や、
    片側のタイルを撤去した後の状態で保存したセーブのロード失敗の修正。従来はロード時に
    「両方が完全なカーブ」の場合しか disjoint と認識しなかったため、交差物を作ろうとして
    `requested for waytypes %i and %i but nothing defined!` で異常終了していた
    (同一 waytype 同士や、rail と air のように交差物が無い組み合わせ)。
  - `891191546` (FEAT-16 分) は、同一 waytype 2脚のタイルを走行・予約するときの不具合修正群。
    経路探索がタイル単位の訪問済み管理のため2脚を順に通る環状経路を見つけられない、
    始点タイルでどちらの脚にいるか分からず逆側の脚から探索してしまう、
    2脚の両方を予約して対向編成同士が待ち合う、同じタイルを2度通る経路で予約記録を誤って外す、
    といった問題に対処している。
    コミット本文の履歴にある「add new OTRP mode: only_one_car and stop_only_halt」に対応する変更は
    差分に含まれていない (FEAT-23 側の変更は無い。作業ブランチ上のコミット名が残ったものと思われる)。
  - `20710ff85` は、2脚のタイルを通過する経路 (片脚の方向から入り、空いている方向へ抜ける) を敷設したときに
    2脚が統合されず、同じ方向へ出る way が2本できてしまう問題の修正。統合は両脚を作り直すので、
    両脚とも自社所有で停留所が無い場合に限って許可する (市の道路建設では市道化禁止の脚も対象外)。
  - `c99642b40` は、2脚のタイルで架線が片脚にしか架かっていない場合や、2脚の統合後に、
    電化状態と速度制限が正しく反映されない問題の修正。コミット本文の前半は `20710ff85` のブランチ履歴。
  - `b5703efaf` は、2本の脚があるタイルで架線等の画像が片方の脚の向きで描かれたり、隣のタイルの斜め判定が
    自分につながらない脚を見てしまったりする問題の修正。コミット本文の前半は `20710ff85` / `c99642b40` のブランチ履歴。
  - `5367977aa` は、これまで片方の脚にしか載せられなかった停留所を両脚に載せるもの。1つの建物が2つのホームの画像を持ち、
    費用・保守費・容量は2ホーム分になる。片方が単独タイルでもう片方がつながっている場合は単独のホームを表示しない
    (両方とも単独なら両方表示する)。
- **取り込み時の注意**:
  - `5367977aa` より前の分はセーブ形式への影響はないが、旧セーブに残る `crossing_t` の扱いが変わる。
  - **`5367977aa` はセーブ形式に影響する** (`gebaeude_t::rdwr()` が OTRP v63 でゲート。MISC-01 参照)。
  - `boden/grund.cc` / `bauer/wegbauer.cc` は基本エンジン部分なので、敷設判定全般への副作用に注意。
  - `58bb2b989` は `get_weg()` の呼び出し規約が変わる広範囲の変更で、`simconvoi.cc` /
    `vehicle/simvehicle.cc` の予約まわりを大きく書き換えるため、`develop-kasumi` 独自改造との競合に注意。
  - FEAT-22 (オフセット付き線路の対向同時進入) は `58bb2b989` の `corner_set` 基盤を前提にしているため、
    FEAT-22 を取り込む場合は本項目が先行する。
  - `58bb2b989` が追加した英語表示文字列 `This tile has a second, disconnected way of the same type:` の訳は
    本家 `dfbd77ae8` で `ja.OTRP.tab` に登録された (MISC-02 参照)。
  - `891191546` は `vehicle/simvehicle.cc` の予約まわり (`block_reserver()` / `leave_tile()` /
    `check_longblock_signal()`) と `simconvoi.cc` の `drive_to()` を広く触るため、FEAT-22 の
    `can_co_reserve_offset()` 周辺や `develop-kasumi` 独自改造と競合しやすい。
    また `route_t` にメンバ (`start_heading` 等) が増えるが、保存対象ではないのでセーブ形式への影響は無い。
  - `c99642b40` は `wayobj_t::finish_rd(const uint8 loaded_OTRP_version)` の新シグネチャ (MISC-04) と
    `close_diagonal` (FEAT-28) を前提にした文脈の上で書かれているため、それらが未取り込みだと素直に当たらない。
  - `b5703efaf` も `close_diagonal` (FEAT-28) を前提にしている (`develop-kasumi` に `close_diagonal` は無い)。
    追加のメンバは `calc_image()` で作り直すものでセーブ対象ではない。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/ribi.h` の `are_disjoint_bends` (本体)、
  `boden/grund.h` の `get_weg(waytype_t, ribi_t::ribi)` または
  `dataobj/route.h` の `get_corner_set` (同一 waytype 拡張分)、
  `dataobj/ribi.h` の `are_disjoint_legs` (`280ced3e8` 分)、
  `boden/grund.h` の `has_two_same_waytype_ways` (`891191546` 分)、
  `boden/grund.h` の `would_merge_same_waytype_legs` (`20710ff85` 分)、
  `obj/wayobj.h` の `update_electrification` (`c99642b40` 分)。
  `280ced3e8` は FIX-10 と、`891191546` は FIX-11 と同一コミットのため、cherry-pick する場合は
  両項目をまとめて扱うか、本項目の分だけを手作業で移植すること。
  **FIX-10 / FIX-11 の分は `develop-kasumi` に手作業で移植済み** (`65ed5e40d` / `e31fd6d88`)。
  本項目を取り込むときに両コミットを cherry-pick すると、移植済みの行 (`strasse_at()`、
  `reanchor_route_index()`、`get_active_ribi_mask_oneway()` 等) と重複・競合するので、
  本項目の分だけを移植すること。特に `vehicle_t::get_ribi(gr, from_dir)` は FIX-11 移植版
  (末尾で `gr->get_weg(get_waytype())` のマスクを掛ける形) を、本家版の脚選択 (`leg`) に合わせて書き換える必要がある。
  `5367977aa` 分の識別子は `obj/gebaeude.h` の `get_tile2`、`b5703efaf` 分は `obj/wayobj.h` の `leg_dir_a`。

### FEAT-17: 保守費・走行費の倍率設定

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: pakset が持つ「道の保守費」「架線等 wayobj の保守費」「車両の走行費」に対して、
  ゲーム側から % 倍率をかけられる設定を追加する。
- **上流コミット**:
  - `fa3506863` maintainance multiplier (#724) — 本体
  - `d98371388` vehicle runningcost update (#746) — 後続修正 (走行費の表示に倍率を反映)
- **主な変更箇所**:
  - `dataobj/settings.{h,cc}` — `maintenance_cost_multiplier_way` /
    `maintenance_cost_multiplier_overhead` / `running_cost_multiplier_vehicle` (いずれも既定 100) を追加。
    `parse_simuconf()` で読み込み (範囲 1〜250)、`rdwr()` は **OTRP v61 以上**で読み書きし、
    それ未満のロード時は 100 にリセット
  - `descriptor/way_desc.{h,cc}`, `descriptor/way_obj_desc.h` — `way_desc_t::get_maintenance()` /
    `way_obj_desc_t::get_maintenance()` を新設し、基底の `obj_desc_transport_related_t::get_maintenance()` を
    意図的に隠して倍率適用後の値を返す (実装はいずれも `way_desc.cc`)
  - `simconvoi.cc` — `add_running_cost()` で `base_sum_running_costs` に倍率を適用した
    `scaled_base_running_costs` を算出し、通行料・過積載補正・帳簿計上のすべてをそれ基準に変更
  - `gui/settings_stats.cc` — 設定ダイアログ「経済」タブに3つの数値入力を追加
  - `simutrans/config/simuconf.tab` — 3パラメータの既定値とコメントを追加
  - **以下は `d98371388` の分**:
  - `simconvoi.{h,cc}` — `convoi_t::get_running_cost()` を廃止し、倍率適用後の値を返す
    `get_running_cost_scaled()` に置き換え (`convoi_t::info()` も追従)
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::get_operating_cost()` をインラインから実装へ移し、倍率適用後の値を返す
  - `gui/convoi_detail_t.cc`, `gui/convoi_info_t.cc`, `gui/depot_frame.cc`, `gui/vehiclelist_frame.cc` —
    編成/車両の走行費表示 (`$/km`) を倍率適用後の値に変更
- **詳細**:
  - `world()` が未生成の間 (pakset 読み込み中など) は倍率をかけず素の値を返す。
  - `d98371388` は、実際の課金は倍率適用後なのに UI の走行費表示が素の値のままだった不整合の修正。
    保守費は `way_desc_t::get_maintenance()` 側で倍率を掛けているので表示も追従済みだが、
    車両の走行費は `vehicle_desc_t::get_running_cost()` が素の値のままなので、表示側で個別に倍率を掛けている。
- **取り込み時の注意**:
  - `settings_t::rdwr()` が `get_OTRP_version() >= 61` でゲートされているため
    **セーブ形式に影響する** (MISC-01 参照)。
  - `d98371388` は `convoi_t::get_running_cost()` を削除するため、`develop-kasumi` 独自コードに
    呼び出しが残っていないか確認すること。`vehicle_t::get_operating_cost()` は
    `gui/convoi_detail_t.cc` の最大収入計算 (`max_income`) でも使われており、そちらも倍率適用後の値になる。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/settings.h` の `maintenance_cost_multiplier_way`、
  `simconvoi.h` の `get_running_cost_scaled` (`d98371388` 分)。

### FEAT-18: 航送 (編成を他の編成に積載して輸送する)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 陸上編成をまるごと船 (キャリア編成) に載せて別の停留所まで運ぶ「航送」を追加する。
  航送中の編成はマップ上から消え、予約も持たず、連結関係を保ったまま運ばれる。
- **上流コミット**:
  - `62a5675b8` shipping (#722) — 本体
  - `cc6546766` fix can_deliver_shipped_to (#732) — 後続修正
  - `2df43b22c` update shipping toll and freight images (#733) — 後続修正
  - `8076cc1f8` add shipping income and fix toll shipping (#740) — 機能拡張 (航送の通行料率と収入分配)
  - `d38064348` change start shipping timing (#750) — 後続修正 (停車時の航送処理の実行順序)
  - `4a34b7c2c` BUG FIX: fix convoy detail tab width (#752) — 後続修正 (航送一覧表示の改行漏れ)
  - `cd625b481` BUG FIX: not shipping child convoys if they should be uncouple (#778) — 後続修正 (航送中の操作禁止・航送開始条件・キャリア表示ボタン)
  - `30427a5e5` remove doubly written shipped convoys (#786) — 後続修正 (積載情報への航送編成一覧の二重表示を削除)
  - `1b61233a2` BUG FIX: child convoy can ship other convoys (#785) — 後続修正 (連結子編成をキャリアにできるようにする)
  - `966b49132` add base revenue of shipping (#774) — 機能拡張 (航送の基本料金表、通行料のキャリア側への計上)
- **主な変更箇所**:
  - `bauer/goods_manager.{h,cc}` — waytype ごとの「航送用ダミー貨物」テーブル `shipping_goods[16]` と
    `get_shipping_goods(waytype_t)` / `is_shipping_goods(desc)` を追加。
    pakset から名前 (`SHIPPING_ROAD` / `SHIPPING_TRACK` / `SHIPPING_MONORAIL` / `SHIPPING_MAGLEV` /
    `SHIPPING_NARROWGAUGE` / `SHIPPING_WATER`) で1回だけ解決する。tram は track と共用。
    未定義の waytype は NULL のままで、その waytype は航送できない
  - `dataobj/schedule_entry.h` — 停留所フラグ `START_SHIPPED` (1<<23) を追加。
    キャリア側にフラグは不要 (積載能力とスケジュールから決まる。`NO_LOAD` が「ここでは積まない」を兼ねる)
  - `dataobj/schedule.cc` — スケジュール表示の属性文字に `S` を追加
  - `simconvoi.{h,cc}` — 状態 `SHIPPED` を追加 (**enum の末尾。序数比較している箇所が多数あるため**)。
    `shipped_convois`, `carrier_convoi`, `shipping_wait_since` を追加し、
    `board_carrier()`, `disembark_convoy()`, `handle_shipping_at_halt()`, `try_start_shipping()`,
    `can_ship()`, `can_deliver_shipped_to()`, `get_shipping_capacity()/_load()/_length()/_weight()`,
    `book_shipping_toll()`, `disembark_all_forced()`, `teleport_to_nearest_depot()`,
    `withdraw_from_map_for_shipping()`, `is_shipping_vehicle_loaded()` (`2df43b22c`) 等を追加
  - `gui/convoi_detail_t.{h,cc}` — 「航送中」「航送している編成一覧」「この編成を航送している編成を表示」の表示
  - `gui/schedule_gui.{h,cc}` — `bt_start_shipped` (航送開始待ち) を追加
  - `gui/depot_frame.cc` — 航送用ダミー貨物を積む車両が車庫一覧から除外されないようにする
  - `simworld.cc` — 追跡カメラが航送中の編成ではキャリアを追う、過剰在庫の自動整理が航送中編成を消さない、
    航送区間はスケジュール経路オーバーレイ (FEAT-09) の経路計算対象外にする
  - `simtool.cc`, `vehicle/simvehicle.cc` — 予約解除ツールの `SHIPPED` 除外、キャリア車両の積載画像
  - `dataobj/settings.{h,cc}` — `toll_shipping_percentage` / `shipping_income_percentage` を追加 (`8076cc1f8`)。
    `simuconf.tab` パラメータと OTRP v62 以上での rdwr。v62 未満のセーブは
    `toll_shipping_percentage = way_toll_runningcost_percentage` / `shipping_income_percentage = 0` に移行
  - `gui/settings_stats.cc` — 設定ダイアログ「経済」タブに上記2項目の数値入力を追加 (`8076cc1f8`)
  - `simconvoi.{h,cc}` — `shipping_income_carrier` (下船後、収入精算が済むまでキャリアを覚えておくハンドル) と
    `deduct_shipping_income_share()` を追加 (`8076cc1f8`)
  - `simconvoi.cc` — `hat_gehalten()` 内の航送処理 (`handle_shipping_at_halt()` / `try_start_shipping()`) を、
    積み下ろし台数の計算より前から、自編成と連結子編成の積み下ろし・発車条件の計算・方向転換判定の後
    (連結編成の反転処理の直前) へ移動 (`d38064348`)
  - `gui/convoi_detail_t.cc` — 「航送している編成一覧」ラベルで各編成の行末に改行を追加 (`4a34b7c2c`)
  - **以下は `cd625b481` の分**:
  - `simconvoi.cc` — 航送中 (`SHIPPED`) の編成に対して、スケジュール変更 (`set_schedule()` / `open_schedule_window()` /
    路線更新 `check_pending_updates()`)、車庫送り (`send_to_depot*()`)、休止 (`set_suspension()`)、譲渡 (`trade_convoi()`)、
    反転 (`reverse_vehicles_on_user_request()` / `reverse_convoy_coupling_by_user_request()`)、次の停留所へ (`next_stop_button_pressed()`) を禁止。
    `can_depart()` の連結・積載・発車許可の判定を静的関数 `check_departure_conditions()` に切り出し、
    航送開始待ちの登録 (`try_start_shipping()`) を `can_start_shipping()` (発車時刻指定を除く発車条件) が満たされてからにする。
    `handle_shipping_at_halt()` は、まだ待機登録していない編成 (`shipping_wait_since == 0`) や
    スケジュール編集中・休止中の編成を積まない。`shipping_wait_since` は 0 を「待機していない」の意味で使うため 0 を格納しない。
    `board_carrier()` で連結・解放・発車許可関係のフラグをリセットし、発車時・スケジュール変更時に `shipping_wait_since` をクリア
  - `simtool.cc` — `tool_change_convoi_t::init()` で、航送中の編成への売却・スケジュール・路線・状態・車庫・解放・
    次の停留所・反転・休止・譲渡受諾のコマンドを拒否する
  - `gui/convoi_info_t.{h,cc}` — 航送しているキャリア/航送中の編成の情報ウィンドウを開くボタン群
    (`container_shipping`, `shipping_buttons`, `update_shipping_buttons()`) を追加。航送中はスケジュール等のボタンを無効化
  - `gui/convoi_detail_t.cc` — 航送中は売却・車庫へ移動・譲渡・休止ボタンを無効化
  - `simconvoi.cc` — `get_freight_info()` 末尾の「Convoys aboard:」一覧 (編成詳細タブと重複) を削除 (`30427a5e5`)
  - **以下は `1b61233a2` の分**:
  - `simconvoi.cc` — `can_ship()` の「連結子編成はキャリアになれない」制限を撤廃し、代わりに
    候補の連結列全体について「自分自身を含む」「航送中の編成を運んでいる (入れ子航送)」場合を拒否する。
    `hat_gehalten()` で `handle_shipping_at_halt()` を連結子編成でも呼ぶ。
    `betrete_depot()` で連結子のキャリアの積荷も強制下船させる。
    `self_destruct()` で航送中の子編成の親との連結を手動で切る
  - `simtool.cc` — `tool_change_line_t` の編成削減ループで航送中/航送している編成を削除しない
  - **以下は `966b49132` の分**:
  - `dataobj/settings.{h,cc}`, `gui/settings_stats.cc` — 料金表 `shipping_revenue[7][8]` (積まれる waytype × キャリアの waytype) を追加。
    `simuconf.tab` のキーと経済設定の項目名は `revenue_<積まれる側>_shipped_by_<キャリア>`。`rdwr()` は **OTRP v63 以上**で読み書きする
  - `simconvoi.{h,cc}` — `book_shipping_revenue(c, landing)` を追加し、`disembark_convoy()` から呼ぶ。
    `book_shipping_toll()` はキャリア編成側にも `CONVOI_WAYTOLL` / `CONVOI_PROFIT` を計上するようにする
- **詳細**:
  - 積載量は「編成長さ単位」で、航送用ダミー貨物を積める車両の容量合計。
    空きが少しでもあれば乗り込むため、積載量を1編成分だけ超過しうる (デッドロック回避のため意図的)。
  - 降ろす駅は保存せず、航送開始時にスケジュールを目的地エントリまで進めて「現在のエントリ = 下船地」とする。
  - 航送中の編成はキャリアに対して `CONVOI_WAYTOLL` を支払い、キャリアは通行料収入として受け取る (`2df43b22c`)。
  - キャリアが破壊された/車庫に入った/下船地に到達できなくなった場合は、
    `disembark_all_forced()` で最寄り車庫へテレポート、それも無理なら除去する。
  - `cc6546766` は `can_deliver_shipped_to()` で `NO_LOAD` の停留所を「乗船駅に戻ってきた」と誤判定していた修正。
  - `8076cc1f8` は課金まわりの拡張。(a) 航送の通行料が道の通行料率 (`way_toll_runningcost_percentage`) を
    流用していたのを専用の `toll_shipping_percentage` に分離 (既定値は道の通行料率なので従来動作と同じ)、
    (b) 航送区間の運賃収入のうち `shipping_income_percentage` % をキャリア側の収入として分配する。
    運賃は下船した**次の**停留所で `last_stop_pos` (=乗船した港) からの区間として計上されるため、
    そこまでキャリアのハンドルを `shipping_income_carrier` に保持し、精算後にクリアする。
    分配分はキャリア自身の waytype で計上される (フェリーの収益が水運の列に載る)。
  - `d38064348` は航送の開始/終了タイミングの変更。コード移動のみだが、移動先はその停車での
    積み下ろし処理の後になるため、キャリアの積み下ろし・乗船側編成の積載が済んでから航送の乗降が行われる。
    移動したブロックのコメント (`Run before loading ...`) は移動前のまま残っており、実際の順序とは一致しない。
    PR 本文の履歴には最大積載・最小積載・重量計算等の複数コミットが並ぶが、squash 後の差分はこの移動のみ。
  - `4a34b7c2c` は、航送中の編成が複数あると一覧が1行に連結され、編成詳細タブの幅が広がってしまう問題の修正。
  - `cd625b481` は航送中の編成をプレイヤー操作から凍結する修正群。航送中の編成は現在のスケジュールエントリが
    下船地なので、スケジュール・状態を書き換えると下船地を失ったり `SHIPPED` 状態から外れたりする。
    また連結の解放が必要な子編成が航送開始待ちの登録を先に済ませてしまい、解放前に積まれる問題も、
    発車条件を満たしてから登録する形にして解消している。PR 本文の履歴の大半は FEAT-18 本体のブランチ履歴。
  - `1b61233a2` で、連結列の子編成 (例: 連結したフェリーの後部編成) もキャリアとして航送できるようになった。
    入れ子の航送 (航送している編成ごと航送する) は重量・画像・救済処理が対応していないため禁止のまま。
  - `966b49132` の料金は、設定値 (1/100 クレジット) × 乗船した港から下船地までの距離 × 積まれた連結列の長さ (タイル換算)。
    通行料として積まれた側の会社からキャリアの会社へ支払い、同じ会社の編成どうしでは発生しない。既定値 0 では何も起きない。
- **取り込み時の注意**:
  - **セーブ形式に影響する**。`convoi_t::rdwr()` が `get_OTRP_version() >= 61` で
    `carrier_convoi` / `shipped_convois` / `shipping_wait_since` を読み書きし、
    状態 enum の書き出しも v61 でゲートされている (MISC-01 参照)。
    さらに `8076cc1f8` により `convoi_t::rdwr()` の `shipping_income_carrier` と
    `settings_t::rdwr()` の2設定が **OTRP v62 以上**でゲートされている。
  - `schedule_entry_t` にフラグが増えるため、`develop-kasumi` 独自のスケジュール入出力
    (`dataobj/schedule_io.cc` の `stop_flag_names`) に `START_SHIPPED` の追加が必要。
  - 動作には pakset 側に航送用ダミー貨物が必要 (MISC-05 参照)。
  - `simconvoi.cc` への追加が 1100 行規模。単独 cherry-pick の難度は高い。
  - `cd625b481` で切り出された `check_departure_conditions()` は FEAT-02 の発車許可の判定を含む。
  - **FIX-22 の残りを合わせて適用すること。** FIX-22 (`f9ad42b26`) は `board_carrier()` の hunk を除いて取り込み済み
    (2026-10-31)。`board_carrier()` の連結列ループで枠の返却 (`release_departure_slot()`) 等を行う変更が未適用なので、
    本項目を取り込むときに FIX-22 の「未取り込み分」に書いたコードを `board_carrier()` へ入れる。
    そのうち `waiting_for_departure_make_another_convoy_depart` のリセットは FEAT-02 も必要。
  - `cd625b481` で追加された表示文字列 `Not allowed while the convoy is aboard another convoy.` は
    `ja.OTRP.tab` に未登録 (2026-10-27 時点)。
  - `966b49132` により `settings_t::rdwr()` の料金表が **OTRP v63 以上**でゲートされている (MISC-01 参照)。
    経済設定の画面に 56 項目 (7×8) が増え、項目名の訳は `ja.OTRP.tab` に未登録 (2026-11-04 時点)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simconvoi.h` の `get_shipping_capacity`、
  または `bauer/goods_manager.h` の `get_shipping_goods`。
  `966b49132` 分の識別子は `dataobj/settings.h` の `get_shipping_revenue`。

### FEAT-19: 無閉塞運行 (`drive_without_reservation`)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 軌道系 (track / monorail / maglev / narrowgauge / tram) のスケジュールで、
  その停留所から次の信号までを閉塞予約なしで運行する設定を追加する。
- **上流コミット**:
  - `85aef1543` Drive without reservation (#713) — 本体
  - `3fc2f3345` BUG FIX: infinite loop when only waypoint (#736) — 後続修正
  - `8ef07c84b` BUG FIX: reservation tiles when loading savedata (#742) — 後続修正
  - `3a889fd37` BUG FIX: check signal at the end of drive without reservation (#796) — 後続修正
- **主な変更箇所**:
  - `dataobj/schedule_entry.h` — 停留所フラグ `WITHOUT_RESERVATION` (1<<22) を追加
    (これに伴い `START_SHIPPED` が 1<<22 → 1<<23 へ移動)
  - `dataobj/schedule.cc` — スケジュール表示の属性文字に `N` を追加
  - `simconvoi.{h,cc}` — `bool drive_without_reservation` を追加。
    出発時に現在エントリのフラグを連結列全体へ設定し、信号/停留所に達するとリセットする。
    `rdwr()` は **OTRP v61 以上**で読み書き (それ未満は false)。
    併せて予約タイルの整合チェックで、`reserved_tiles` の先頭に前の経路のタイルが残っている場合に
    対応する `offset` 探索を追加
  - `vehicle/simvehicle.cc` — `rail_vehicle_t::block_reserver()` で、
    無閉塞運行指定の中継点に達したら予約を打ち切る
  - `gui/schedule_gui.{h,cc}` — `bt_drive_without_reservation` と
    `bt_all_without_reservation` (全停留所へ一括適用) を追加。軌道系 waytype でのみ表示
  - `vehicle/simvehicle.cc` — `rail_vehicle_t::can_enter_tile()` の無閉塞運行の終了処理で、`set_next_stop_index()` が
    信号のタイル自身を指すように引数を修正する (`3a889fd37`)
- **詳細**: `3fc2f3345` は、スケジュールが中継点のみで構成されている場合に
  `block_reserver()` 内の走査ループが終了せず無限ループになる問題の修正
  (`get_current_stop()` が uint8 のため終了条件 `i_stop != get_current_stop()-1` が成立しない)。
  ループをエントリ数で明示的に打ち切る形に書き換えている。
  `8ef07c84b` は下記「本家に未修正の不具合」に挙げていた `simconvoi.cc` の `?:` 優先順位バグの修正。
  `convoi_t::reserve_route()` の予約ループ上限を `(drive_without_reservation ? front()->get_route_index() : next_reservation_index)`
  と括弧で囲み、意図どおりの上限で予約するようにした。
  `3a889fd37` は、無閉塞運行の終わりの信号を普通の信号として判定させる修正。`set_next_stop_index(n)` は n+1 を保持するため、
  従来の値では信号より後ろを指して「信号が消えた」扱いになり、単純な閉塞予約だけをして
  choose / pre / longblock / priority の各信号の処理を飛ばしていた。コミット本文の前半は本体のブランチ履歴。
- **取り込み時の注意**:
  - `convoi_t::rdwr()` が `get_OTRP_version() >= 61` でゲートされているため **セーブ形式に影響する** (MISC-01 参照)。
  - `schedule_entry_t` にフラグが増えるため、`develop-kasumi` 独自のスケジュール入出力
    (`dataobj/schedule_io.cc` の `stop_flag_names`) に `WITHOUT_RESERVATION` の追加が必要。
  - **本家に未修正の不具合が同梱されている** (2026-11-04 時点の `origin/OTRP-KUTAv6` (v63_0_1) でも残存):
    - `simconvoi.cc:1536` (v63_0_1 時点の行番号) — `get_most_parent_convoi()->state==ROUTING_1;` が代入 `=` ではなく比較 `==` になっており、
      連結反転時の状態設定が効いていない (元は `state=ROUTING_1`)
    取り込む場合はこの点を直してから入れること。
  - `simconvoi.cc:367` の `?:` 優先順位バグ
    (`idx < drive_without_reservation?A:B && ...` が `(idx < drive_without_reservation) ? A : (B && ...)` と解釈される)
    は `8ef07c84b` で本家修正済み。**この後続修正も併せて取り込むこと。**
  - `3a889fd37` の差分は `get_current_corner_set()` を使う FEAT-16 の文脈の上で書かれている。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simconvoi.h` の `is_drive_without_reservation`。

### FEAT-20: choose 標識の「長さを無視」設定 (`ignore_length`)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 軌道系の choose 標識に「長さを無視」フラグを追加し、編成長より短い停留所にも
  進入させられるようにする。
- **上流コミット**:
  - `4c27b3eca` ignore length for choose signal (#743)
- **主な変更箇所**:
  - `obj/roadsign.h` — `choose_sign_flag` に `ignore_length` (1<<10) を追加し、
    `is_ignore_length()` / `set_ignore_length()` を新設
  - `gui/signal_info.{h,cc}` — choose 標識の情報ウィンドウに `bt_ignore_length` を追加
  - `simtool.cc` — `tool_change_roadsign_t` に `'i'` (ignore length のトグル) を追加
  - `dataobj/route.{h,cc}`, `ifc/simtestdriver.h` — `find_route()` / `is_target()` /
    `is_coupling_target()` に `ignore_length` 引数を追加
  - `vehicle/simvehicle.{h,cc}` — `rail_vehicle_t::is_target()` / `is_coupling_target()` で、
    `ignore_length` が立っていれば停留所長のチェックを省く。choose 標識からの
    `find_route()` 呼び出し4か所に `sig->is_ignore_length()` を渡す
- **詳細**: 従来は「編成長 ≦ 停留所の有効長 (+ margin)」を満たす停留所しか選ばれなかった。
  フラグを立てると長さ条件が 0 扱いになり、はみ出す停留所も選択対象になる。
  連結相手探索 (`is_coupling_target()`) も同様に長さ条件を無視する。
- **取り込み時の注意**: `choose_sign_flag` は `uint16` で `rdwr_short()` により保存されるため、
  ビット追加によるセーブ形式の分岐 (バージョン引き上げ) は不要。
  ただし `find_route()` / `is_target()` のシグネチャが変わるので、
  同じ関数群を触る他の項目 (FEAT-15 / FIX-09) と取り込み順に注意。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `obj/roadsign.h` の `is_ignore_length`。
  表示文字列 `ignore length` / `ignore length. Convoy can enter shorter stop` の訳は
  本家 `dfbd77ae8` で `ja.OTRP.tab` に登録された (MISC-02 参照)。

### FEAT-21: 停留所の私有化で道・架線の所有権も移転し、費用を精算する

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 公共停留所を会社の私有停留所にする操作 (SHIFT + 停留所公共化ツール) で、
  停留所建物だけでなく**同じタイルの道・架線 (wayobj) の所有権も移転**し、その対価を精算するようにする。
- **上流コミット**:
  - `a9c90e534` change owner when make halt private (#725)
- **主な変更箇所**:
  - `simhalt.{h,cc}` — `change_owner(player, halt_only, no_cost=false)` に引数を追加し、
    公共化と私有化の両方をこの1関数で扱うよう統合。
    私有化 (`to_private`) では権限ビット (`HS_ALLOW_OTHER_PLAYER_CONNECTION` / `set_permissions`) を
    新所有者のみに絞り、建物・道・wayobj の代金を「新所有者 → 旧所有者」方向で計上する。
    `make_private_and_join()` は統合先の停留所探索のみを担当する形に整理され、
    探索を所有権移転の**前**に行う (移転後は自分のタイルも一致してしまうため)
  - `simtool.cc` — `tool_make_stop_public_t::work()` の私有化分岐で、
    停留所の維持費だけでなく引き取る道・トンネル・wayobj の維持費も含めて所持金チェックを行う。
    公共化側の所持金チェックにも wayobj の維持費を加算 (漏れの修正)
  - `documentation/ja.OTRP.tab` — 「(%s) は私有化されました．」を追加 (MISC-02 参照)
- **詳細**:
  - `is_shift_pressed()` で私有化、`is_ctrl_pressed()` を併用すると公共事業モード (費用なし)。
  - ネットワークモードでは公共化・私有化の両方でメッセージを出す
    (従来は公共化のみ)。道については1タイル分だけ通知する。
- **取り込み時の注意**: 停留所の私有化機能自体は v58_3 時点で存在する (`[mod : shingoushori]`)。
  セーブ形式への影響はないが、権限ビットの扱いが変わるため既存セーブでの挙動確認が必要。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simhalt.h` の `change_owner( player_t *player, bool halt_only, bool no_cost )`
  (第3引数の有無)。

### FEAT-22: 車両オフセット付き線路で対向2編成が同一タイルに進入できる

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 車両オフセット (`vehicle_offset`) が設定された線路では左右にずれて描画されるため、
  対向する2編成が同じタイルを同時に予約・通過できるようにする。
- **上流コミット**:
  - `d61963fed` allow enter two convoys in one track when offset (#737)
  - `711ed3a0a` BUG FIX: savedata cannot be loaded when many convoys are in the same tile (#794) — 後続修正
- **主な変更箇所**:
  - `boden/wege/schiene.{h,cc}` — `reserved_travel_dir` / `reserved2_travel_dir` を追加し、
    `reserve()` / `can_reserve()` / `can_co_reserve_with()` に `travel_dir` 引数を追加。
    `can_co_reserve_offset(travel_dir)` を新設
  - `dataobj/route.{h,cc}` — `route_t::get_travel_dir(index)` を新設
    (タイルに進入する向き。`corner_set` と違い正反対の2方向を区別できる)
  - `simconvoi.{h,cc}` — `get_reserved_tiles_travel_dir(index)` を新設。
    `reserve_route()` / `vorfahren()` / `uncouple_convoi()` / `finish_rd()` の予約呼び出しに向きを渡す
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::get_current_travel_dir()` を新設し、予約系の呼び出しに反映
  - `vehicle/simvehicle.cc` — `rail_vehicle_t::leave_tile()` の同じタイルの編成の走査で、
    ロード途中の目印 `(convoi_t*)1` を飛ばす (`711ed3a0a`)
- **詳細**:
  - 同時進入が許されるのは、(a) 2つの向きが厳密に正反対の single ribi、
    (b) `vehicle_offset_mode == 0` (絶対オフセット) かつ `vehicle_offset != 0`、
    (c) タイルが直線または斜め (`is_twoway`) で分岐・交差でない、をすべて満たす場合のみ。
    オフセットモード1 (方向依存) は向きを `d%4` に畳むため両編成が同じ側になり、許可しない。
  - 予約方向 (`corner_set`) は正反対の2つの走行方向を区別できないため、走行方向を別フィールドで持つ。
  - `finish_rd()` (ロード直後) でも走行方向を渡さないと `reserved_travel_dir` が
    `ribi_t::none` のままになり、以後この機能が効かなくなるため併せて修正されている。
  - `711ed3a0a` は、同じタイルに複数の編成がいるセーブのロードに失敗する問題の修正。ロード中は編成がまだ接続されていない車両の
    `cnv` が `(convoi_t*)1` になっており、上記の走査がこれを本物の編成として参照していた。
- **取り込み時の注意**:
  - `schiene_t::reserve()` の呼び出し規約が FEAT-16 の `corner_set` 対応を前提にしている
    (`gr->get_weg(waytype, corner_set)` 経由で way を引く形)。**FEAT-16 (特に `58bb2b989`) が先行する**。
  - `reserved_travel_dir` はセーブ対象ではなく、ロード後に `finish_rd()` で再計算される。
    セーブ形式への影響はない。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `boden/wege/schiene.h` の `can_co_reserve_offset`。

### FEAT-23: 追越モードに `exclusive_area` / `passing_lane_stop_only` を追加

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 道路の追越モードに2種類を追加する。`exclusive_area` は連続した該当区間に
  同時に1編成しか入れない一編成専用区間、`passing_lane_stop_only` は追越車線を停車にのみ使える区間。
- **上流コミット**:
  - `b78c3685c` add new OTRP mode: only_one_car and stop_only_halt (#735)
- **主な変更箇所**:
  - `simtypes.h` — `overtaking_mode_t` に `exclusive_area_mode` (5) と
    `passing_lane_stop_only_mode` (6) を追加。両者を `prohibited_mode` に写す
    `effective_overtaking_mode()` を新設
  - `boden/wege/strasse.{h,cc}` — `get_overtaking_mode()` は写像後の値を返し、
    表示・保存・敷設用に `get_overtaking_mode_raw()` を新設
  - `gui/overtaking_mode.{h,cc}` — モード選択ボタンを6個→8個に拡張
  - `simconvoi.{h,cc}` — `can_stop_on_passing_lane()` を新設
  - `vehicle/simvehicle.{h,cc}` — `get_blocking_convoi_in_exclusive_area()` と
    `holds_passing_lane_to_stop()` を新設
  - `bauer/wegbauer.cc`, `boden/grund.cc`, `boden/wege/weg.cc`, `simtool.cc`,
    `vehicle/simroadtraffic.cc`, `script/api/api_map_objects.cc` — `get_overtaking_mode_raw()` への置き換え
  - `script/api/api_const.cc` — Squirrel 定数 `exclusive_area_mode` / `passing_lane_stop_only_mode` を追加
  - `documentation/ja.OTRP.tab` — 「一編成専用区間」「追越車線は停車のみ可」(MISC-02 参照)
- **詳細**: 追越モードは走行ロジック全体で**大小比較**されている
  (`<=oneway_mode`, `>twoway_mode` 等) ため、新モードは走行判定に渡る前に
  `prohibited_mode` へ写像し、追加ルールだけを別途チェックする設計になっている。
  連結待ちの相手編成は `exclusive_area` の占有相手とみなさない。
- **取り込み時の注意**:
  - `strasse_t::rdwr()` は `get_overtaking_mode_raw()` の値をそのまま保存するため
    セーブ形式のバージョン分岐は無いが、新モードを設定したセーブを**旧バイナリで読むと未知の値**になる。
  - `get_overtaking_mode()` の意味が変わるので、`develop-kasumi` 独自の車線制御改造がある箇所は
    `raw` を使うべきかどうか個別に確認が必要。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simtypes.h` の `effective_overtaking_mode`、
  または `boden/wege/strasse.h` の `get_overtaking_mode_raw`。

### FEAT-26: スケジュール編集で選択した停留所を先頭にする (Make first)

- **分類**: 新機能追加
- **状態**: 取り込み不要
- **概要**: スケジュール編集画面に「先頭へ」(`Make first`) モードを追加し、クリックした停留所が先頭になるよう
  順序を保ったままスケジュールを回転させる。
- **上流コミット**:
  - `a560206be` Add schedule make first button (#749)
- **主な変更箇所**:
  - `gui/schedule_gui.h` — 編集モード enum に `making_first` を追加、`button_t bt_make_first` を追加
  - `gui/schedule_gui.cc` — 「往復切符」ボタンの行 (`add_table(7,1)`) に `bt_make_first` (roundbox_state) を配置。
    追加/挿入/削除と排他のモードボタンとして扱い、`update_tool()` ではツールを解除する。
    モード中に停留所をクリックすると、`move_entry_backward()` を (行番号) 回繰り返して回転させ、
    その後は追加モードに戻る
  - `documentation/ja.OTRP.tab` — 「先頭へ」「選択したエントリが先頭となるようにスケジュールを並べ替えます。」(MISC-02 参照)
- **詳細**: Next line の引き継ぎ用ダミーエントリがある場合は、それを末尾に残したまま実エントリだけを回転させる。
  GUI 上の並べ替えのみで、編成の行き先 (`current_stop`) や発車枠の項目番号の追従は行わない。
- **取り込み時の注意**: セーブ形式への影響なし。
- **develop-kasumi 側コミット**: —
- **備考**: **取り込み不要 (2026-10-18 ユーザー判断)**。`develop-kasumi` に同等の機能を独自に実装済みのため
  (`84592de0b`「スケジュール編集画面に「この駅を先頭に」ボタンを追加」、ボタン `bt_rotate_to_front` / `schedule_t::rotate_to_front()`)。
  独自版はモードではなく選択中の停留所に対する即時ボタンで、路線編成への反映時の行き先の引き継ぎ、
  発車枠の項目番号の更新、所要時間記録の引き継ぎまで対応している。状態確認用の識別子は `gui/schedule_gui.h` の `bt_make_first`。

### FEAT-27: 市街建築物の選択状態をプリセットとして保存/読込する

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 市街建築物の配置ダイアログで複数選択した建物の組を「プリセット」としてファイルに保存し、
  後から読み込んで同じ選択状態を復元できるようにする (建物パレット)。
- **上流コミット**:
  - `42d5061e9` Add city building palette feature (#748)
- **主な変更箇所**:
  - `dataobj/citybuilding_preset.{h,cc}` (新規) — `citybuilding_preset_t` (建物名の配列) と
    `citybuilding_preset_load()` / `citybuilding_preset_save()` / `citybuilding_preset_recover()` /
    `citybuilding_preset_file_state()` を追加。1プリセット = 1ファイルの tab 形式 (`building[n]=<名前>`、`%XX` エスケープ)
  - `gui/citybuilding_preset_frame.{h,cc}` (新規) — `savegame_frame_t` 派生のファイル選択ダイアログ
    `citybuilding_preset_frame_t`。上書き時の確認ダイアログ、破損ファイルの復旧結果の通知
  - `gui/citybuilding_edit.{h,cc}` — `bt_preset_load` / `bt_preset_save`、`load_preset()` / `save_preset()` を追加。
    読込時は回転設定を維持する (`restoring_preset`)。未使用だった `lb_name_filter_input` を削除
  - `gui/savegame_frame.{h,cc}` — `close_after_ok()` (OK 後にウィンドウを閉じるか) と `get_input_filename()` を追加
  - `gui/simwin.h` — `magic_citybuilding_preset_confirm` / `magic_citybuilding_preset_file` を追加
  - `Makefile`, `Simutrans-Main.vcxitems` — 新規ソースの追加
  - `documentation/ja.OTRP.tab` — 「プリセットを読込」「プリセットを保存」等 16 語 (MISC-02 参照)
- **詳細**:
  - 保存先は `env_t::pak_dir + "building_preset/"` (使用中の pakset のディレクトリ配下)。
  - 書き込みは `.tmp` に書いてから置き換え、旧ファイルは一時的に `.bak` に退避する。
    中断で `.bak` / `.tmp` が残った場合は次回の読込/保存時に復旧し、壊れたファイルは `.corrupt[.n]` として残す。
  - 読込時は建物名で照合し、見つかった件数を「%u / %u 件を選択」で通知する (pakset に無い建物は無視)。
- **取り込み時の注意**: セーブ形式への影響なし。
  - 保存先が pakset のディレクトリなので、pakset を書き込みできない場所に置いている環境では保存できない。
  - FEAT-25 と同じ `gui/citybuilding_edit.cc` を触る。本家では FEAT-25 の後に入っているため、その順で取り込む方が競合が少ない。
  - `savegame_frame_t` に仮想関数が増えるため、`develop-kasumi` 側で同クラスを改造していれば追従が必要。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/citybuilding_preset.h` の `citybuilding_preset_t`、
  または `gui/citybuilding_edit.h` の `bt_preset_load`。

### FEAT-28: makeobj 60.12 形式の pak 読み込み

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: makeobj 60.12 で出力される pak を読み込めるようにする
  (建物ノード v12 の `height_clearance`、標識の追加画像リスト、way/架線の近接斜め (close diagonal) 画像)。
- **上流コミット**:
  - `51a401fa9` load makeobj 60_12 (#771)
- **主な変更箇所**:
  - `descriptor/reader/building_reader.cc`, `descriptor/building_desc.h` — 建物ノード v12 (旧 `btyp` を廃止し `height_clearance` を追加) の読み込み。
    v11 以前は `height_clearance = 255` (未設定)。取得用に `get_height_clearance()` を追加
  - `descriptor/roadsign_desc.h`, `descriptor/reader/roadsign_reader.cc` — 標識のカーソル以降の任意子ノード
    (斜め・前面・左側通行用とその組合せ、子 4〜10) を子ノード数 (`children`) で判別して参照する
    `get_image_id(index, diagonal, left)` / `get_front_image_id()` / `has_diagonal_image()` 等を追加
  - `descriptor/way_desc.h`, `descriptor/reader/way_reader.cc` — 前面画像リストの有無 (`front_image_lists`) と
    実際に前面画像があるか (`front_images`) を分離。斜め・近接斜め・分岐 (switch / switch_ex) 画像の有無を
    `register_obj()` で一度だけ計算してビットフィールドにキャッシュ。`get_close_diagonal_image_id()` を追加
  - `descriptor/way_obj_desc.h`, `descriptor/reader/way_obj_reader.cc` — 架線等の斜め・近接斜め画像の有無をキャッシュし、
    `get_front/back_close_diagonal_image_id()` を追加
  - `descriptor/image_list.h` — 画像リストが全て空かを返す `is_empty()` を追加
  - `boden/wege/weg.{h,cc}` — `close_diagonal_state` と `check_close_diagonal()` を追加。十字 (`ribi_t::all`) の way タイルで
    周囲4タイルの形から「平行な2本の斜め」かを判定し、近接斜め画像 (`image_close_diagonal`) を使う。
    `check_diagonal()` は近接斜め画像を持つ way なら両隣が十字/分岐でも斜めとみなす。回転時は状態の縦横を入れ替える
  - `obj/wayobj.{h,cc}` — 架線等も同様に近接斜め画像を選ぶ
- **詳細**:
  - 近接斜め: 1タイル間隔で平行に走る2本の斜め線路/道路で、1タイルおきに十字タイルになる箇所を、
    交差ではなく2本の斜めとして描画する。`close_diagonal_state` は 1=北西+南東、2=北東+南西。
    近接斜め画像が pak に無い way では従来どおりで、コード中のコメントでは「1タイルに2本の way」(FEAT-16) で表現する前提。
  - 標識の追加画像リストは本コミット時点では記述子のアクセサのみで、`obj/roadsign.cc` の描画からは未使用。
    `height_clearance` も読み込むだけで、建設判定等には未使用 (本家では後に FEAT-29 で建設判定に使われるようになった)。
  - 標識の子ノード数・建物の `height_clearance` はチェックサムに含めていない (既存 pak のチェックサムは不変)。
- **取り込み時の注意**:
  - セーブ形式への影響なし (`close_diagonal_state` は保存せず `calc_image()` で再計算)。
  - v58_3 の `building_reader_t` は未知の建物ノード版数で `dbg->fatal()` するため、makeobj 60.12 で作った建物 pak は
    本項目を取り込むまで develop-kasumi では読み込めない。
  - `weg_t::check_diagonal()` / `calc_image()` は FEAT-16 でも変更されており、FEAT-16 未取り込みのまま cherry-pick すると衝突が見込まれる。
- **develop-kasumi 側コミット**: —
- **備考**: 関連: FEAT-16 (1タイルに2本の way を斜めで共存させる)、FEAT-29 (`height_clearance` を建設判定に使う)

### FEAT-29: 建物の高さ (`height_clearance`) を使った建設判定

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 建物ごとの高さ (`height_clearance`、高さレベル単位) を、高架道・橋の下の建物チェックと、
  高架の下への建物配置チェックに使う。pak で指定されていない建物は画像から高さを算出する。
- **上流コミット**:
  - `3c4aff9e9` add building height clearance (#696)
- **主な変更箇所**:
  - `descriptor/building_desc.{h,cc}` — `building_tile_desc_t::get_drawn_height()` (タイル画像が地面線より上に何レベル出ているか) と
    `building_desc_t::calc_height_clearance()` (全レイアウト・全タイルの最大値を建物全体の値にする) を追加。
    pak で `height_clearance` が指定されている (255 以外) 場合は上書きしない。
    `calc_checksum()` に `height_clearance` を追加
  - `descriptor/reader/building_reader.cc` — 建物ノード v11 以前は `height_clearance = 255` (未設定) にする
  - `bauer/hausbauer.cc` — `successfully_loaded()` で全建物の `calc_height_clearance()` を呼ぶ
  - `bauer/wegbauer.cc` — `is_allowed_step()` の「高架の下の市街建築物は高すぎると不可」の判定を、
    「2段目の画像があるか」(`get_background(0,1,0)!=IMG_EMPTY`) から `get_height_clearance() > get_way_height_offset(to)` に変更
  - `bauer/brueckenbauer.cc` — `is_blocked()` で、橋の下の地表の建物が橋桁までの隙間より高ければ
    `Bridge blocked by building below.` で建設を拒否
  - `simworld.{h,cc}` — `karte_t::square_is_free()` に `const building_desc_t *desc = NULL` 引数を追加し、
    建物の高さの範囲 (z+1〜z+`height_clearance`) に高架・橋の地面があれば配置不可にする
  - `simtool.cc` — 駅舎・建物・工場 (産業チェーン/単独)・本社の建設で `square_is_free()` に `desc` を渡す
- **詳細**:
  - 画像からの算出は、基本画像の最上段の行と地面行の画像の上端から「地面線より上に何ピクセル出ているか」を求め、
    `tile_raster_scale_y(TILE_HEIGHT_STEP, cell)` で高さレベルに換算する (ズームによらず一定)。季節画像の最大値を採る。
  - 高さは建物全体で1つ。1段だけのタイルでも同じ建物に高いタイルがあれば、その高さで判定する。
- **取り込み時の注意**:
  - **FEAT-28 が前提** (`height_clearance` メンバ・`get_height_clearance()`・建物ノード v12 の読み込み)。
  - `calc_checksum()` に `height_clearance` が加わるため、**全建物の記述子チェックサムが変わり pakset のチェックサムが変わる**。
    ネットワークゲームではサーバーとクライアントの両方を同時に更新する必要がある。
  - 高架の下の市街建築物の判定基準が変わるため、既存マップで以前は建てられた/建てられなかった高架が逆になりうる。
  - セーブ形式への影響なし。表示文字列 `Bridge blocked by building below.` は `ja.OTRP.tab` に未登録 (2026-10-27 時点)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `descriptor/building_desc.h` の `calc_height_clearance`。
  コミット本文の履歴にある「BUG FIX: load clip_below settings of pakfiles」は FIX-02 のもので、本コミットの差分には含まれない。

### FEAT-30: 着陸待ちの航空機が空いている別の滑走路を選ぶ (choose runway)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 着陸予定の滑走路が塞がっていて旋回待ちになった航空機が、目的の空港につながる別の空いている滑走路を探し、
  そちらへ着陸経路を組み直す。
- **上流コミット**:
  - `bff207012` choose runway (#757)
- **主な変更箇所**:
  - `vehicle/simvehicle.{h,cc}` — `air_vehicle_t` に `is_landing_runway_end(gr, approach)`、
    `append_landing_route(route, end_pos, touchdown_idx, search_idx)`、`find_runway()` を追加。
    `is_target()` の滑走路端判定と `calc_route()` の着陸経路 (直線進入・旋回・着陸滑走) 生成をそれぞれ切り出したもの
  - `vehicle/simvehicle.cc` — `can_enter_tile()` で旋回の終わり (`route_index == touchdown - landing_distance`) に
    滑走路の予約に失敗したとき、`find_runway()` を試し、成功すればそのまま進む (失敗すれば従来どおりもう1周旋回)
- **詳細**:
  - 候補は、目的の停留所から `air_wt` の way をたどって `max_choose_route_steps` 以内にある、進入方向に合う滑走路端
    (現在の着陸予定の端を除く)。滑走路全体が他の編成に予約されておらず、その端から目的地まで地上走行の経路があるものから、
    「現在位置からの距離 + 地上走行経路の長さ」が最小のものを選ぶ。
  - 選んだら旧滑走路の予約を解放して新滑走路を予約し、現在位置以降の経路を差し替える。見つからなければ従来どおり旋回を続ける。
  - 経路探索は再入不可のため、編成が待機状態 (`cnv->is_waiting()`) でなければ探索せず一旦 false を返して停止させ、
    待機状態になって step から再び呼ばれたときに探索する。
- **取り込み時の注意**: セーブ形式への影響なし。航空機の着陸処理を大きく書き換えるため、
  develop-kasumi で航空機まわりを改造している場合は競合に注意 (FIX-06 / FIX-24 も同じ領域)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `vehicle/simvehicle.h` の `find_runway`。FEAT-31 はこのブランチから派生している。

### FEAT-31: 誘導路・滑走路 (`air_wt`) で私有道路標識を有効にする

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 航空機の地上走行で私有道路標識 (`is_private_way()`) を判定し、通行を許可されていない会社の航空機を通さない。
- **上流コミット**:
  - `93d04522d` add private roadsign for air_wt (#788)
- **主な変更箇所**:
  - `vehicle/simvehicle.cc` — `air_vehicle_t::check_next_tile()` の地上走行状態 (`taxiing` / `taxiing_to_halt` / `looking_for_parking`) で、
    way に標識があり、それが `air_wt` の私有標識で、自社のビットが `get_player_mask()` に無ければ false を返す
- **詳細**: 他の waytype (`rail_vehicle_t` 等) の私有標識チェックと同じ形。way が無い場合は先に false を返す。
- **取り込み時の注意**:
  - 本家版は FEAT-16 (`891191546`) で追加された `check_next_tile(bd, ..., prev)` オーバーロード内の変更で、
    会社マスクは FEAT-01 の64社対応 (`(uint64)1 << get_owner_nr()`)。develop-kasumi (16社・旧シグネチャ) に入れる場合は
    `air_vehicle_t::check_next_tile(bd, need_electric)` へ手作業で移植し、シフトも既存コードの `1<<get_owner_nr()` に合わせる。
  - セーブ形式への影響なし。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `air_vehicle_t::check_next_tile()` 内の `is_private_way()`。
  コミット本文の「choose runway」は FEAT-30 のブランチ履歴。

### FEAT-32: 表示設定ダイアログにキー設定タブを追加

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 表示設定ダイアログに「キー設定」タブを追加し、各ツールのショートカットキーをゲーム内で変更・解除・初期化できるようにする。
  変更は `menuconf.tab` に直接書き戻す。
- **上流コミット**:
  - `6b520a4a8` Add in-game keybindings settings synchronized with menuconf.tab (#766) — 本体
  - `b39f88729` BUG FIX: crash when open display settings (#784) — 後続修正
- **主な変更箇所**:
  - `gui/display_settings.{h,cc}` — `shortcut_settings_t` (一覧のスクロール、各行の「キー変更」「割り当て解除」、「初期設定に戻す」) を追加し、
    `color_gui_t` のタブに組み込む。キー入力の取得 (`capture_event()`) のため `color_gui_t::infowin_event()` を追加
  - `simmenu.{h,cc}` — `tool_t` に `get_shortcut_count()` / `get_shortcut_tool()` / `get_shortcut_name()` /
    `get_shortcut_key_name()` / `is_shortcut_key_allowed()` / `find_shortcut_conflict()` / `set_shortcut()` /
    `reset_shortcuts()` / `reload_shortcuts()` を追加。`menuconf.tab` のキー欄だけを書き換えて保存する
  - `simutrans/text/ja.tab` — 「キー設定」「キー変更」「割り当て解除」等 17 語 (`ja.OTRP.tab` ではなく基本翻訳ファイル)
  - `tests/keyboard-settings.md`, `tests/test_keybindings_unicode.py` (新規) — 手動試験手順と、ファイル処理・キー解析の単体テスト
  - **以下は `b39f88729` の分**:
  - `simmenu.cc` — `shortcut_requires_parameter()` に `TOOL_REMOVE_WAYOBJ` / `TOOL_CHANGE_GAME_SPEED` / `TOOL_TRAFFIC_LEVEL` / `TOOL_MOVE_MAP` を追加
    (パラメータ必須のツールはキー設定の一覧に出さない)
  - `simtool.{h,cc}` — `default_param` が NULL / 記述子が見つからない場合の `get_tooltip()` の NULL 参照を回避
    (トンネル・wayobj 撤去・車庫・ゲーム速度変更)。`tool_reset_game_speed_t` / `tool_fix_game_speed_t` の
    `get_tooltip()` で `return` が抜けていた (戻り値が不定) のを修正
- **詳細**:
  - 初回にキー設定タブを開いたとき、現在の `menuconf.tab` を `menuconf.default.tab` としてバックアップし、「初期設定に戻す」はそこからキーを復元する。
  - 書き込みは一時ファイル経由で置き換え、コメント・改行コード・アイコン等のキー以外の欄は保持する。
    外部で `menuconf.tab` を編集した場合はタブを切り替え直すと再読込される。
  - 他の操作に割り当て済みのキーは警告ダイアログを出して変更しない。ゲーム固定操作のキーは割り当て不可。
  - `b39f88729` は、キー設定タブが全ツールのツールチップを `default_param` なしで取得するため、
    従来は呼ばれなかった経路で NULL 参照・不定値の返却が起き、表示設定を開くと落ちる問題の修正。
- **取り込み時の注意**:
  - セーブ形式への影響なし。設定は `menuconf.tab` (pakset 側の設定ファイル) に保存される。
  - `simmenu.cc` に約380行の追加。develop-kasumi 独自のメニュー/ツール追加がある場合は一覧の対象・除外条件を確認すること。
  - `b39f88729` のうち `tool_reset_game_speed_t` / `tool_fix_game_speed_t` の `return` 抜けは v58_3 時点から存在し、
    develop-kasumi にも同じコードがある。キー設定とは独立に `simtool.h` 部分だけ取り込むことも可能。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simmenu.h` の `get_shortcut_count`、または `gui/display_settings.h` の `shortcut_settings_t`。

### FEAT-33: サーバー接続ダイアログのアドレス入力欄の内容をローカルに保存する

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: サーバー一覧ダイアログで手入力した接続先アドレスを保存し、次回ダイアログを開いたときに入力欄へ復元する。
  復元しただけでは接続確認 (問い合わせ) を行わない。
- **上流コミット**:
  - `4c4b865ee` Persist connection textbox locally without querying restored addresses (#758)
- **主な変更箇所**:
  - `gui/server_frame.{h,cc}` — 入力欄のバッファを `env_t::newserver_name` から `server_frame_t` の
    メンバ `server_address` に分離。`load_server_address()` / `save_server_address()` とデストラクタを追加し、
    `env_t::pak_dir + "config/servers.tab"` の `# last_server = <アドレス>` 行に保存する (一時ファイル経由で置き換え)。
    問い合わせ・接続・pakset 比較はすべて `server_address` を使う
  - `utils/last_server_setting.h` (新規) — `# last_server` 行の解析 (`parse()` / `read()`) と書き換え (`update()`)
- **詳細**:
  - コマンドライン `-ip` (`env_t::newserver_name`) が指定されていればそちらを優先し、その場合だけ従来どおり自動で問い合わせる。
    ネットワークモード中 (`env_t::networkmode`) は自動問い合わせを行わない。
  - 入力欄の編集後にフォーカスが外れたとき (`WIN_UNTOP` で値 0 が来る) は問い合わせず、Enter または追加ボタンでのみ問い合わせる。
- **取り込み時の注意**:
  - セーブ形式への影響なし。保存先は pakset の `config/servers.tab`。
  - 普段 `-ip` 付きで起動する運用では復元は働かず、従来の自動問い合わせのままになる。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/server_frame.h` の `load_server_address`。

### FEAT-34: 信号情報ウィンドウで「信号を撤去」ボタンを区切り線の下・右寄せに配置

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 信号情報ウィンドウで、信号のフラグ設定ボタン群と「信号を撤去」ボタンの間に区切り線を入れ、撤去ボタンを右端に寄せる (誤操作防止)。
- **上流コミット**:
  - `f2b352a3f` add divider between signal flags buttons and remove button (#783)
- **主な変更箇所**:
  - `gui/signal_info.cc` — `bt_two_ways` の後に `gui_divider_t` を追加し、`bt_remove_signal` を `gui_fill_t` と並べた2列テーブルに入れる
- **詳細**: —
- **取り込み時の注意**: セーブ形式への影響なし。FEAT-20 (`bt_ignore_length`) も同じファイルを触るが、変更箇所は離れている。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/signal_info.cc` の `gui_divider_t`。
  同じファイルの FEAT-37 のボタンは、本項目の区切り線より前に置かれる (`4b4625c1a`。FEAT-37 参照)。

### FEAT-35: 産業の情報ウィンドウで基本生産量・出荷単位・自動拡張の禁止を設定する

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 産業の情報ウィンドウの「詳細」タブに、基本生産量と出荷単位の設定 (それぞれ設定ボタンで確定) と、
  自動拡張の禁止を追加する。併せて大きな生産量での桁あふれを修正する。
- **上流コミット**:
  - `dc6e88bbb` Add explicit controls for existing factory production and shipment size (#768)
- **主な変更箇所**:
  - `gui/fabrik_info.{h,cc}` — `bt_no_expand_factory` / `numinp_production` / `bt_set_production` / `bt_set_shipment_size` を追加し、
    「この産業を閉鎖しない」と出荷単位を「詳細」タブへ移す。数値は入力しただけでは反映しない
  - `simtool.cc` — `tool_change_factory_t` に `e` (自動拡張の禁止) と `p` (月あたりの基本生産量) を追加し、引数の検査を強化。
    産業を建てるツールの初期生産量を `MAX_BASE_PRODUCTION` で制限
  - `simfab.{h,cc}` — `no_expand_factory`、`MAX_BASE_PRODUCTION` (131072)、`can_auto_expand()`、`get_min_base_production()`、
    `get_min_editable_base_production()`、`get_min_editable_monthly_base_production()`、`set_edited_base_production()`、
    `get_fields_by_removal_order()` を追加。`rdwr()` は **OTRP v63 以上**で `no_expand_factory` を読み書きする。
    生産・消費・統計の計算を桁あふれしない形に直す
  - `utils/saturated_math.h` (新規) — `saturate()` / `saturated_add()` / `saturated_mul_div()`
  - `bauer/fabrikbauer.cc`, `gui/factory_edit.cc`, `gui/factorylist_stats_t.cc`, `simworld.h` — 64ビット計算への修正
  - `documentation/ja.OTRP.tab` — 「この産業を自動拡張しない」「基本生産量:」「出荷単位を設定」等 6 語 (MISC-02 参照)
  - `tests/automated-tests/tests/test_factory.nut`, `tests/automated-tests/all_tests.nut`
- **詳細**:
  - 基本生産量を減らすと、畑を持つ産業では必要な数の畑を `get_fields_by_removal_order()` の順に撤去する。
    下限は残す畑の生産量から決まる。
  - 自動拡張の禁止は畑の追加と自動の生産量増加を止めるだけで、既存の畑と生産量はそのまま残る。
  - 操作の権限は `player_t::check_owner()` で確認する。自動拡張の禁止のみ、さらに所有者本人に限る。
- **取り込み時の注意**:
  - **セーブ形式に影響する** (`fabrik_t::rdwr()` が OTRP v63 でゲート。MISC-01 参照)。
    `OTRP_VERSION_MAJOR` を上げないと `no_expand_factory` が保存されず、再同期を含むロードで設定が消える。基本生産量は従来の項目で保存される。
  - `git apply --check` で当たらないのは `simfab.cc` / `simfab.h` / `simtool.cc` の各1か所以降 (2026-11-04 時点)。
    `gui/fabrik_info.cc` は `develop-kasumi` で変更しておらず、そのまま当たる。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simfab.h` の `no_expand_factory`。

### FEAT-36: Simutrans standard 125.0.1 までのセーブデータを読み込む

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: Simutrans standard の新しいセーブデータ (124.2 以降の形式の違いを含む) を OTRP で読み込めるようにする。
- **上流コミット**:
  - `c429c9192` update rdwr to read 125.0.1 (#770)
- **主な変更箇所**:
  - `simversion.h` — `SIM_LOADABLE_MAJOR` / `SIM_LOADABLE_MINOR` を 124.4 から 125.6 へ
  - `dataobj/scenario.{h,cc}` — `rdwr_standard_forbidden_tools()` を追加 (standard 124.3 以降の、会社ごとの禁止ツール一覧)。
    OTRP v59 未満のセーブの「全社」の番号 15 を `PLAYER_UNOWNED` へ読み替える
  - `obj/roadsign.cc` — standard 124.2 以降は私有道路マスクの下位バイトが `ticks_offset` にある
  - `simworld.cc` — standard 124.3 以降のセーブはウィンドウの状態を復元しない (形式が異なるため。ファイル末尾なので読み飛ばして問題ない)
- **詳細**: standard のセーブかどうかは `get_OTRP_version() == 0` で判定する。
  許可ルールと、ツールの引数を指定した禁止ルールは対応しておらず、警告を出して読み飛ばす。
- **取り込み時の注意**:
  - OTRP のセーブ形式は変わらず、バージョン引き上げは不要。
  - `develop-kasumi` には `private_way_mask` (FEAT-01) が無いため、`obj/roadsign.cc` の変更は `ticks_ns` / `ticks_ow` を使う現在の形に書き直す必要がある。
  - `dataobj/scenario.cc` の「番号 15 の読み替え」は FEAT-01 のためのもので、`PLAYER_UNOWNED` が 15 の `develop-kasumi` では何も変わらない。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/scenario.h` の `rdwr_standard_forbidden_tools`。

### FEAT-37: choose 標識で同じ停留所に複数の編成を進入させる

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: choose 標識に「同じホームに複数編成を停車させる」設定を追加する。先行編成がいるホームも行き先に選び、その後ろに停車できる。
- **上流コミット**:
  - `a7faad137` Add allow multi convoys at same platform (#767) — 本体
  - `4b4625c1a` fix buttons position (#795) — 後続修正 (信号情報ウィンドウのボタンを区切り線の前へ移す)
- **主な変更箇所**:
  - `obj/roadsign.h` — `choose_sign_flag` に `allow_multi_convoys` (1<<11) を追加し、`is_allow_multi_convoys()` / `set_allow_multi_convoys()` を新設
  - `gui/signal_info.{h,cc}`, `gui/onewaysign_info.{h,cc}` — `bt_allow_multi_convoys` を追加
  - `simtool.cc` — `tool_change_roadsign_t` に `q` を追加
  - `dataobj/route.{h,cc}`, `ifc/simtestdriver.h` — `find_route()` と `is_target()` に `allow_multi_convoys` 引数を追加
  - `vehicle/simvehicle.{h,cc}` — `rail_vehicle_t::is_target()` は、次のタイルを予約できなければ、まだ先へ進める場合でもそこを目的地とする。
    `road_vehicle_t::is_target()` は、追越禁止で空き車線が無いと進入しない判定を外す。`choose_route()` に引数を追加
- **詳細**: 従来の軌道系の判定はホームの終端まで進めることを条件にしていたため、先行編成がいるホームは選ばれなかった。
  フラグを立てると、先行編成の手前で止まる形で選べるようになる。
- **取り込み時の注意**:
  - `choose_sign_flag` は `rdwr_short()` で保存されるためセーブ形式の変更は無い。
  - FEAT-20 (`ignore_length` 引数) と FEAT-15 (`guide_route()`、`bt_guide_signal`) の上に書かれており、`develop-kasumi` では
    11 ファイルで当たらない (2026-11-04 時点)。手作業での移植になる。
  - `4b4625c1a` は FEAT-34 の区切り線の位置を前提にしている。
  - 表示文字列 `Stop more than one convoys.` / `Allow more than one convoys at same spot(platform).` は `ja.OTRP.tab` に未登録 (2026-11-04 時点)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `obj/roadsign.h` の `is_allow_multi_convoys`。関連: FEAT-20。コミット本文の前半は FEAT-20 のブランチ履歴。

### FEAT-38: 駅の集客範囲を waytype ごと・海タイル用に設定する

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: これまで1つだった集客範囲 (`station_coverage`) を、駅の建物の waytype ごとと、海タイル用に別々に設定できるようにする。
- **上流コミット**:
  - `4c1dc9d74` can change station coverage by waytype (#791)
- **主な変更箇所**:
  - `dataobj/settings.{h,cc}` — `station_coverage_size_wt[]`、`station_coverage_size_water`、`get_station_coverage(waytype_t)`、
    `get_station_coverage_water()`、`get_max_station_coverage()` を追加。`simuconf.tab` のキーは
    `station_coverage[road]` 等と `station_coverage_water`。`rdwr()` は **OTRP v63 以上**で読み書きする
  - `gui/settings_stats.cc`, `gui/settings_frame.cc` — 設定項目を追加し、変更時に `haltestelle_t::recalc_all_coverages()` を呼ぶ
  - `simhalt.{h,cc}` — `tile_t::coverage`、`get_tile_coverage()`、`get_tile_reach()`、`is_tile_covering()`、`is_covering()`、
    `covers_any_tile_of()`、`add_tile_coverage()`、`refresh_coverage()`、`recalc_all_coverages()` を追加し、
    `add_grund()` / `rem_grund()` / `reconnect_factories()` / `rebuild_connections()` / `rebuild_linked_connections()` を書き換える
  - `simplan.{h,cc}` — `update_haltlist()` / `clear_haltlist()` を追加。陸と海が入れ替わったタイルは範囲の一覧を作り直す
  - `simworld.cc` — `enlarge_map()` は `refresh_coverage()` を使う
  - `simtool.cc`, `player/ai.{h,cc}`, `player/ai_goods.cc`, `player/ai_passenger.cc` — カーソルの範囲と AI の建設位置を waytype 別の範囲で求める
  - `script/api/api_settings.cc` — `get_station_coverage` のオーバーロードに合わせて登録を修正
- **詳細**:
  - タイルの範囲は建物の `get_finance_waytype()` で決まる。waytype の無い建物 (拡張建物など) は従来の `station_coverage` を使う。
  - 海タイル (川は含まない) に対しては、どの駅のタイルからでも `station_coverage_water` を使う。
  - `station_coverage` は全 waytype と海の既定値を兼ね、個別のキーがそれを上書きする。
  - 徒歩連絡は、範囲が waytype で異なる場合があるので、両方の駅が互いのタイルを範囲に含むことを条件にする。
- **取り込み時の注意**:
  - **セーブ形式に影響する** (`settings_t::rdwr()` が OTRP v63 でゲート。MISC-01 参照)。
    `OTRP_VERSION_MAJOR` を上げないと個別の設定が保存されず、ロードすると単一の値へ戻る。
  - `develop-kasumi` 独自の `network/live_info.cc:278` が単一の `get_station_coverage()` を使っており、Web ビューアへ渡す範囲の見直しが必要。
  - `dataobj/settings.cc` と `gui/settings_frame.cc` の差分は FEAT-18 と FEAT-07 の文脈の上で書かれている。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simhalt.h` の `is_tile_covering`、または `dataobj/settings.h` の `get_max_station_coverage`。

### FEAT-39: 需要がなければ区間ごと飛ばす停車設定、連結待ちの判定更新、貨物の単一目的地積載

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: v58_3 からある「利用者がいなければこの停留所へ行かない」(1停留所単位) を、開始と終了で囲む区間単位の判定に作り直す。
  連結の待ち合わせも実際に停車する停留所で比較するようにし、スケジュールの設定「貨物の単一目的地積載」を追加する。
- **上流コミット**:
  - `86cf813ef` no_go_no_users update, wait_for_coupling update, one destination loading (#741)
- **主な変更箇所**:
  - `dataobj/schedule_entry.h` — `NO_GO_NO_USERS_SECTION_START` (1<<24) / `NO_GO_NO_USERS_SECTION_END` (1<<25) を追加。
    `NO_GO_NO_USERS` (1<<15) は旧セーブの解読専用になり、`is_no_go_no_users()` / `set_no_go_no_users()` は削除
  - `dataobj/schedule.{h,cc}` — `rdwr()` で OTRP v63 未満のセーブの旧フラグを1停留所の区間へ移行。
    `get_no_go_no_users_section()` と、スケジュールのフラグ `SINGLE_FREIGHT_DESTINATION` (1<<6) を追加。表示文字は `s` / `e` / `W` (開始と終了の両方は従来の `G`)
  - `simconvoi.{h,cc}` — 区間の判定 (`section_decisions`、`would_stop_at_entry()`、`decide_section()`、`lock_upcoming_section_decision()`、
    `forget_left_section_decisions()`、`section_has_demand()`、`has_own_demand_at_entry()`、`has_demand_at_halt_before_section()`)、
    連結列の判定 (`chain_wants_stop_at_offset()`、`chain_would_stop_at_entry()`、`get_next_real_stop_entry()`、`get_real_stop_entry_after_offset()`)、
    積載先の計算 (`calc_loading_destination_halts()`、`get_committed_destination()`) を追加。
    `calc_reachable_halts()` に起点エントリの引数を追加し、`can_continue_coupling()` / `can_start_coupling()` を実際の停車先で比較する形へ書き換える
  - `simhalt.{h,cc}` — `has_goods_for_zwischenziel()` を追加
  - `vehicle/simvehicle.cc` — `vehicle_t::hop()` の判定を `chain_wants_stop_at_offset()` に置き換える
  - `gui/schedule_gui.{h,cc}` — `bt_no_go_no_users` を `bt_no_go_no_users_section_start` / `bt_no_go_no_users_section_end` に置き換え、`bt_single_freight_destination` を追加
  - `tests/automated-tests/tests/test_no_go_section.nut` (新規, 18 件), `tests/automated-tests/all_tests.nut`
- **詳細**:
  - 区間は START のエントリから次の END まで (END より先に次の START があればその手前まで)。1つのエントリに両方を付けると1停留所の区間になる。
    区間内のどこにも需要が無ければ、区間全体を飛ばす。
  - 判定は区間の手前の停留所を出るときに確定し、次に区間の外の停留所へ停車するまで保持する。
    需要の無い区間の停留所は `calc_reachable_halts()` で到達先から外れ、そこ行きの貨物は積まれない。
  - 需要とは、積載中の貨物の降車先、そのエントリで積める待機貨物、区間の手前の停留所で積込中の区間行き貨物。
    従来の `fetch_loadable_fresh_goods()` (新着のみ。新着の印を消す) をやめ、読み取りだけの `has_goods_for_zwischenziel()` で全待機貨物を見る。
  - 連結列は、全編成が飛ばしたい場合のみ飛ばす。連結の待ち合わせはこの判定を踏まえた次の停車先で比較する。
  - 貨物の単一目的地積載は旅客・郵便には効かない。行き先は積載中の貨物から求め、別に保存はしない。
- **取り込み時の注意**:
  - **バージョン引き上げが必要** (MISC-01 参照)。`schedule_t::rdwr()` の移行は旧フラグ (1<<15) を消さないため、OTRP v58 のままだとロードのたびに移行が走り、
    GUI で外した区間の設定が元に戻る。`OTRP_VERSION_MAJOR` を上げるか、移行時に旧フラグを消すこと。
  - `develop-kasumi` 独自の `dataobj/schedule_io.cc` の対応が必要。`stop_flag_names` に区間の2フラグ、`schedule_flag_names` に `SINGLE_FREIGHT_DESTINATION` を追加する。
    既存の `no_go_no_users` を読み込むと効かない旧フラグが立つだけになるので、区間の2フラグへ変換すること。
  - `has_own_demand_at_entry()` は FEAT-18 の `is_start_shipped()` / `is_carrying_convoys()` / `has_shipping_capacity()` を使っており、FEAT-18 未取り込みなら除く必要がある。
  - `section_decisions` は保存されない。再同期ではサーバーもクライアントもロードし直し、判定を呼ぶのは `simconvoi.cc` と `vehicle/simvehicle.cc` だけなので (GUI からは呼ばれない)、
    コード上はデシンクの要因にならないはず。ただしロードで確定済みの判定が消えて決め直しになる。実機では未確認。
  - `can_start_coupling()` の書き換えがあるので、連結まわりの回帰確認が必要。
  - 表示文字列 `no-go-no-users section start` / `no-go-no-users section end` / `Single freight destination` と各ツールチップは
    `ja.OTRP.tab` に未登録 (2026-11-04 時点)。既存の `not go if no users` の訳は使われなくなる。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/schedule_entry.h` の `is_no_go_no_users_section_start`、または `dataobj/schedule.h` の `is_single_freight_destination`。

### FIX-22: 連結編成の発車時刻枠 (departure slot) の予約・解放の不整合

- **分類**: バグ修正
- **状態**: 一部取り込み
- **概要**: 発車時刻指定 (発車枠) と編成の連結が組み合わさった場合に、子編成が枠を取ってしまう、
  親子で別々に枠を取る、発車しなかった枠が返却されない、短いホームで枠の持ち主が駅にいないと判定されて枠が消える、等の不具合を修正する。
- **上流コミット**:
  - `f9ad42b26` fix departure time slot (#754)
- **主な変更箇所**:
  - `simconvoi.{h,cc}` — `release_departure_slot(halthandle_t halt = halthandle_t())` を追加。
    停留所は、積込中ならスケジュールの現在エントリから、そうでなければ全車両の位置から探す (先頭車両がホーム外にいる場合に対応)。
    `step()` のスケジュール変更時、`next_stop_button_pressed()`、`board_carrier()` (FEAT-18) で枠を返却する。
    `is_staying_at(halt)` を追加 (最上位の親編成の状態とスケジュールで「その停留所に停車中か」を判定)
  - `simconvoi.cc` — `can_depart()` は最上位の親編成以外では判定しない。
    `hat_gehalten()` で連結子編成は発車判定をせず (`!is_coupled()`)、親は子が既に持っている枠 (holder) があればそれに乗る。
    親が発車時刻指定で、かつ子と枠グループが異なる場合のみ親が自分の枠を取り、取れた時点で子の枠を返却する
  - `simhalt.{h,cc}` — `is_same_departure_slot_group(a, b)` (静的) と `is_departure_booked_by(dep_tick, cnv)` を追加。
    `book_departure()` の「駅にいない編成の枠は無効」の判定を `is_staying_at()` に変更し、
    同じ連結列・同じ枠グループの編成が持つ枠を自分の枠とみなす。`erase_departure()` も同様に連結列単位で消す
- **詳細**:
  - 連結列は常に1つの枠だけを持つようにする設計。子が親になる前 (連結前) に取った枠は、連結後も列全体の枠として維持する。
  - 通常の発車では枠は期限まで残す (遅れた編成が同じ枠を取り直さないため)。発車せずに停留所を離れる場合のみ返却する。
  - `next_stop_button_pressed()` では FEAT-02 の `waiting_for_departure_make_another_convoy_depart` もリセットする。
- **取り込み時の注意**:
  - 発車枠の機能自体は v58_3 時点から存在するが、本コミットは FEAT-02 (`waiting_for_departure_make_another_convoy_depart`) と
    FEAT-18 (`board_carrier()`、FEAT-18 の `cd625b481` で切り出された `check_departure_conditions()` の直後の文脈) を前提にした差分を含む。
    これらが未取り込みの develop-kasumi へは該当部分を除いて手作業で移植することになる。
  - **develop-kasumi には同じ問題に対する独自修正がある**: `bfa2be8b7`「子編成が発車枠を取ってしまう問題の修正」は
    `hat_gehalten()` の判定を `!is_coupled()` に限定し、`couple_convoi()` で子の枠を返却する
    (`release_departure_slot()` を引数なしで独自に追加)。本家は逆に「子が取った枠を列の枠として残す」方式で、
    方針が異なり、同名関数のシグネチャも異なる。取り込む場合はどちらの方式にするかを決めてから統合すること。
    また `a750cd86a`「ticksリセット時に発車時刻指定が乱れる問題の修正」も `simhalt.cc` の発車枠処理を触っている。
    → 2026-10-31 に**本家の方式へ統一**して取り込んだ (下記「取り込み内容」)。
  - セーブ形式への影響なし。
- **取り込み内容 (2026-10-31)**:
  - `simhalt.{h,cc}` の変更は全部。`a750cd86a` との衝突はない (`is_departure_booked()` の `i->cnv.is_bound()` は develop-kasumi に既にあった)。
  - `simconvoi.{h,cc}` は、`release_departure_slot(halthandle_t)` / `is_staying_at()` の追加、`step()` のスケジュール変更時の返却、
    `can_depart()` の親以外の早期 return、`hat_gehalten()` の holder 処理、`next_stop_button_pressed()` の `loading_halt` と枠の返却。
  - `bfa2be8b7` の独自修正は本家版で置き換えた。`hat_gehalten()` の `!is_coupled()` 判定は本家にも同じものがあるので残り、
    `couple_convoi()` で子の枠を返却する処理は削除した (本家は子の枠を列の枠として残すため)。
    独自版の `release_departure_slot()` (引数なし) は本家版 (引数省略可) に置き換わっている。
- **未取り込み分 (FEAT-02 / FEAT-18 依存)**: 該当機能が develop-kasumi に無いため除外した。
  それぞれの項目を取り込むときに、以下を合わせて適用すること (FEAT-02 / FEAT-18 の「取り込み時の注意」からもここを参照している)。
  - **FEAT-02 と一緒に取り込む分**: `convoi_t::next_stop_button_pressed()` の `if(  c->is_loading()  )` 内に
    `c->set_waiting_for_departure_make_another_convoy_depart(false);` を追加する。
    develop-kasumi では同じ場所に `set_waiting_for_departure_allowance_by_other_convoy(false)` と合わせて
    「本家 FEAT-02 (発車待ち合わせ) 未取り込みのため省略」という `NOTE` コメントを残してあるので、それを目印に両方を戻す。
  - **FEAT-18 と一緒に取り込む分**: `convoi_t::board_carrier()` の連結列ループ (`while(  k.is_bound()  )`) の先頭で、
    `state = SHIPPED` にする前に以下を行う (本家 `f9ad42b26` の `board_carrier()` の hunk そのもの)。
    ```cpp
    // the per-stop flags and the departure slot are already cleared above
    k->push_goods_waiting_time_if_needed();
    k->push_convoy_stopping_time();
    // the convoy is still on the map, so the halt is found from its position.
    k->release_departure_slot();
    k->unset_convoi_coupling_in_progress();
    k->loading_limit = 0;
    k->loading_waiting_time = 0;
    ```
    (`loading_limit` / `loading_waiting_time` の代入は `state = SHIPPED` の後ろから前へ移動したもの。)
    同じループの `k->waiting_for_departure_allowance_by_other_convoy = false;` の後ろに
    `k->waiting_for_departure_make_another_convoy_depart = false;` も追加されているが、これは FEAT-02 も必要。
    なお `next_stop_button_pressed()` 冒頭の `|| self->is_shipped()` は FIX-22 ではなく FEAT-18 (`cd625b481`) の変更。
- **develop-kasumi 側コミット**: `d6fa870c1` (FEAT-02 / FEAT-18 依存部分を除く)
- **備考**: 状態確認用の識別子は `simhalt.h` の `is_same_departure_slot_group`、または `simconvoi.h` の `is_staying_at`
  (`release_departure_slot` は develop-kasumi の独自版と同名のため識別子には使えない)。
  識別子があっても上記「未取り込み分」が残っているため、FEAT-02 と FEAT-18 の両方を取り込んで
  未取り込み分を適用するまでは `一部取り込み` のままとする。
  - 動作確認 (2026-10-31): 自動テストマップで、`1Diesellokomotive` 単機を3編成使った一時テストを実施。
    (a) 親 (`flags=1|16`, spacing 4) に子 (`flags=2`) が連結 → 連結列は親の枠どおりに発車する。
    (b) 戻ってきた連結列の先頭側に3編成目が連結して列が反転 (旧子が親、枠を持つ旧親が子) → 新しい親は
    発車時刻指定を持たないが、旧親の枠 (holder) に乗ってその時刻に発車する (本家方式で変わる挙動)。
    既存テスト `test_schedule_entry_spacing` ほか関連5件も通過。

### MISC-01: 本家バージョン番号のインクリメント

- **分類**: その他
- **状態**: 取り込み不要
- **概要**: 本家のリリースに伴う `simversion.h` のバージョン番号更新のみのコミット群。
- **上流コミット**:
  - `5617ce5a3` increment OTRP_VERSION_MAJOR (v59)
  - `44bb3b5e3` increment OTRP_VERSION_PATCH (v59_0_1)
  - `133e02261` increment OTRP_VERSION_PATCH (v59_0_2)
  - `ad3e3b553` increment OTRP_VERSION_MAJOR (v60)
  - `04c0219d0` increment OTRP_VERSION_PATCH (v60_0_1)
  - `f3aebc3a7` increment OTRP_VERSION_MINOR (v60_1)
  - `8ed0304d3` increment OTRP_VERSION_PATCH (v60_1_1)
  - `93192762a` increment OTRP_VERSION_MINOR (v60_2)
  - `6cef21d81` increment OTRP_VERSION_MAJOR (v61)
  - `867f82d5b` increment OTRP_VERSION_PATCH (v61_0_1)
  - `a20bca417` increment OTRP_VERSION_PATCH (v61_0_2)
  - `39993c64d` increment OTRP_VERSION_PATCH (v61_0_3)
  - `f30c237c5` increment OTRP_VERSION_PATCH (v61_0_4)
  - `559d52c2a` increment OTRP_VERSION_MAJOR (v62)
  - `252e90dfa` increment OTRP_VERSION_PATCH (v62_0_1)
  - `aa09b864f` increment OTRP_VERSION_PATCH (v62_0_2)
  - `5e2e05510` increment OTRP_VERSION_PATCH (v62_0_3)
  - `e495a7ba5` increment OTRP_VERSION_MINOR (v62_1)
  - `e2bdac807` increment OTRP_VERSION_MINOR (v62_2)
  - `11a7c338b` increment OTRP_VERSION_MINOR (v62_3)
  - `002e51b32` increment OTRP_VERSION_MINOR (v62_4)
  - `0be2ebc5e` increment OTRP_VERSION_PATCH (v62_4_1)
  - `7f70dd2e5` increment OTRP_VERSION_PATCH (v62_4_2)
  - `f491c6adf` increment OTRP_VERSION_MINOR (v62_5)
  - `882557913` increment OTRP_VERSION_PATCH (v62_5_1)
  - `67f6bdb6d` increment OTRP_VERSION_PATCH (v62_5_2)
  - `ae3a243bc` increment OTRP_VERSION_MAJOR (v63)
  - `5b254a9c2` increment OTRP_VERSION_PATCH (v63_0_1)
- **主な変更箇所**: `simversion.h` — `OTRP_VERSION_MAJOR` / `OTRP_VERSION_MINOR` / `OTRP_VERSION_PATCH`
- **詳細**: —
- **取り込み時の注意**:
  `develop-kasumi` は `OTRP_VERSION_MAJOR 58` / `MINOR 3` に独自の `AHOZURA_VERSION` を組み合わせて運用しているため、
  本家のバージョン番号はそのまま取り込まない。ただし **セーブ形式が `get_OTRP_version()` で分岐する機能を
  取り込む場合は、`OTRP_VERSION_MAJOR` の引き上げ判断が必須**:
  - `OTRP_VERSION_MAJOR >= 59` を要求する項目: FEAT-01 / FEAT-04 / FEAT-05
  - `OTRP_VERSION_MAJOR >= 60` を要求する項目: FEAT-07
  - `OTRP_VERSION_MAJOR >= 61` を要求する項目: FEAT-17 / FEAT-18 / FEAT-19 / MISC-04
  - `OTRP_VERSION_MAJOR >= 62` を要求する項目: FEAT-18 (`8076cc1f8` の航送収入・通行料設定の分)
  - `OTRP_VERSION_MAJOR >= 63` を要求する項目: FEAT-16 (`5367977aa` の分) / FEAT-18 (`966b49132` の分) / FEAT-35 / FEAT-38 /
    FEAT-39 (旧フラグの移行が v63 未満で毎回走る)
  - FIX-14 の `6af88343f` は `get_OTRP_version()<63` で分岐するが、v58 のままでも再集計が毎回走るだけで動く
  - 上記以外の項目 (FEAT-09 拡張分、FEAT-11〜FEAT-16、FEAT-20〜FEAT-34、FEAT-36 / FEAT-37、FIX-03〜FIX-25、
    MISC-03 / MISC-05) は `get_OTRP_version()` によるセーブ形式の分岐を持たないため、バージョン引き上げは不要。
    ただし FEAT-29 は pakset のチェックサムを変える (ネットワーク互換性に影響する) ので別途注意。
- **develop-kasumi 側コミット**: — (独自運用: `cf52a5750` 等)
- **備考**: バージョン番号自体は取り込まない方針。上記の依存関係のみ管理する。

### MISC-02: 日本語訳 `ja.OTRP.tab` の更新

- **分類**: その他
- **状態**: 未取り込み
- **概要**: v59 / v60 / v61 / v62 (v62_1 / v62_2 を含む) / v63 で追加された機能に対応する日本語訳の追加。
- **上流コミット**:
  - `8da4abc3e` update ja.OTRP.tab for v59
  - `d65dc2063` add ja.OTRP.tab for v60
  - `68348e37e` update ja.OTRP.tab for v61
  - `68d996bca` update ja.OTRP.tab for dummy-goods name
  - `dfbd77ae8` update ja.OTRP.tab for v62
  - (`d272db352` にも v59 訳文の修正が含まれる。FEAT-01 参照)
  - (`dc6e88bbb` にも v63 分の訳が含まれる。FEAT-35 参照)
- **主な変更箇所**: `documentation/ja.OTRP.tab`
- **詳細**:
  - v59分: 「常に最長の待機時間を使用する．」「発車許可待ち」「他編成の到着を待機」(FEAT-02)、
    `penalty_wait_for_two_month` (FEAT-04)、`base_revenue_from_halt` / 「収入」(FEAT-05)、
    16社以上の保存警告 (FEAT-01)、「キャッシュ中のルートを表示」(FEAT-09 関連)
  - v60分: 「他編成を発車させるまで待機」等 (FEAT-02)、`tile_length` / 「走行距離 (m)」(FEAT-07)
  - v61分 (`68348e37e`): 「航送中」「航送開始待ち」「航送終了予定駅」等の航送関連 (FEAT-18)、
    「無閉塞運行」「全駅に適用」等 (FEAT-19)
  - `68d996bca`: 航送用ダミー貨物名 `SHIPPING_{ROAD,TRACK,MONORAIL,MAGLEV,NARROWGAUGE,WATER}` の訳
    (「自動車を航送」「鉄道車両を航送」等) を追加 (FEAT-18 / MISC-05 関連)
  - v62分の大半は機能コミットに同梱されている:
    `a9c90e534` の「(%s) は私有化されました．」(FEAT-21)、
    `859a2244d` の「マップ画像を出力」等 (FEAT-24)、
    `b78c3685c` の「一編成専用区間」「追越車線は停車のみ可」(FEAT-23)
  - v62分 (`dfbd77ae8`): それまで本家でも未登録だった文字列の訳を追加。
    `ignore length` →「短いホームに進入可」、`ignore length. Convoy can enter shorter stop` →
    「振分を利用して編成長より短いホームに進入できます」(FEAT-20)、
    `This tile has a second, disconnected way of the same type:` →「このタイルは2種類の線路等があります」
    (FEAT-16 の `58bb2b989`)。
  - v62_1 / v62_2 分は翻訳のみのコミットは無く、すべて機能コミットに同梱されている:
    `8bdf0e5b8` の「サムネイル表示」「大きく表示」(FEAT-25)、
    `a560206be` の「先頭へ」「選択したエントリが先頭となるようにスケジュールを並べ替えます。」(FEAT-26)、
    `42d5061e9` の「プリセットを読込」「プリセットを保存」「プリセットの上書き」等 16 語 (FEAT-27)、
    `43f0ce22e` の「同時に表示できないマップ表示オプションが指定されています:」(FEAT-24 拡張分)。
    なお `develop-kasumi` は FEAT-26 相当の独自機能用に `Set as first` 等の訳を独自に登録している (`84592de0b`)。
  - v60_1 〜 v61 の間は翻訳のみのコミットは無く、機能コミットに同梱されている:
    `5461ae2d2` の「発車許可を出す路線を指定」(FEAT-02)、
    `eaae6f8b2` の「通行方向の詳細設定をコピー/貼り付け」「通行可能な方向」等 (FEAT-13)
  - v63 分も翻訳のみのコミットは無い。`dc6e88bbb` の「この産業を自動拡張しない」「基本生産量:」「出荷単位を設定」等 6 語 (FEAT-35) のみ登録されており、
    FEAT-18 の `revenue_*`、FEAT-37、FEAT-38 の `station_coverage[...]`、FEAT-39 の表示文字列は未登録 (2026-11-04 時点)。
- **取り込み時の注意**: `develop-kasumi` は独自に `ja.OTRP.tab` を編集している (`dd440ea4b`, `fa91f31fc` 等) ため
  ファイル単位の cherry-pick は競合しやすい。**対応する機能項目を取り込む際に、必要な行だけを追加する**運用とする。
- **develop-kasumi 側コミット**: —
- **備考**: FEAT-24 / FEAT-25 の訳 (「マップ画像を出力」等4語、「同時に表示できないマップ表示オプションが指定されています:」、
  「サムネイル表示」「大きく表示」) は各機能の取り込みコミット (`90c913f79`, `e9cf6b0d8`, `51a5310ed`) で追加済み。

### MISC-03: SDL3 バックエンド対応と CI のビルド構成更新

- **分類**: その他
- **状態**: 未取り込み
- **概要**: 新しいバックエンド `sdl3` を追加し、Linux / macOS / Windows の CI ビルドを
  SDL3 (Ubuntu は SDL2 と SDL3 の両方) に切り替える。
- **上流コミット**:
  - `a7aa63f9e` Merge pull request #716 from teamhimeh/sdl3-support (マージコミット)
  - `08f9bc0e2` ADD: SDL3 backend support — 本体
  - `a961e69d8` switch Linux and macOS builds to SDL3
  - `ac2e2a3b6` ADD: Windows SDL3 build
  - `a24090e8b` FIX: use the MSYS2 SDL3 package name
  - `e22d59740` CI: build Ubuntu SDL2 and SDL3 variants
  - `7235365ca` FIX: support miniupnpc API 18
  - `713a26cb6` FIX: support the current MSYS2 Brotli layout
  - `5846222f8` FIX: apply SDL3 display scale to user scaling
  - `52a74c56b` remove SDL3 build (#727) — 後続修正 (Ubuntu 26.04 / SDL3 の CI ジョブを削除)
  - `64abad676` change release name for windows — 後続修正 (Windows GDI 版の成果物名を `sim-WinGDI64-OTRP` へ戻す)
- **主な変更箇所**:
  - `sys/simsys_s3.cc` (新規, 約1960行), `sys/clipboard_s3.cc` (新規), `sound/sdl3_sound.cc` (新規)
  - `Makefile`, `config.default.in`, `config.template`, `configure.ac` — `BACKEND := sdl3` と
    `SDL3_CONFIG` を追加。`configure` は SDL2 より SDL3 を優先して検出する
  - `network/network.cc` — miniupnpc API 18 対応 (`7235365ca`)
  - `.github/build64-SDL3.sh` (新規), `.github/build-mac.sh`, `.github/package-mac-app.sh`,
    `.github/workflows/otrp-{windows-64,ubuntu,macos,automated-tests}.yml`
- **詳細**: `5846222f8` は SDL3 のディスプレイスケールをユーザ拡大率へ反映する修正。
  `52a74c56b` で Ubuntu 26.04 / SDL3 の CI ビルドマトリクスが削除され、
  現在の CI では Ubuntu は SDL2 のみをビルドする (`sdl3` バックエンドのコード自体は残っている)。
- **取り込み時の注意**: `develop-kasumi` のローカルビルドは MSYS2 / MINGW64 の
  **gdi バックエンド** を使っているため、実行バイナリへの影響は無い。
  CI ワークフローを取り込む場合のみ検討すればよい。
- **develop-kasumi 側コミット**: —
- **備考**: バックエンド追加のみで、ゲームロジック・セーブ形式には影響しない。

### MISC-04: `obj_t::finish_rd()` にセーブの OTRP バージョンを渡す

- **分類**: その他
- **状態**: 未取り込み
- **概要**: マップ上オブジェクトのロード後処理 `finish_rd()` に、読み込んだセーブの
  OTRP バージョンを引数で渡せるようにする基盤変更。併せて旧セーブの道路 choose 標識の互換処理を入れる。
- **上流コミット**:
  - `0ceed13b1` add OTRPversion in obj_t::finish_rd() (#731)
- **主な変更箇所**:
  - `obj/simobj.h` — `virtual void finish_rd()` → `virtual void finish_rd(const uint8 loaded_OTRP_version)`。
    オーバーライドしている全クラス (`weg_t`, `baum_t`, `bruecke_t`, `crossing_t`, `gebaeude_t`,
    `label_t`, `leitung_t`, `roadsign_t`, `tunnel_t`, `wayobj_t`, `private_car_t` ほか) と
    その呼び出し側 (`bauer/*`, `boden/grund.cc`, `simconvoi.cc`, `simtool.cc`) を追従 (計31ファイル)
  - `simworld.{h,cc}` — `uint8 load_otrp_version` を追加。`rdwr_gamestate()` のロード時に
    `file->get_OTRP_version()` を格納し、`plans_finish_rd()` から各オブジェクトへ渡す。
    セーブを読んでいない間は `OTRP_VERSION_MAJOR`
  - `obj/roadsign.cc` — `roadsign_t::finish_rd()` で、**OTRP v61 未満**のセーブから読んだ
    `road_wt` の choose 終端標識に対し `set_end_of_choose(true)` / `set_end_of_guide(true)` を強制する
- **詳細**: v61 より前は道路の choose 終端フラグが UI にも出ず道路車両も無視していたため、
  保存されている値が無意味だった。FEAT-15 で道路がフラグを参照するようになったので、
  旧セーブの標識がフラグ未設定のせいで突然「終端でなくなる」ことを防ぐ。
- **取り込み時の注意**:
  - シグネチャ変更が広範囲に及ぶため、`develop-kasumi` 独自の `finish_rd()` 実装がある場合は追従が必要。
  - `roadsign_t` の互換処理は `get_OTRP_version() < 61` を見るため、**FEAT-15 とセットで扱うこと**。
    バージョン番号の扱いは MISC-01 参照。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simworld.h` の `load_otrp_version`。

### MISC-05: OTRP 用アドオン pak を `documentation/` に同梱

- **分類**: その他
- **状態**: 未取り込み
- **概要**: OTRP 独自機能が必要とするアドオン pak (と、その `.dat` / 元画像) を
  `documentation/OTRP_addons/` 以下に同梱する。
- **上流コミット**:
  - `db82b5881` add OTRP paks in documentation (#734)
- **主な変更箇所**:
  - `documentation/OTRP_addons/RibiArrow/for64/`, `.../for128/` — 一方通行等の矢印表示用
    `misc.RibiArrow.pak` と `OTRP_Arrow.dat` / `OTRP_Arrows.png`
  - `documentation/OTRP_addons/dummy_goods/` — `OTRP_dummy_goods.dat` と
    `good.Reverse.pak` / `good.No_Electric.pak` / `good.Reverse_No_Electric.pak`、および
    航送用ダミー貨物 `good.SHIPPING_{ROAD,TRACK,MONORAIL,MAGLEV,NARROWGAUGE,WATER}.pak`
- **詳細**: `SHIPPING_*` は FEAT-18 (航送) が名前で解決するダミー貨物。
  pakset にこれらが無い waytype は航送できない。
- **取り込み時の注意**: ソースコードの変更は一切無く、pakset 側の追加ファイルのみ。
  `develop-kasumi` のバイナリ動作には影響しないが、**FEAT-18 を取り込む場合は
  使用中の pakset にダミー貨物を導入する必要がある**。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `documentation/OTRP_addons/` ディレクトリの有無。
