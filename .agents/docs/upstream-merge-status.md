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
| 追跡済み最新コミット (head) | `d65dc206394d8e76b9c4a3dab627bd0e16e8382a` (add ja.OTRP.tab for v60) |
| 対象コミット数 | 25 |
| 最終更新 | 2026-08-30 |

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
| FEAT-09 | 新機能追加 | ミニマップへの路線/編成経路表示 | 未取り込み |
| FEAT-10 | 新機能追加 | 路線一覧のメモ欄をフィルタ対象に追加 | 未取り込み |
| FIX-01 | バグ修正 | choose 標識/choose 区間終端が経路上にある場合の判定 | 取り込み済み |
| FIX-02 | バグ修正 | pak の `clip_below` 設定の読み込み順序 | 取り込み済み |
| MISC-01 | その他 | 本家バージョン番号のインクリメント (v59〜v60_0_1) | 取り込み不要 |
| MISC-02 | その他 | 日本語訳 `ja.OTRP.tab` の更新 (v59/v60分) | 未取り込み |

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
- **主な変更箇所**:
  - `dataobj/environment.{h,cc}` — `iconsize_set_by_pak` (menuconf.tab の `icon_height` をテーマ再読み込みで上書きさせない)、
    `menu_scrollbar_thickness` (=10px) を追加
  - `gui/tool_selector.{h,cc}` — `is_scrollbar_dragging`, `get_scrollbar_rect()`, `get_scroll_metrics()`,
    `get_tool_count()`, `get_icon_area_offset()` (`6cf6f5dd7`) を追加。
    1行のみのツールバーは水平、複数行のグリッドは行単位の垂直スクロール
  - `gui/simwin.{h,cc}` — `get_main_menu_scrollbar_extra()` を追加し、メニューバーの実占有量をウィンドウ側とマップ表示側で共有
  - `display/simview.cc`, `display/simgraph16.cc`, `gui/gui_theme.cc`, `gui/welt.cc`, `gui/enlarge_map_frame_t.cc`, `simmenu.cc`
  - `simmain.cc` — `tool_t::read_menu()` の呼び出しをテーマ読み込みの**後**に移動 (menuconf.tab の `icon_height` を最終値にするため)
- **詳細**:
  - スクロール不要なときはスクロールバーを描画しない。アイコン高さ0による除算を回避。テーマ再読み込み時もアイコンサイズを維持。
  - `6cf6f5dd7` は `env_t::menupos` がメニューバーを画面下端/右端に置いている場合に、スクロールバー帯の分だけアイコン領域を外側にずらす修正。
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

### FEAT-09: ミニマップへの路線/編成経路表示

- **分類**: 新機能追加
- **状態**: 未取り込み
- **概要**: 編成情報/路線一覧の「経路を表示」で、メインマップだけでなくミニマップ上にも経路タイルをハイライト表示する。
- **上流コミット**:
  - `abb9c92a0` show route in minimap (#702)
- **主な変更箇所**:
  - `gui/minimap.{h,cc}` — `vector_tpl<koord3d> highlighted_route_tiles` と
    `set_highlighted_route()` / `clear_highlighted_route()` を追加。`draw()` で `COL_SOFT_BLUE` の矩形として描画。`init()` でクリア
  - `gui/convoi_info_t.cc` — `convoi_info_t::show_route()` からミニマップへ経路を設定/クリア
  - `gui/schedule_list.cc` — `show_route_cache()` からも同様に設定/クリア。
    経路キャッシュ設定が無効でも、路線所属編成の実経路にフォールバックして表示する。
    ボタンの有効条件を「経路キャッシュ有効」から「路線に編成が1つ以上ある」に変更
- **詳細**: —
- **取り込み時の注意**: —
- **develop-kasumi 側コミット**: —
- **備考**: 状態確認用の識別子は `gui/minimap.h` の `highlighted_route_tiles`。

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
- **主な変更箇所**: `simversion.h` — `OTRP_VERSION_MAJOR` / `OTRP_VERSION_MINOR` / `OTRP_VERSION_PATCH`
- **詳細**: —
- **取り込み時の注意**:
  `develop-kasumi` は `OTRP_VERSION_MAJOR 58` / `MINOR 3` に独自の `AHOZURA_VERSION` を組み合わせて運用しているため、
  本家のバージョン番号はそのまま取り込まない。ただし **セーブ形式が `get_OTRP_version()` で分岐する機能を
  取り込む場合は、`OTRP_VERSION_MAJOR` の引き上げ判断が必須**:
  - `OTRP_VERSION_MAJOR >= 59` を要求する項目: FEAT-01 / FEAT-04 / FEAT-05
  - `OTRP_VERSION_MAJOR >= 60` を要求する項目: FEAT-07
- **develop-kasumi 側コミット**: — (独自運用: `cf52a5750` 等)
- **備考**: バージョン番号自体は取り込まない方針。上記の依存関係のみ管理する。

### MISC-02: 日本語訳 `ja.OTRP.tab` の更新

- **分類**: その他
- **状態**: 未取り込み
- **概要**: v59 / v60 で追加された機能に対応する日本語訳の追加。
- **上流コミット**:
  - `8da4abc3e` update ja.OTRP.tab for v59
  - `d65dc2063` add ja.OTRP.tab for v60
  - (`d272db352` にも v59 訳文の修正が含まれる。FEAT-01 参照)
- **主な変更箇所**: `documentation/ja.OTRP.tab`
- **詳細**:
  - v59分: 「常に最長の待機時間を使用する．」「発車許可待ち」「他編成の到着を待機」(FEAT-02)、
    `penalty_wait_for_two_month` (FEAT-04)、`base_revenue_from_halt` / 「収入」(FEAT-05)、
    16社以上の保存警告 (FEAT-01)、「キャッシュ中のルートを表示」(FEAT-09 関連)
  - v60分: 「他編成を発車させるまで待機」等 (FEAT-02)、`tile_length` / 「走行距離 (m)」(FEAT-07)
- **取り込み時の注意**: `develop-kasumi` は独自に `ja.OTRP.tab` を編集している (`dd440ea4b`, `fa91f31fc` 等) ため
  ファイル単位の cherry-pick は競合しやすい。**対応する機能項目を取り込む際に、必要な行だけを追加する**運用とする。
- **develop-kasumi 側コミット**: —
- **備考**: —
