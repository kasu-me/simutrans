//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for the building commands of the web viewer: bridges and tunnels with settings and their
// previews (calc_bridge, calc_tunnel), way settings, signals on routes, spacing of way objects,
// previews of removing (calc_remove), and creating / removing players with exec
//


/// the bridge object at x, y and exactly height z, null if there is none
function web_build_bridge_at(x, y, z)
{
	local t = square_x(x, y).get_tile_at_height(z)
	return t ? t.find_object(mo_bridge) : null
}

/// the error raised by @p f (a function without arguments), null if there is none
function web_build_raised(f)
{
	try {
		f()
	}
	catch (e) {
		return e
	}
	return null
}

/// 1/100 credits spent by @p pl since @p cash
function web_build_spent(pl, cash)
{
	return ((cash - pl.get_current_cash()) * 100 + 0.5).tointeger()
}


function test_web_build_bridge_high_and_preview()
{
	local pl = player_x(0)
	local remover = command_x(tool_remove_way)
	local start = coord3d(3, 5, 0), end = coord3d(3, 2, 0)

	// a road bridge that can be built at two heights
	local bridge = null, low = null, high = null
	foreach (b in bridge_desc_x.get_available_bridges(wt_road)) {
		local l = command_x.calc_bridge(pl, start, end, b, false)
		local h = command_x.calc_bridge(pl, start, end, b, true)
		if (l.ok  &&  h.ok  &&  (bridge == null  ||  h.height > l.height)) {
			bridge = b; low = l; high = h
		}
	}
	ASSERT_TRUE(bridge != null)
	ASSERT_EQUAL(low.end[0], 3)
	ASSERT_EQUAL(low.end[1], 2)
	ASSERT_EQUAL(low.length, 4)
	ASSERT_TRUE(high.height >= low.height)
	// nothing was built
	ASSERT_EQUAL(web_build_bridge_at(3, 5, 0), null)

	// low
	local cash = pl.get_current_cash()
	ASSERT_EQUAL(command_x.build_bridge(pl, start, end, bridge, false), null)
	ASSERT_EQUAL(web_build_spent(pl, cash), low.cost)
	ASSERT_TRUE(web_build_bridge_at(3, 4, low.height) != null)
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 6, 0), coord3d(3, 1, 0), "" + wt_road), null)
	ASSERT_EQUAL(web_build_bridge_at(3, 5, 0), null)

	// high, and the same without the argument (as before)
	foreach (args in [true, {high = true}, null]) {
		cash = pl.get_current_cash()
		local err = args == null ? command_x.build_bridge(pl, start, end, bridge) : command_x.build_bridge(pl, start, end, bridge, args)
		ASSERT_EQUAL(err, null)
		ASSERT_EQUAL(web_build_spent(pl, cash), high.cost)
		ASSERT_TRUE(web_build_bridge_at(3, 4, high.height) != null)
		ASSERT_EQUAL(remover.work(pl, coord3d(3, 6, 0), coord3d(3, 1, 0), "" + wt_road), null)
	}

	// (the game connects the ends with a way on the next tiles)
	ASSERT_EQUAL(tile_x(3, 6, 0).find_object(mo_way), null)

	// one click: the preview finds the end itself
	ASSERT_EQUAL(command_x.calc_bridge(pl, coord3d(5, 5, 0), bridge).ok, false)

	// bad arguments
	ASSERT_EQUAL(command_x.calc_bridge(pl, start, end, bridge, {overtaking_mode = 7}).error, "invalid: the overtaking mode is -1 to 4")
	ASSERT_EQUAL(command_x.calc_bridge(pl, start, end, bridge, "x").error, "invalid: the arguments are a table, a bool (high) or null")

	// (no RESET_ALL_PLAYER_FUNDS: removing the ramps leaves some maintenance, as in test_way_bridge)

}


function test_web_build_bridge_oneway()
{
	local pl = player_x(0)
	local remover = command_x(tool_remove_way)
	local bridge = bridge_desc_x.get_available_bridges(wt_road)[0]
	local start = coord3d(3, 5, 0), end = coord3d(3, 2, 0)

	local r = command_x.calc_bridge(pl, start, end, bridge, {overtaking_mode = 0})
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(command_x.build_bridge(pl, start, end, bridge, {overtaking_mode = 0, street_flag = 0}), null)
	// the ramps and the tiles between
	foreach (y in [5, 4, 3, 2]) {
		local z = (y == 5  ||  y == 2) ? 0 : r.height
		local t = square_x(3, y).get_tile_at_height(z)
		ASSERT_TRUE(t != null)
		ASSERT_EQUAL(t.find_object(mo_way).get_overtaking_mode(), 0)
	}
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 6, 0), coord3d(3, 1, 0), "" + wt_road), null)

	// two-way (default)
	ASSERT_EQUAL(command_x.build_bridge(pl, start, end, bridge), null)
	ASSERT_EQUAL(square_x(3, 4).get_tile_at_height(r.height).find_object(mo_way).get_overtaking_mode(), 1)
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 6, 0), coord3d(3, 1, 0), "" + wt_road), null)

	// (no RESET_ALL_PLAYER_FUNDS: removing the ramps leaves some maintenance, as in test_way_bridge)

}


function test_web_build_tunnel()
{
	local pl = player_x(0)
	local remover = command_x(tool_remover)
	local tunnel = tunnel_desc_x.get_available_tunnels(wt_road)[0]

	// a hill (as test_way_tunnel_build_straight)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(3, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 3, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(3, 2, 0)), null)
	ASSERT_EQUAL(command_x.grid_raise(pl, coord3d(4, 2, 0)), null)

	// flat ground: an error, nothing built
	local cash = pl.get_current_cash()
	local r = command_x.calc_tunnel(pl, coord3d(7, 7, 0), tunnel)
	ASSERT_EQUAL(r.ok, false)
	ASSERT_TRUE(r.error != null)
	ASSERT_EQUAL(command_x.build_tunnel(pl, coord3d(7, 7, 0), tunnel), "Tunnel muss an\neinfachem\nHang beginnen!\n")
	ASSERT_EQUAL(pl.get_current_cash(), cash)

	// through the hill, one-way
	r = command_x.calc_tunnel(pl, coord3d(3, 1, 0), tunnel)
	ASSERT_EQUAL(r.error, null)
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(r.end[0], 3)
	ASSERT_EQUAL(r.end[1], 3)
	ASSERT_EQUAL(r.length, 3)
	ASSERT_EQUAL(command_x.build_tunnel(pl, coord3d(3, 1, 0), tunnel, {overtaking_mode = 0}), null)
	ASSERT_EQUAL(web_build_spent(pl, cash), r.cost)
	ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_tunnel) != null, true)
	ASSERT_EQUAL(tile_x(3, 1, 0).find_object(mo_way).get_overtaking_mode(), 0)
	ASSERT_EQUAL(tile_x(3, 2, 0).find_object(mo_way).get_overtaking_mode(), 0)
	ASSERT_EQUAL(tile_x(3, 3, 0).find_object(mo_way).get_overtaking_mode(), 0)

	// there is a tunnel now: no second one (a tunnel tile is the start of two clicks)
	ASSERT_EQUAL(web_build_raised(@() command_x.build_tunnel(pl, coord3d(3, 1, 0), tunnel)), "Tool has no effects")
	ASSERT_EQUAL(command_x.calc_tunnel(pl, coord3d(3, 1, 0), tunnel).ok, false)

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(3, 1, 0), coord3d(3, 3, 0), "" + wt_road), null)
	ASSERT_EQUAL(square_x(3, 2).get_tile_at_height(0), null)

	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_way_settings()
{
	local pl = player_x(0)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local remover = command_x(tool_remove_way)

	ASSERT_EQUAL(command_x.build_road(pl, coord3d(2, 4, 0), coord3d(6, 4, 0), road, true, {}), null)
	// one-way from west to east
	ASSERT_EQUAL(command_x.change_way_settings(pl, coord3d(2, 4, 0), coord3d(6, 4, 0), 0, 0), null)
	for (local x = 2; x <= 6; x++) {
		local w = tile_x(x, 4, 0).find_object(mo_way)
		ASSERT_EQUAL(w.get_overtaking_mode(), 0)
		// the dead end at the east end cannot be left
		ASSERT_EQUAL(w.get_oneway_ribi(), x < 6 ? dir.east : 0)
		ASSERT_EQUAL(w.get_oneway_mask(), x > 2 ? dir.west : 0)
	}
	// two-way again
	ASSERT_EQUAL(command_x.change_way_settings(pl, coord3d(2, 4, 0), coord3d(6, 4, 0), 1), null)
	for (local x = 2; x <= 6; x++) {
		local w = tile_x(x, 4, 0).find_object(mo_way)
		ASSERT_EQUAL(w.get_overtaking_mode(), 1)
		ASSERT_EQUAL(w.get_oneway_ribi(), 0)
	}
	ASSERT_EQUAL(web_build_raised(@() command_x.change_way_settings(pl, coord3d(2, 4, 0), coord3d(6, 4, 0), 5)), "invalid: the overtaking mode is -1 to 4")

	ASSERT_EQUAL(remover.work(pl, coord3d(2, 4, 0), coord3d(6, 4, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_signals_on_route()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]
	local remover = command_x(tool_remove_way)
	local start = coord3d(2, 4, 0), end = coord3d(11, 4, 0)

	// (no signals at dead ends)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 4, 0), coord3d(12, 4, 0), rail, true), null)

	// every third tile: 4 signals on 10 tiles
	local r = command_x.calc_signs_on_route(pl, start, end, signal, {spacing = 3})
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(r.tiles.len(), 4)
	ASSERT_EQUAL(r.remove.len(), 0)
	foreach (i, t in r.tiles) {
		ASSERT_EQUAL(t[0], 2 + 3 * i)
	}
	// nothing built
	ASSERT_EQUAL(tile_x(2, 4, 0).find_object(mo_signal), null)

	ASSERT_EQUAL(command_x.build_signs_on_route(pl, start, end, signal, {spacing = 3}), null)
	for (local x = 2; x <= 11; x++) {
		local s = tile_x(x, 4, 0).find_object(mo_signal)
		ASSERT_EQUAL(s != null, (x - 2) % 3 == 0)
	}
	foreach (t in r.tiles) {
		ASSERT_EQUAL(tile_x(t[0], t[1], t[2]).find_object(mo_signal).get_dir(), t[3])
	}

	// a signal between, kept or removed
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(3, 4, 0), signal), null)
	r = command_x.calc_signs_on_route(pl, start, end, signal, {spacing = 3, remove_intermediate = false})
	ASSERT_EQUAL(r.remove.len(), 0)
	ASSERT_EQUAL(command_x.build_signs_on_route(pl, start, end, signal, {spacing = 3, remove_intermediate = false}), null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_signal) != null)
	r = command_x.calc_signs_on_route(pl, start, end, signal, {spacing = 3})
	ASSERT_EQUAL(r.remove.len(), 1)
	ASSERT_EQUAL(r.remove[0][0], 3)
	ASSERT_EQUAL(command_x.build_signs_on_route(pl, start, end, signal, {spacing = 3}), null)
	ASSERT_EQUAL(tile_x(3, 4, 0).find_object(mo_signal), null)

	// one signal in a given direction, whatever it was before
	foreach (d in [dir.east, dir.west, dir.east, dir.eastwest]) {
		ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(4, 4, 0), signal, d), null)
		ASSERT_EQUAL(tile_x(4, 4, 0).find_object(mo_signal).get_dir(), d)
	}
	ASSERT_EQUAL(web_build_raised(@() command_x.build_sign_at(pl, coord3d(4, 4, 0), signal, dir.north)), "invalid: the sign cannot have this direction here")

	ASSERT_EQUAL(command_x.calc_signs_on_route(pl, start, end, signal, {spacing = 0}).error, "invalid: the spacing is 1 to 127")

	ASSERT_EQUAL(remover.work(pl, coord3d(1, 4, 0), coord3d(12, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_wayobj_spacing()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local overhead_line = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())[0]
	local remover = command_x(tool_remove_way)
	local start = coord3d(2, 4, 0), end = coord3d(11, 4, 0)

	ASSERT_EQUAL(command_x.build_way(pl, start, end, rail, true), null)
	ASSERT_EQUAL(command_x.build_wayobj(pl, start, end, overhead_line, 2), null)
	for (local x = 2; x <= 11; x++) {
		ASSERT_EQUAL(tile_x(x, 4, 0).find_object(mo_wayobj) != null, x % 2 == 0)
	}
	ASSERT_EQUAL(command_x(tool_remove_wayobj).work(pl, start, end, "" + wt_rail), null)

	// on the given tiles
	local tiles = []
	for (local x = 2; x <= 11; x++) {
		tiles.append([x, 4, 0])
	}
	ASSERT_EQUAL(command_x.build_wayobj_on_route(pl, tiles, overhead_line, 0, 3), null)
	for (local x = 2; x <= 11; x++) {
		ASSERT_EQUAL(tile_x(x, 4, 0).find_object(mo_wayobj) != null, (x - 2) % 3 == 0)
	}
	ASSERT_EQUAL(web_build_raised(@() command_x.build_wayobj(pl, start, end, overhead_line, 51)), "invalid: the spacing is 1 to 50")

	ASSERT_EQUAL(remover.work(pl, start, end, "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_calc_remove()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local remover = command_x(tool_remove_way)

	ASSERT_EQUAL(command_x.build_road(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), road, true, {}), null)
	local r = command_x.calc_remove(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "way", wt_road)
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(r.tiles.len(), 7)
	ASSERT_EQUAL(r.refused.len(), 0)
	foreach (t in r.tiles) {
		ASSERT_EQUAL(t[3], 0)
	}
	// part of it: the ends keep their other directions
	r = command_x.calc_remove(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "way", wt_road)
	ASSERT_EQUAL(r.tiles.len(), 3)
	ASSERT_EQUAL(r.tiles[0][3], dir.west)
	ASSERT_EQUAL(r.tiles[1][3], 0)
	ASSERT_EQUAL(r.tiles[2][3], dir.east)
	ASSERT_EQUAL(remover.work(pl, coord3d(3, 4, 0), coord3d(5, 4, 0), "" + wt_road), null)
	ASSERT_EQUAL(tile_x(4, 4, 0).find_object(mo_way), null)
	ASSERT_EQUAL(tile_x(3, 4, 0).get_way_dirs(wt_road), dir.west)
	ASSERT_EQUAL(tile_x(5, 4, 0).get_way_dirs(wt_road), dir.east)

	// another player's road: refused (and then nothing is removed; the public player's ways can be removed by all)
	ASSERT_TRUE(world.create_player(2, 1))
	local other = player_x(2)
	ASSERT_EQUAL(command_x.build_road(other, coord3d(3, 4, 0), coord3d(5, 4, 0), road, true, {}), null)
	r = command_x.calc_remove(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "way", wt_road)
	ASSERT_EQUAL(r.ok, false)
	ASSERT_TRUE(r.refused.len() > 0)
	ASSERT_TRUE(r.error != null)
	ASSERT_TRUE(remover.work(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "" + wt_road) != null)
	ASSERT_TRUE(tile_x(2, 4, 0).find_object(mo_way) != null)

	ASSERT_EQUAL(command_x.calc_remove(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "x", wt_road).error, "invalid: kind is way, wayobj or signal")

	ASSERT_EQUAL(remover.work(public_pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "" + wt_road), null)
	ASSERT_TRUE(world.remove_player(other))
	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_calc_remove_signals()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local signal = sign_desc_x.get_available_signs(wt_rail).filter(@(idx, sign) sign.is_signal())[0]
	local overhead_line = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())[0]
	local start = coord3d(2, 4, 0), end = coord3d(11, 4, 0)

	ASSERT_EQUAL(command_x.build_way(pl, coord3d(1, 4, 0), coord3d(12, 4, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_signs_on_route(pl, start, end, signal, {spacing = 3}), null)
	ASSERT_EQUAL(command_x.build_wayobj(pl, start, end, overhead_line), null)

	local r = command_x.calc_remove(pl, start, end, "signal", wt_rail)
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(r.tiles.len(), 4)
	ASSERT_EQUAL(command_x(tool_remove_signal).work(pl, start, end, ""), null)
	foreach (t in r.tiles) {
		ASSERT_EQUAL(tile_x(t[0], t[1], t[2]).find_object(mo_signal), null)
	}

	r = command_x.calc_remove(pl, start, end, "wayobj", wt_rail)
	ASSERT_TRUE(r.ok)
	ASSERT_EQUAL(r.tiles.len(), 10)
	ASSERT_EQUAL(command_x(tool_remove_wayobj).work(pl, start, end, "" + wt_rail), null)
	ASSERT_EQUAL(tile_x(5, 4, 0).find_object(mo_wayobj), null)

	ASSERT_EQUAL(command_x(tool_remove_way).work(pl, coord3d(1, 4, 0), coord3d(12, 4, 0), "" + wt_rail), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_web_build_exec_players()
{
	local public_pl = player_x(1)
	local cmd = command_x(tool_change_player)
	local p = player_x(6)
	ASSERT_FALSE(p.is_valid())

	// by the public player only
	ASSERT_EQUAL(web_build_raised(@() cmd.exec(player_x(0), "n,6,1")), "permission: only the public player can create players")
	ASSERT_FALSE(p.is_valid())
	ASSERT_EQUAL(cmd.exec(public_pl, "n,6,1"), true)
	ASSERT_TRUE(p.is_valid())
	ASSERT_EQUAL(p.get_type(), 1)

	// it builds something, removing the player removes it (roads go to the public player)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	ASSERT_EQUAL(command_x.build_way(p, coord3d(2, 4, 0), coord3d(5, 4, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_road(p, coord3d(2, 6, 0), coord3d(5, 6, 0), road, true, {}), null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_way) != null)
	ASSERT_EQUAL(cmd.exec(public_pl, "d,6"), true)
	ASSERT_FALSE(p.is_valid())
	ASSERT_EQUAL(tile_x(3, 4, 0).find_object(mo_way), null)
	ASSERT_EQUAL(tile_x(3, 6, 0).find_object(mo_way).get_owner().get_name(), public_pl.get_name())
	ASSERT_EQUAL(command_x(tool_remove_way).work(public_pl, coord3d(2, 6, 0), coord3d(5, 6, 0), "" + wt_road), null)

	// the checks
	foreach (param in ["n,1,1", "n,0,1", "n,15,1", "n,6,0", "n,6,5", "d,1", "d,6", "n,6"]) {
		ASSERT_TRUE(web_build_raised(@() cmd.exec(public_pl, param)) != null)
	}
	ASSERT_FALSE(p.is_valid())
}
