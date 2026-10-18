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

### FIX-10: 道路が無いタイルでの道路参照による異常終了と、連結/解放時の `route_index` 不整合

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 「そのタイルには必ず道路がある」前提で `get_weg(road_wt)` の結果を即座に参照していた箇所を
  NULL 安全にし、併せて連結・解放の境界で各車両の `route_index` を自編成の経路に合わせて付け直す。
- **上流コミット**:
  - `280ced3e8` BUG FIX: fix crash bug of v62 (#747) — **コミットの一部のみ** (残りは FEAT-16)
- **主な変更箇所**:
  - `boden/wege/strasse.{h,cc}` — フリー関数 `strasse_t *strasse_at(const koord3d &pos)` を新設
    (タイルが無い/道路が無い場合は NULL)
  - `simconvoi.cc` — `step()` / `vorfahren()` / `can_overtake()` の道路参照を `strasse_at()` に置き換え、
    NULL のときは車線変更・追い越し判定を行わない
  - `vehicle/simvehicle.cc` — `road_vehicle_t::calc_disp_lane()` / `is_target()` / `can_enter_tile()` の
    道路参照を NULL 安全に変更
  - `vehicle/simroadtraffic.cc` — `private_car_t::ist_weg_frei()` を `strasse_at()` に置き換え。
    `enter_tile()` で道路が無ければ `time_to_life = 0` にして即座に消滅させる
  - `vehicle/simvehicle.{h,cc}` — `vehicle_t::reanchor_route_index()` を新設。
    車両が実際に立っているタイルを自編成の経路上で (旧 `route_index` 付近から外側へ) 探し直し、`route_index` を付け直す
  - `simconvoi.{h,cc}` — `convoi_t::reanchor_chain_route_indices()` を新設し、
    `couple_convoi()` / `couple_convoi_during_running()` / `uncouple_convoi()` で呼ぶ
  - `vehicle/simvehicle.cc` — `rail_vehicle_t::leave_tile()` で同一タイル上の他編成の向きを求める際、
    他編成の経路インデックスを経路長でクランプする (空経路での `get_count()-1u` のラップアラウンドも回避)
- **詳細**:
  - 道路車両の編成が道路の無いタイルを報告するケースとして、航送中 (キャリア = 船のタイル, FEAT-18)、
    車庫内 (所属車庫の位置)、車両の無い編成 (`koord3d::invalid`)、旧セーブのロード後に way が欠けている場合、
    停車中の車両の下の way が撤去された場合が挙げられている。従来はいずれも
    `strasse_t::get_overtaking_mode()` の NULL 参照で異常終了していた。
  - 連結中の子編成は自分の経路のコピーを持つが実際は親編成に牽引されるため、`vehicle_t::hop()` で
    `route_index` が自分の経路長を超えて増え続ける。連結/解放時にその値のまま別編成の経路を引くと
    範囲外参照になるため、境界で付け直す。
  - 修正対象のコードの多くは v58_3 時点から存在するが、本家の件名 (crash bug of v62) のとおり
    v62 までに入った機能 (航送 FEAT-18、道路連結 FEAT-15 等) によって顕在化した。
- **取り込み時の注意**:
  - `simconvoi.cc` の `step()` 内の修正箇所は、周辺の行に FEAT-23 の `get_overtaking_mode_raw()` /
    `passing_lane_stop_only_mode` と FEAT-08 の `reversing_lane_hold` を含むため、
    **cherry-pick では競合する**。`develop-kasumi` に取り込む場合は該当箇所を手作業で移植すること。
  - `280ced3e8` は FEAT-16 と同一コミット。FEAT-16 を取り込まずに本項目だけ取り込む場合は、
    `dataobj/ribi.h` / `boden/grund.cc` / `boden/wege/weg.cc` / `obj/crossing.cc` / `obj/roadsign.*` /
    `simtool.cc` / テストの変更を除外する。
  - **移植内容 (2026-09-28)**: FEAT-16 の分を除外し、手作業で移植した。
    本家の差分のうち `develop-kasumi` に該当コードが無い次の2か所は移植対象外:
    `convoi_t::vorfahren()` の逆走発車時の車線設定 (FEAT-08 の `reversing_lane_hold` 周辺) と、
    `step()` の条件式中の `get_overtaking_mode_raw()!=passing_lane_stop_only_mode` (FEAT-23)。
    `step()` の2か所は `develop-kasumi` の既存条件式に `str0 &&` と `strasse_at()` を適用する形にした。
    `road_vehicle_t::set_sideways_image()` と `rail_vehicle_t::leave_tile()` の修正は本家と同一。
    `strasse_at()` のコメントからは航送 (FEAT-18) への言及を除いた。
    FEAT-08 / FEAT-23 を後から取り込む場合は、該当行を `strasse_at()` 版に合わせて移植すること。
  - 取り込み後に `test_schedule_entry_length_coupling_done`、`test_sign_build_oneway`、
    `test_road_choose_*` (6件)、`test_reservation_clear_road`、`test_transport_*` (4件) の通過を確認。
    異常終了の再現シナリオ自体の自動テストは無い。
- **develop-kasumi 側コミット**: `65ed5e40d`
- **備考**: 関連項目 FEAT-15 / FEAT-16 / FEAT-18。状態確認用の識別子は `boden/wege/strasse.h` の `strasse_at`、
  または `vehicle/simvehicle.h` の `reanchor_route_index`。

### FIX-11: 一方通行詳細設定の標識がある道路で、一方通行モード (`oneway_mode`) の進行方向制限が経路判定に効かない

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 一方通行の詳細設定 (`detailed_oneway`) を持つ標識が置かれた道路タイルでは、
  `vehicle_t::get_ribi(gr, from_dir)` が標識の方向マスクだけを適用し、
  一方通行モードの道路が持つ `ribi_mask_oneway` を適用していなかったのを修正する。
- **上流コミット**:
  - `891191546` add grund_t::has_two_same_waytype_ways() and fix vehicle_t::get_ribi() (#751) — **コミットの一部のみ** (残りは FEAT-16)
- **主な変更箇所**:
  - `boden/wege/strasse.h` — `strasse_t::get_active_ribi_mask_oneway()` を新設
    (`overtaking_mode<=oneway_mode` のとき `ribi_mask_oneway`、それ以外は `ribi_t::none`)
  - `boden/wege/strasse.cc` — `strasse_t::get_ribi()` を上記ヘルパーで書き直し (挙動同一)
  - `vehicle/simvehicle.cc` — `vehicle_t::get_ribi(gr, from_dir)` の詳細一方通行分岐で、
    非マスク ribi に標識の `get_detailed_oneway_out_ribi()` を掛けた後、
    way が道路なら `~get_active_ribi_mask_oneway()` も掛ける
- **詳細**: 詳細一方通行の標識は `ribi_maske` (通常の一方通行標識のマスク) を進入方向ごとのマスクで
  置き換えるため、非マスクの ribi から計算し直している。そのとき、標識とは無関係な
  道路自体の一方通行モードのマスクまで一緒に落としていたため、一方通行モードの道路に
  詳細一方通行標識を置くと、そのタイルでは道路の一方通行方向が経路判定に反映されなかった。
  該当コードは v58_3 時点から存在し、`develop-kasumi` (FIX-03 取り込み済み) の
  `vehicle/simvehicle.cc` にも同じ形で残っている。
- **取り込み時の注意**:
  - `891191546` は FEAT-16 と同一コミット。本項目だけを取り込む場合は、
    `strasse.{h,cc}` の変更と `vehicle_t::get_ribi(gr, from_dir)` の末尾に
    `get_active_ribi_mask_oneway()` を掛ける数行のみを手作業で移植する
    (同関数の脚選択部分は FEAT-16 の `get_weg(waytype_t, ribi_t::ribi)` 等に依存する)。
  - セーブ形式への影響は無い。
  - **移植内容 (2026-09-28)**: 上記どおり `strasse.{h,cc}` の変更と、
    `vehicle_t::get_ribi(gr, from_dir)` の末尾で `gr->get_weg(get_waytype())` が道路なら
    `~get_active_ribi_mask_oneway()` を掛ける数行のみを移植した。本家版はここで FEAT-16 の脚 (`leg`) を
    優先して参照するが、`develop-kasumi` には同一 waytype の2脚が無いため `get_weg(waytype)` のみとした。
- **develop-kasumi 側コミット**: `e31fd6d88`
- **備考**: 関連項目 FEAT-16 / FIX-03。状態確認用の識別子は `boden/wege/strasse.h` の `get_active_ribi_mask_oneway`。

### FIX-12: 市街建築物を複数タイルの建物に建て替える際の異常終了

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 市街建築物の建て替え (`renovate_city_building()`) で、複数タイルの建物に置き換える際に
  他の都市に属する建物や市街建築物でないタイルを巻き込んで異常終了する問題を修正する。
- **上流コミット**:
  - `5a763f099` BUG FIX: crash when upgrade to multitile building (#756)
- **主な変更箇所**:
  - `simcity.cc` — `stadt_t::renovate_city_building()` の近傍探索で、隣の建物が市街建築物であることに加えて
    **同じ都市に属する** (`testgb->get_stadt() == this`) ことを条件にする。
    さらに置き換えの直前に、置き換える全タイルが「この都市の市街建築物」であることを確認し、
    1つでも外れたら建て替えを中止する
- **詳細**: 他都市のタイルを置き換えようとすると `update_gebaeude_from_stadt()` が失敗する
  (コード中のコメントより)。
- **取り込み時の注意**: セーブ形式への影響なし。対象のコードは v58_3 時点と同一で、そのまま cherry-pick できる見込み。
- **develop-kasumi 側コミット**: `10a37e4c1`
- **備考**: —

### FIX-13: `-load net:` で起動したときに pakset のチェックサムを照合しない

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: コマンドライン引数 `-load net:<server>` でサーバーへ直接参加する場合に、
  pakset の一致を確認せずに参加してしまう問題を修正する (参加前にチェックサムを照合する)。
- **上流コミット**:
  - `ae80b9154` BUG FIX: add pakset checksum when using -load:net (#755)
- **主な変更箇所**:
  - `dataobj/gameinfo.{h,cc}` — `network_gameinfo()` で埋める用の空の既定コンストラクタ `gameinfo_t()` を追加
  - `simmain.cc` — `check_pakset_checksum(server, msg)` を新設。`simu_main()` で `loadgame` が `net:` で始まる場合、
    `network_gameinfo()` でサーバーの pakset チェックサムを取得して自分のものと比較し、
    不一致なら `network_compare_pakset_with_server()` で差分を取得して参加を取りやめる (既定マップで起動)。
    起動後に差分を `help_frame_t` (`magic_pakset_info_t`) で表示する
- **詳細**: サーバーに問い合わせできなかった場合は照合を省略し、通常の参加処理にエラー報告を任せる。
  GUI のサーバー一覧 (`server_frame_t`) は従来から pakset チェックサムの一致を表示しており、本修正はコマンドライン経由が対象。
- **取り込み時の注意**: セーブ形式・ネットワークプロトコルへの影響なし。
  ローカルのネットワーク再現実験では `-load net:` で参加しているため、取り込み後は
  pakset が不一致だと参加せずに既定マップが立ち上がる挙動に変わる点に注意。
- **develop-kasumi 側コミット**: `6c7b58822`
- **備考**: 状態確認用の識別子は `simmain.cc` の `check_pakset_checksum`。

### FIX-14: 編成の譲渡で会社ごとの編成数統計が更新されない

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 編成を他社へ譲渡 (`trade_convoi()`) した際に、旧所有者・新所有者の編成数の統計が増減しない問題を修正する。
- **上流コミット**:
  - `d0d7ec16e` BUG FIX: convoi count when trade convoi (#760)
- **主な変更箇所**:
  - `simconvoi.cc` — `convoi_t::trade_convoi()` で、所有者変更前に `owner->book_convoi_number(-1)`、
    変更後に新所有者で `owner->book_convoi_number(1)` を呼ぶ
- **詳細**: —
- **取り込み時の注意**: セーブ形式への影響なし (財務統計の値が正しくなるだけ)。2行の追加のみ。
- **develop-kasumi 側コミット**: `a9f1ae415`
- **備考**: —

### FIX-15: 車庫の車両名フィルタ文字列のコピー長の誤り

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 車庫ウィンドウの車両名フィルタを `sizeof(ポインタ)` の長さでコピーしていたため、
  ウィンドウ状態の保存/復元 (ネットワーク再同期を含む) のたびに先頭 8 バイト程度しか残らず、
  日本語などのマルチバイト文字が途中で切れて文字化けする問題を修正する。
- **上流コミット**:
  - `9e6810f68` BUG FIX: fix depot filter (#761)
- **主な変更箇所**:
  - `gui/depot_frame.cc` — コンストラクタと `rdwr()` の `strncpy(..., sizeof(depot->get_name_filter()))` を
    `tstrncpy(..., lengthof(name_filter_value))` に置き換え。`set_text()` の長さも `lengthof(name_filter_value)` に統一。
    OTRP v51 形式の読み込み経路では `depot` が NULL の場合にコピーしない
  - `gui/simwin.cc` — `rdwr_all_win()` の車庫フィルタ保存で同様に `tstrncpy()` を使い、作業用バッファ `filter[64]` を空文字で初期化
  - `simdepot.h` — `set_name_filter()` を `tstrncpy()` 化し、NULL を空文字として扱う。`macros.h` / `utils/simstring.h` を include
- **詳細**: PR 本文の履歴には車庫ウィンドウのレイアウト変更・スクロール修正・サーバーでのエラーウィンドウ抑止が並ぶが、
  squash 後の差分は上記のフィルタ修正のみ。
- **取り込み時の注意**: セーブ形式への影響なし。
- **develop-kasumi 側コミット**: `9e7f1072c` (先行して独自修正 `f8e8a9a00`)
- **備考**: `develop-kasumi` は同じ不具合を `f8e8a9a00`「BUG FIX: depot vehicle name filter got garbled after a network resync」で
  先に独自修正していた (`depot_frame.cc` の `sizeof` → `tstrncpy(..., lengthof(...))` 3箇所、`simwin.cc`、`set_name_filter()` の終端保証)。
  残りの防御的修正 ((a) `set_text()` の長さの統一、(b) v51 経路の `depot` NULL ガード、(c) `simwin.cc` の `filter[64] = ""` 初期化、
  (d) `set_name_filter()` の NULL 引数ガード) を `9e7f1072c` で cherry-pick した。競合箇所はすべて本家側を採用しており、
  取り込み後の該当箇所は本家と同一。

### FIX-16: パスワード画面と会社ロック関連のネットワークコマンドの不具合

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 会社のパスワード/ロック操作まわりの細かな不具合をまとめて修正する
  (空き会社番号への認証要求でサーバーが異常終了しうる、「Clear Password」ボタンの状態が更新されない、等)。
- **上流コミット**:
  - `ed547bf0f` fix nettool to use players>15 (#759)
- **主な変更箇所**:
  - `network/network_cmd_ingame.cc` —
    `nwc_auth_player_t::execute()` で要求された会社を先に `welt->get_player()` で引き、存在しない番号/空き枠なら警告を出して何もしない
    (従来は NULL を逆参照していた)。ログ出力を 64 ビットのビット列表示に変更。
    `nwc_service_t::execute()` の会社ロック解除で、`unlock_player_all()` の除外対象にソケット (`packet->get_sender()`) ではなく
    クライアント ID (`sender_id`) を渡すよう修正
  - `gui/password_frame.{h,cc}` — 「Clear Password」ボタンの有効条件を `update_unlock_button()` に切り出し、
    `draw()` のたびに再評価する (サーバー応答や他クライアント・nettool による変更を、ウィンドウ表示中にも反映)。
    解除要求の応答待ち (`is_unlock_pending()`) の間はボタンを無効化し、押されても二重送信しない
  - `nettools/nettool.cc` — `lock_company` で範囲外の会社番号を指定した場合にエラーメッセージを出す
- **詳細**: PR 名は「players>15」だが、差分の内容は会社数上限 (FEAT-01) に依存しない。
  コミット本文の履歴は FEAT-01 の作業ブランチのものがそのまま残っている。
  `unlock_player_all()` の第3引数はクライアント ID (`uint32 except_client`) なので、ソケットを渡すと
  除外すべきクライアントとは別のクライアントが除外されるか、誰も除外されない。
- **取り込み時の注意**: セーブ形式・ネットワークプロトコルへの影響なし。
  対象のコードは v58_3 時点と同一 (`develop-kasumi` でも未修正) で、FEAT-01 を取り込まなくても単独で適用できる見込み。
  ログ出力の `%llx` は `player_unlocked` を `unsigned long long` にキャストしているので、
  FEAT-01 未取り込みのままでも問題ない。
- **develop-kasumi 側コミット**: `1c639142e`
- **備考**: 関連項目 FEAT-01。状態確認用の識別子は `gui/password_frame.h` の `update_unlock_button`。
  `nettools/nettool.cc` は FEAT-01 未取り込みで `lock_company()` の上限が `15 /*PLAYER_UNOWNED*/` の直書きのままのため競合した。
  上限はそのままにエラーメッセージだけを取り込んだ (表示は `15-1`)。FEAT-01 を取り込む際は `PLAYER_UNOWNED` に置き換えること。

### FIX-17: シンプルツール番号のずれ (`TOOL_HALT_PERMISSION` を enum 末尾へ移動)

- **分類**: バグ修正
- **状態**: 取り込み済み
- **概要**: 本家 #540 (`f0fac8dcf`, v58_3 より前) で停留所の権限設定ツール `TOOL_HALT_PERMISSION` が
  シンプルツール enum の途中に挿入され、後続ツールの番号が1つずつずれていたのを、末尾へ移して元の番号に戻す。
- **上流コミット**:
  - `57b53b49d` change menubar number (#764)
- **主な変更箇所**:
  - `simmenu.h` — シンプルツール enum で `TOOL_HALT_PERMISSION` を `TOOL_CHANGE_HALT` の直後から
    `TOOL_EXPORT_MINIMAP` (FEAT-24 拡張分) の後ろへ移動
- **詳細**: 番号の変化は次のとおり (`develop-kasumi` / v58_3 → 本家修正後)。
  `TOOL_HALT_PERMISSION` 47 → 54、`TOOL_CHANGE_FACTORY` 48 → 47、`TOOL_RESET_GAME_SPEED` 49 → 48、
  `TOOL_FIX_GAME_SPEED` 50 → 49、`TOOL_SHOW_WAY_OFFSET_LABEL` 51 → 50、
  `TOOL_SHOW_ONLY_OWN_VEHICLE_STATES` 52 → 51、`TOOL_FOLLOW_CONVOI_UNDERGROUND` 53 → 52、
  (新設) `TOOL_EXPORT_MINIMAP` 53。
  47〜52 は #540 以前の番号と一致する (`f0fac8dcf^` の `simmenu.h` で確認)。
  PR 本文の履歴は FEAT-24 拡張 (`43f0ce22e`) の作業ブランチのものが残っている。
- **取り込み時の注意**:
  - シンプルツールの番号は pakset の `menuconf.tab` の `simple_tool[n]` と、ネットワークで送るツール ID に使われる。
    取り込むと、**`develop-kasumi` 向けにツール番号 47〜53 を指定している `menuconf.tab` は書き換えが必要**になり、
    サーバーとクライアントのバイナリも揃える必要がある (2026-10-17 時点のテスト環境の pakset には該当番号の参照なし)。
  - FEAT-24 拡張分 (`TOOL_EXPORT_MINIMAP`) と番号が連動するため、どちらかだけを取り込む場合は番号の並びを個別に判断すること。
- **develop-kasumi 側コミット**: `11d84abb5`
- **備考**: 関連項目 FEAT-24。`develop-kasumi` には `TOOL_EXPORT_MINIMAP` が未取り込みのため、`11d84abb5` で
  53 番を未使用枠 `UNUSED_TOOL_EXPORT_MINIMAP` として確保し、本家と同じ番号 (`TOOL_HALT_PERMISSION` = 54) に揃えた。
  未使用枠は既存の `UNUSED_*` と同様に `init_menu()` で NULL のままにし、`create_simple_tool()` では警告して NULL を返す。
  あわせて `read_menu()` に、`menuconf.tab` が未使用枠 (NULL) を指定した場合に警告して無視するガードを追加した
  (従来は `create_tool` 経路で NULL を逆参照していた)。
