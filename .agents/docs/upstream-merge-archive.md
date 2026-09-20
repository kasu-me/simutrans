# 本家 (OTRP-KUTAv6) 取り込み状況アーカイブ

[upstream-merge-status.md](upstream-merge-status.md) の項目のうち、
**状態が `取り込み済み` になったもの**の「詳細」をここに退避する。

- 一覧表は退避せず、本体の [upstream-merge-status.md](upstream-merge-status.md) 側に残す。
  本体の一覧表に載っている `取り込み済み` の項目は、詳細が必ずこのファイルにある。
- 項目IDと本文は移設時のまま保持する。**ID の変更・再利用はしない。**
- 取り込み済みの項目に後続の本家コミットが現れた場合は、
  本体側へ項目を戻して状態を `一部取り込み` に変更する (このファイルからは削除する)。

## 詳細 (取り込み済み)

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
- **状態**: 取り込み済み
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
  - **develop-kasumi での再現性 (2026-09-20 確認)**: 修正前の本家 (`4980770e6^`) と
    `develop-kasumi` では `rail_vehicle_t::can_couple()` の呼び出し箇所がすべて一致しており
    (いずれも `cnv->get_route()` を渡す)、経路末尾より奥のホームタイルは検査対象外。
    `block_reserver()` の失敗時処理 (`!success` で予約解放して `false` を返す) も同一のため、
    **待機編成が占有するホームタイルを予約できず手前で停止し、連結が成立しない**という症状が
    そのまま発生する。誘導標識を置いた場合は `find_route(..., coupling=true)` /
    `is_coupling_target()` が相手のタイルまで経路を引くため問題にならない
    (= 本家コミット件名の "non-using guide signal")。
- **取り込み時の注意**:
  - 連結 (coupling) 機能自体は v58_3 時点から存在する。セーブ形式への影響なし。
  - **FEAT-15 (道路の連結) には依存しない**。変更対象は `rail_vehicle_t` のみで、
    軌道系の連結・誘導標識はどちらも v58_3 時点から存在する。
  - **`develop-kasumi` にも同じバグが存在していた** (上記「詳細」の検証結果を参照)。
  - `vehicle/simvehicle.cc` の `can_enter_tile()` / `block_reserver()` を触るため、
    同領域を変更する FEAT-15 / FEAT-20 との取り込み順に注意。
  - **競合とその解決 (2026-09-20)**: cherry-pick 時の競合は `block_reserver()` 末尾の1箇所のみ。
    本家は本コミットで `dbg->message("rail_vehicle_t::block_reserver()","we reserve to %i",i)` を
    削除しているが `develop-kasumi` にはこの行が無いため差分が衝突した。
    解決は本家側をそのまま採用 (`set_next_reservation_index()` の `!coupling_found` 条件化 +
    末尾での `check_platform_coupling()` 呼び出し)。他のハンクは自動マージできた。
    結果として差分は本家コミットと同一 (削除済みの `dbg->message` 1行を除く)。
- **develop-kasumi 側コミット**: `72f659731`
- **備考**: 状態確認用の識別子は `vehicle/simvehicle.h` の `check_platform_coupling`。
