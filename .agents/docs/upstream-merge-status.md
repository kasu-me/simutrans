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
| 追跡済み最新コミット (head) | `e2dd3a237ae55c0071bd6f97e5652d699fae4d46` (Merge branch 'OTRP-KUTAv6' / v62_0_1 時点) |
| 対象コミット数 | 84 |
| 最終更新 | 2026-09-20 |

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

---

## 一覧表

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
| FEAT-16 | 新機能追加 | 1タイルに2本の way を斜めで共存させる (異なる waytype / 同一 waytype) | 未取り込み |
| FEAT-17 | 新機能追加 | 保守費・走行費の倍率設定 (`maintenance_cost_multiplier_*` 等) | 未取り込み |
| FEAT-18 | 新機能追加 | 航送 (編成を他の編成に積載して輸送する) | 未取り込み |
| FEAT-19 | 新機能追加 | 無閉塞運行 (`drive_without_reservation`) | 未取り込み |
| FEAT-20 | 新機能追加 | choose 標識の「長さを無視」設定 (`ignore_length`) | 未取り込み |
| FEAT-21 | 新機能追加 | 停留所の私有化で道・架線の所有権も移転し、費用を精算する | 未取り込み |
| FEAT-22 | 新機能追加 | 車両オフセット付き線路で対向2編成が同一タイルに進入できる | 未取り込み |
| FEAT-23 | 新機能追加 | 追越モードに `exclusive_area` / `passing_lane_stop_only` を追加 | 未取り込み |
| FEAT-24 | 新機能追加 | ミニマップ全体の PNG 出力 | 未取り込み |
| FIX-01 | バグ修正 | choose 標識/choose 区間終端が経路上にある場合の判定 | 取り込み済み |
| FIX-02 | バグ修正 | pak の `clip_below` 設定の読み込み順序 | 取り込み済み |
| FIX-03 | バグ修正 | 一方通行詳細設定の waytype 判定漏れと経路喪失時の異常終了 | 取り込み済み |
| FIX-04 | バグ修正 | 積込中に「次の停留所へ」を押した際の停車時間等の未更新 | 取り込み済み |
| FIX-05 | バグ修正 | 経路予約解除ツールが連結子編成を解放してしまう | 取り込み済み |
| FIX-06 | バグ修正 | 橋脚が滑走路 (`air_wt`) タイル上に建つ | 取り込み済み |
| FIX-07 | バグ修正 | Windows GDI の DPI スケーリング時のリサイズ処理 | 取り込み済み |
| FIX-08 | バグ修正 | 整地ツールが基礎 (`fundament`) タイルで高低差制限を無視する | 取り込み済み |
| FIX-09 | バグ修正 | 誘導標識を使わない軌道系連結で、ホーム上の待機編成を見つけられない | 未取り込み |
| MISC-01 | その他 | 本家バージョン番号のインクリメント (v59〜v61_0_4) | 取り込み不要 |
| MISC-02 | その他 | 日本語訳 `ja.OTRP.tab` の更新 (v59/v60/v61/v62分) | 未取り込み |
| MISC-03 | その他 | SDL3 バックエンド対応と CI のビルド構成更新 (Ubuntu 分は後に削除) | 未取り込み |
| MISC-04 | その他 | `obj_t::finish_rd()` にセーブの OTRP バージョンを渡す基盤変更 | 未取り込み |
| MISC-05 | その他 | OTRP 用アドオン pak (矢印・ダミー貨物) を `documentation/` に同梱 | 未取り込み |

---

## 詳細

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
- **詳細**:
  - 会社番号を扱う多数の箇所を uint8 前提で見直し、フラグ用ビットマスクを64社分に拡張している。
  - 旧セーブ形式で保存しようとすると、ネットワークモードでは「16社以上あるため現行形式で保存する」旨を通知、
    シングルプレイでは「会社0へ統合が必要。SHIFT+保存で確定」という2段階確認になる。
- **取り込み時の注意**:
  - **セーブデータ形式が変わる** (本家では OTRP v59 相当)。取り込む場合は `simversion.h` の
    `OTRP_VERSION_MAJOR` の扱い (MISC-01) を必ず併せて検討すること。
  - 変更範囲が非常に広くネットワーク互換性にも影響する。単独 cherry-pick の難度は高い。
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
- **取り込み時の注意**: `schedule_entry_t` にフラグとフィールドが増えるため、スケジュールのセーブ形式に影響する。
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
- **取り込み時の注意**:
  - セーブ形式への影響はないが、旧セーブに残る `crossing_t` の扱いが変わる。
  - `boden/grund.cc` / `bauer/wegbauer.cc` は基本エンジン部分なので、敷設判定全般への副作用に注意。
  - `58bb2b989` は `get_weg()` の呼び出し規約が変わる広範囲の変更で、`simconvoi.cc` /
    `vehicle/simvehicle.cc` の予約まわりを大きく書き換えるため、`develop-kasumi` 独自改造との競合に注意。
  - FEAT-22 (オフセット付き線路の対向同時進入) は `58bb2b989` の `corner_set` 基盤を前提にしているため、
    FEAT-22 を取り込む場合は本項目が先行する。
  - `58bb2b989` が追加した英語表示文字列 (`This tile has a second, disconnected way of the same type:` 等) は
    本家でも `ja.OTRP.tab` に未登録 (MISC-02 参照)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/ribi.h` の `are_disjoint_bends` (本体)、
  `boden/grund.h` の `get_weg(waytype_t, ribi_t::ribi)` または
  `dataobj/route.h` の `get_corner_set` (同一 waytype 拡張分)。

### FEAT-17: 保守費・走行費の倍率設定

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: pakset が持つ「道の保守費」「架線等 wayobj の保守費」「車両の走行費」に対して、
  ゲーム側から % 倍率をかけられる設定を追加する。
- **上流コミット**:
  - `fa3506863` maintainance multiplier (#724)
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
- **詳細**: `world()` が未生成の間 (pakset 読み込み中など) は倍率をかけず素の値を返す。
- **取り込み時の注意**: `settings_t::rdwr()` が `get_OTRP_version() >= 61` でゲートされているため
  **セーブ形式に影響する** (MISC-01 参照)。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `dataobj/settings.h` の `maintenance_cost_multiplier_way`。

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
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `simconvoi.h` の `get_shipping_capacity`、
  または `bauer/goods_manager.h` の `get_shipping_goods`。

### FEAT-19: 無閉塞運行 (`drive_without_reservation`)

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 軌道系 (track / monorail / maglev / narrowgauge / tram) のスケジュールで、
  その停留所から次の信号までを閉塞予約なしで運行する設定を追加する。
- **上流コミット**:
  - `85aef1543` Drive without reservation (#713) — 本体
  - `3fc2f3345` BUG FIX: infinite loop when only waypoint (#736) — 後続修正
  - `8ef07c84b` BUG FIX: reservation tiles when loading savedata (#742) — 後続修正
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
- **詳細**: `3fc2f3345` は、スケジュールが中継点のみで構成されている場合に
  `block_reserver()` 内の走査ループが終了せず無限ループになる問題の修正
  (`get_current_stop()` が uint8 のため終了条件 `i_stop != get_current_stop()-1` が成立しない)。
  ループをエントリ数で明示的に打ち切る形に書き換えている。
  `8ef07c84b` は下記「本家に未修正の不具合」に挙げていた `simconvoi.cc` の `?:` 優先順位バグの修正。
  `convoi_t::reserve_route()` の予約ループ上限を `(drive_without_reservation ? front()->get_route_index() : next_reservation_index)`
  と括弧で囲み、意図どおりの上限で予約するようにした。
- **取り込み時の注意**:
  - `convoi_t::rdwr()` が `get_OTRP_version() >= 61` でゲートされているため **セーブ形式に影響する** (MISC-01 参照)。
  - `schedule_entry_t` にフラグが増えるため、`develop-kasumi` 独自のスケジュール入出力
    (`dataobj/schedule_io.cc` の `stop_flag_names`) に `WITHOUT_RESERVATION` の追加が必要。
  - **本家に未修正の不具合が同梱されている** (2026-09-16 時点の `origin/OTRP-KUTAv6` でも残存):
    - `simconvoi.cc:1521` — `get_most_parent_convoi()->state==ROUTING_1;` が代入 `=` ではなく比較 `==` になっており、
      連結反転時の状態設定が効いていない (元は `state=ROUTING_1`)
    取り込む場合はこの点を直してから入れること。
  - `simconvoi.cc:367` の `?:` 優先順位バグ
    (`idx < drive_without_reservation?A:B && ...` が `(idx < drive_without_reservation) ? A : (B && ...)` と解釈される)
    は `8ef07c84b` で本家修正済み。**この後続修正も併せて取り込むこと。**
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
  表示文字列 `ignore length` は本家でも `ja.OTRP.tab` に未登録 (MISC-02 参照)。

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
- **主な変更箇所**:
  - `boden/wege/schiene.{h,cc}` — `reserved_travel_dir` / `reserved2_travel_dir` を追加し、
    `reserve()` / `can_reserve()` / `can_co_reserve_with()` に `travel_dir` 引数を追加。
    `can_co_reserve_offset(travel_dir)` を新設
  - `dataobj/route.{h,cc}` — `route_t::get_travel_dir(index)` を新設
    (タイルに進入する向き。`corner_set` と違い正反対の2方向を区別できる)
  - `simconvoi.{h,cc}` — `get_reserved_tiles_travel_dir(index)` を新設。
    `reserve_route()` / `vorfahren()` / `uncouple_convoi()` / `finish_rd()` の予約呼び出しに向きを渡す
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::get_current_travel_dir()` を新設し、予約系の呼び出しに反映
- **詳細**:
  - 同時進入が許されるのは、(a) 2つの向きが厳密に正反対の single ribi、
    (b) `vehicle_offset_mode == 0` (絶対オフセット) かつ `vehicle_offset != 0`、
    (c) タイルが直線または斜め (`is_twoway`) で分岐・交差でない、をすべて満たす場合のみ。
    オフセットモード1 (方向依存) は向きを `d%4` に畳むため両編成が同じ側になり、許可しない。
  - 予約方向 (`corner_set`) は正反対の2つの走行方向を区別できないため、走行方向を別フィールドで持つ。
  - `finish_rd()` (ロード直後) でも走行方向を渡さないと `reserved_travel_dir` が
    `ribi_t::none` のままになり、以後この機能が効かなくなるため併せて修正されている。
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

### FEAT-24: ミニマップ全体の PNG 出力

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: ミニマップ (地図) ウィンドウに「マップ画像を出力」ボタンを追加し、
  現在の表示設定・ズームでマップ全体を PNG ファイルに書き出せるようにする。
- **上流コミット**:
  - `859a2244d` Add full minimap PNG export (#744)
- **主な変更箇所**:
  - `io/raw_image.h`, `io/raw_image_png.cc` — `raw_image_png_writer_t` (行単位に書き出す
    逐次 PNG ライタ) を新設。メモリに載らない大きさの画像に対応する
  - `display/simgraph.h`, `display/simgraph16.cc`, `display/simgraph0.cc` —
    画面領域を `raw_image_t` の指定位置へ写す `display_snapshot(area, image, destination)` を追加
  - `gui/minimap.{h,cc}` — `minimap_t::export_to_png(std::string &filename)` を新設。
    画面サイズ単位のタイルに分けて描画しつつ、最大32MBの帯 (strip) 単位で PNG へ流し込む
  - `gui/map_frame.{h,cc}` — `b_export_map` ボタンと結果通知 (`news_img`) を追加
  - `documentation/ja.OTRP.tab` — 「マップ画像を出力」等 (MISC-02 参照)
- **詳細**: 出力先はスクリーンショットフォルダ (`simmap00.png` から連番)。
  出力中はミニマップのオフセット/サイズとクリップ領域を一時的に差し替え、終了後に復元する。
- **取り込み時の注意**: セーブ形式への影響なし。`display_snapshot()` のオーバーロード追加のため
  バックエンド側 (`simgraph16.cc` / `simgraph0.cc`) の追従が必要。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `io/raw_image.h` の `raw_image_png_writer_t`、
  または `gui/minimap.h` の `export_to_png`。

### FIX-01: choose 標識/choose 区間終端が経路上にある場合の判定

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 道路車両の choose 経路判定で、経路上にさらに choose 標識または choose 区間終端がある場合に `true` を返すようにする。
- **上流コミット**:
  - `6848892d1` BUG FIX: return true when end of choose or choose sign is on the route (#697)
- **主な変更箇所**:
  - `vehicle/simvehicle.cc` — `road_vehicle_t::choose_route()` に、現在位置より先の経路を走査して
    `END_OF_CHOOSE_AREA` フラグを持つ標識、または次の choose 標識が空き経路を示す場合に `true` を返すループを追加
- **詳細**: 従来は経路終端の停留所のみを見ていたため、choose 区間が経路の途中で終わるケースで誤判定していた。
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: `c7e9c04d5`
- **備考**: —

### FIX-02: pak の `clip_below` 設定の読み込み順序

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 橋 pak の読み込みで `clip_below` と `number_of_seasons` の読み出し順序が逆になっていた不具合を修正する。
- **上流コミット**:
  - `1168d03f2` BUG FIX: load clip_below settings of pakfiles (#695)
- **主な変更箇所**:
  - `descriptor/reader/bridge_reader.cc` — `desc->clip_below` と `desc->number_of_seasons` の `decode_uint8()` 呼び出し順を入れ替え
  - `descriptor/reader/way_reader.cc` — インデント修正のみ (機能変更なし)
- **詳細**: —
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: `2a8290a43`
- **備考**: —

### FIX-03: 一方通行詳細設定の waytype 判定漏れと経路喪失時の異常終了

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 一方通行の詳細設定 (`detailed_oneway`) を、同じタイルを共有する別 waytype の
  標識にまで適用してしまう問題と、再経路探索に失敗したときに `route_t::at()` が
  範囲外参照して異常終了する問題を修正する。
- **上流コミット**:
  - `4c3287d11` BUG FIX: fatal when no route (#714)
- **主な変更箇所**:
  - `obj/roadsign.h` — `get_governed_waytype()` を追加 (`tram_wt` の標識は `track_wt` を返す)
  - `dataobj/route.cc` — `find_route()` / `intern_calc_route()` の一方通行判定に
    `rs->get_governed_waytype() == w->get_waytype()` の条件を追加
  - `vehicle/simvehicle.{h,cc}` — `hop_check()` と `get_ribi()` に同じ waytype 判定を追加。
    `vehicle_t::clamp_route_index()` を新設
  - `vehicle/simroadtraffic.cc` — 市内車 (`private_car_t`) の3箇所にも `road_wt` 判定を追加
  - `simconvoi.cc` — `drive_to()` で経路探索に失敗したとき、連結子編成も含む全車両の
    `route_index` を1タイルのスタブ経路に合わせてクランプする
- **詳細**: 例えば路面電車の線路と道路が同居するタイルで、道路用の一方通行標識が
  路面電車の経路判定に効いてしまっていた。また `calc_route()` 失敗時は経路が
  `[start]` の1タイルだけになるのに車両側は以前の大きな `route_index` を保持したままで、
  25秒後の再試行までの間に `route_t::at()` が範囲外を参照していた。
- **取り込み時の注意**: `develop-kasumi` の車線制御まわりと同じファイルを触るため、
  cherry-pick 時は `vehicle/simvehicle.cc` の競合に注意
  (2026-09-14 の取り込み時は競合なく適用でき、上流 diff と内容一致)。
- **develop-kasumi 側コミット**: `5325bfc8a`
- **備考**: FEAT-13 と同じ作業ブランチから派生している (コミット本文の履歴が共通)。
  ただし FEAT-13 は GUI のみの変更で依存関係は無く、FIX-03 単独で取り込める。
  状態確認用の識別子は `obj/roadsign.h` の `get_governed_waytype`。

### FIX-04: 積込中に「次の停留所へ」を押した際の停車時間等の未更新

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 積込中の編成に対して「次の停留所へ」ボタンを押したとき、停車時間や
  貨物待機時間などの記録・状態リセットが行われずに出発してしまうのを修正する。
- **上流コミット**:
  - `785fb44fb` push stopping time and another values when next stop button pressed during loading (#710)
- **主な変更箇所**:
  - `simconvoi.cc` — `convoi_t::next_stop_button_pressed()` で、対象編成が `is_loading()` の場合に
    `push_goods_waiting_time_if_needed()`, `push_convoy_stopping_time()`,
    `set_coupling_done(false)`, `set_waiting_for_departure_allowance_by_other_convoy(false)`,
    `reset_departure_time()` を呼ぶ
- **詳細**: 通常の発車経路では行われる後始末が、ボタンによる強制発車では抜けていた。
- **取り込み時の注意**: `set_waiting_for_departure_allowance_by_other_convoy()` は
  FEAT-02 (他編成との発車待ち合わせ) で追加されるメソッドで `develop-kasumi` には存在しない。
- **develop-kasumi 側コミット**: `aefd5a3b1`
- **備考**: FEAT-02 未取り込みのため
  `set_waiting_for_departure_allowance_by_other_convoy(false)` の1行のみ省略して取り込んだ
  (該当箇所にコメントを残してある)。**FEAT-02 を取り込む際はこの行を復活させること。**

### FIX-05: 経路予約解除ツールが連結子編成を解放してしまう

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 予約解除ツールを連結編成の車両に使うと、子編成が単独で `ROUTING_1` にされて
  連結が壊れる問題を修正する。
- **上流コミット**:
  - `033e83a21` Do not release child convoys when clear reservation (#720)
- **主な変更箇所**:
  - `simtool.cc` — `tool_clear_reservation_t::work()` で `veh->get_convoi()` ではなく
    `get_most_parent_convoi()` の状態を見て `set_state()` する
- **詳細**: —
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: `8764b27c8`
- **備考**: —

### FIX-06: 橋脚が滑走路 (`air_wt`) タイル上に建つ

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 橋の橋脚 (`pillar_t`) が滑走路タイルの上に生成されるのを禁止する。
- **上流コミット**:
  - `2d93c0086` Prohibit bridge pillars from landing on air_wt tiles (#643)
- **主な変更箇所**:
  - `bauer/brueckenbauer.cc` — `build_bridge()` の橋脚生成条件に `!gr->hat_weg(air_wt)` を追加
- **詳細**: 従来は橋脚の設置間隔が pak の `pillars_every` のみで決まり、
  エンジン側の waytype 制限が無かった。同ファイルの「滑走路を跨ぐ橋は不可」ルールを補完する形。
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: `46e5d7a9c`
- **備考**: —

### FIX-07: Windows GDI の DPI スケーリング時のリサイズ処理

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: Windows GDI バックエンドで、ディスプレイ拡大率が 100% 以外のときに
  ウィンドウのリサイズが正しく反映されない/取りこぼされる問題を修正する。
- **上流コミット**:
  - `221c85198` Fix Windows GDI resize handling under DPI scaling (#721)
- **主な変更箇所**:
  - `sys/simsys_w.cc` — `dr_textur_resize()` が既に論理ピクセルの `w`, `h` を
    さらに `x_scale` で割っていた二重スケーリングを修正。`WM_SIZE` は最新のクライアントサイズのみを保持し、
    `GetEvents()` が空き次第 `sys_event` スロットへ渡すように変更
  - `simevent.cc` — `queue_event()` でキュー済みのリサイズイベントを破棄し、
    古いサイズが後から再適用されないようにする
- **詳細**: `WM_PAINT` は `WindowSize` を物理ピクセルとして扱うため、拡大率150%では
  実際の 4/9 程度のサイズになっていた。また `WM_SIZE` が単一の `sys_event` スロットに書くため、
  直後の `WM_MOUSEMOVE` に上書きされてリサイズが失われることがあった。
- **取り込み時の注意**: `develop-kasumi` のビルドは GDI バックエンドのため影響が大きい
  (取り込み後のビルドは通過済み。実機での拡大率100%以外の動作確認は未実施)。
- **develop-kasumi 側コミット**: `4f0948468`
- **備考**: 状態確認用の識別子は `simevent.cc` / `sys/simsys_w.cc`。フォーラム topic 23805 の報告に関連。

### FIX-08: 整地ツールが基礎 (`fundament`) タイルで高低差制限を無視する

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 整地ツールで産業付属地 (field) の下にある基礎タイルの高さを変えるとき、
  隣接タイルとの高低差チェックが行われず、不正な地形ができてしまうのを修正する。
- **上流コミット**:
  - `87adadee4` BUG FIX: field groundlevel change (#728)
- **主な変更箇所**:
  - `simtool.cc` — `tool_setslope_t::tool_set_slope_work()` の隣接高低差チェックの条件を
    `gr1->get_typ()==grund_t::boden` から `... || gr1->get_typ()==grund_t::fundament` に拡張
- **詳細**: この地点まで到達する基礎タイルは field を持つもの (建物タイルは手前で弾かれる) で、
  ツールによって高さを変更されうるため、平地と同様に隣接高低差制限に従う必要がある。
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: `6ae1d7c88`
- **備考**: 状態確認用の識別子は `simtool.cc` の `grund_t::fundament` を含む条件式。

### FIX-09: 誘導標識を使わない軌道系連結で、ホーム上の待機編成を見つけられない

- **分類**: バグ修正
- **状態**: 未取り込み
- **概要**: 誘導標識 (guide signal) を使わずに軌道系の編成を連結する場合、
  ホームの奥で待っている編成が連結相手として検出されず、連結が成立しない問題を修正する。
- **上流コミット**:
  - `4980770e6` BUG FIX: coupling target find method for non-using guide signal (#739)
- **主な変更箇所**:
  - `vehicle/simvehicle.{h,cc}` — `rail_vehicle_t` に `get_next_coupling_stop()`,
    `get_coupling_halt()`, `get_platform_tiles_behind_route()`, `check_platform_coupling()` を追加。
    `can_couple()` を `const` 化。`can_enter_tile()` と `block_reserver()` から
    `check_platform_coupling()` を呼ぶ
  - `dataobj/schedule.cc` — `get_next_halt()` のデバッグ出力を削除
- **詳細**:
  - 自編成の経路は「自分に割り当てられた停留所位置」で終わるため、ホームのさらに奥に
    停まっている編成は経路上に現れない。修正後は経路末尾から線路をたどって
    **同じ停留所に属するタイル**を集め、ホーム全体を連結相手の探索対象にする。
    たどる際は `check_next_tile()` (架線・速度・私有道路等) と `ribi_maske` (信号・一方通行) を見る。
  - 経路全体が予約済みになると新たな予約契機 (信号) が無くなるため、
    連結停留所へ接近中は**タイルを進むたび**に探索し直す。
    無駄な探索を避けるため「進入するタイルが連結先停留所に属する」ことを先に確認する。
  - 併せて、`drive_to()` による経路再構築で `next_coupling_index` が経路範囲外に取り残される
    ケースを検出し、破棄して通常編成として走行を続けるようにしている。
- **取り込み時の注意**: 連結 (coupling) 機能自体は v58_3 時点から存在する。セーブ形式への影響なし。
  ただし `vehicle/simvehicle.cc` の `can_enter_tile()` / `block_reserver()` を触るため、
  同領域を変更する FEAT-15 / FEAT-20 との取り込み順に注意。
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `vehicle/simvehicle.h` の `check_platform_coupling`。

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
  - 上記以外の項目 (FEAT-09 拡張分、FEAT-11〜FEAT-16、FEAT-20〜FEAT-24、FIX-03〜FIX-09、
    MISC-03 / MISC-05) は `get_OTRP_version()` によるセーブ形式の分岐を持たないため、バージョン引き上げは不要。
- **develop-kasumi 側コミット**: — (独自運用: `cf52a5750` 等)
- **備考**: バージョン番号自体は取り込まない方針。上記の依存関係のみ管理する。

### MISC-02: 日本語訳 `ja.OTRP.tab` の更新

- **分類**: その他
- **状態**: 未取り込み
- **概要**: v59 / v60 / v61 / v62 で追加された機能に対応する日本語訳の追加。
- **上流コミット**:
  - `8da4abc3e` update ja.OTRP.tab for v59
  - `d65dc2063` add ja.OTRP.tab for v60
  - `68348e37e` update ja.OTRP.tab for v61
  - `68d996bca` update ja.OTRP.tab for dummy-goods name
  - (`d272db352` にも v59 訳文の修正が含まれる。FEAT-01 参照)
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
  - v62分も翻訳のみのコミットは無く、機能コミットに同梱されている:
    `a9c90e534` の「(%s) は私有化されました．」(FEAT-21)、
    `859a2244d` の「マップ画像を出力」等 (FEAT-24)、
    `b78c3685c` の「一編成専用区間」「追越車線は停車のみ可」(FEAT-23)
  - **本家でも未登録の文字列**: `ignore length` / `ignore length. Convoy can enter shorter stop` (FEAT-20)、
    `This tile has a second, disconnected way of the same type:` (FEAT-16 の `58bb2b989`)。
    これらの項目を取り込む場合は訳文を自前で追加する必要がある。
  - v60_1 〜 v61 の間は翻訳のみのコミットは無く、機能コミットに同梱されている:
    `5461ae2d2` の「発車許可を出す路線を指定」(FEAT-02)、
    `eaae6f8b2` の「通行方向の詳細設定をコピー/貼り付け」「通行可能な方向」等 (FEAT-13)
- **取り込み時の注意**: `develop-kasumi` は独自に `ja.OTRP.tab` を編集している (`dd440ea4b`, `fa91f31fc` 等) ため
  ファイル単位の cherry-pick は競合しやすい。**対応する機能項目を取り込む際に、必要な行だけを追加する**運用とする。
- **develop-kasumi 側コミット**: —
- **備考**: —

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
