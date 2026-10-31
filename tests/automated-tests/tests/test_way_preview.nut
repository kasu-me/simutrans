//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// Tests for the previews of way building (command_x.calc_way, calc_route_on_ways)
// and for building way objects on given tiles (command_x.build_wayobj_on_route)
//


/// removes the way objects and ways of @p wt on the tiles from (x0, y0) to (x1, y1)
function way_preview_clear_area(pl, wt, x0, y0, x1, y1)
{
	for (local x = x0; x <= x1; x++) {
		for (local y = y0; y <= y1; y++) {
			local pos = coord3d(x, y, 0)
			if (tile_x(x, y, 0).find_object(mo_wayobj)) {
				ASSERT_EQUAL(command_x(tool_remove_wayobj).work(pl, pos, pos, "" + wt), null)
			}
			if (tile_x(x, y, 0).get_way(wt)) {
				ASSERT_EQUAL(command_x(tool_remove_way).work(pl, pos, pos, "" + wt), null)
			}
		}
	}
}


function test_way_preview_calc_way_matches_build()
{
	local pl = player_x(0)
	local public_pl = player_x(1)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local tree_desc = tree_desc_x("Ahorn-1")
	local remover = command_x(tool_remove_way)

	// a road across the future track (crossing), trees on two tiles
	ASSERT_EQUAL(command_x.build_way(public_pl, coord3d(5, 2, 0), coord3d(5, 6, 0), road, true), null)
	ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(3, 4, 0), "00," + tree_desc.get_name()), null)
	ASSERT_EQUAL(command_x(tool_plant_tree).work(public_pl, coord3d(7, 4, 0), "00," + tree_desc.get_name()), null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_tree) != null)

	local cash = pl.get_current_cash()
	local r = command_x.calc_way(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), rail, true, {})

	// nothing changed
	ASSERT_EQUAL(pl.get_current_cash(), cash)
	ASSERT_TRUE(tile_x(2, 4, 0).find_object(mo_way) == null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_tree) != null)
	ASSERT_EQUAL(tile_x(5, 4, 0).find_object(mo_crossing), null)

	ASSERT_EQUAL(r.ok, true)
	ASSERT_EQUAL(r.error, null)
	ASSERT_EQUAL(r.aborted, false)
	ASSERT_EQUAL(r.length, 7)
	ASSERT_EQUAL(r.tiles.len(), 7)
	ASSERT_EQUAL(r.bridges.len(), 0)
	ASSERT_EQUAL(r.tunnels.len(), 0)
	// from start to end, flags: 1 new, 8 crossing, 32 trees
	foreach (i, t in r.tiles) {
		ASSERT_EQUAL(t[0], 2 + i)
		ASSERT_EQUAL(t[1], 4)
		ASSERT_EQUAL(t[2], 0)
		ASSERT_EQUAL(t[3], i == 0 ? dir.east : (i == 6 ? dir.west : dir.eastwest))
		ASSERT_EQUAL(t[4], 1 | (i == 3 ? 8 : 0) | (i == 1  ||  i == 5 ? 32 : 0))
	}
	ASSERT_EQUAL(r.breakdown.fresh, 7 * rail.get_cost())
	ASSERT_TRUE(r.breakdown.trees > 0)
	ASSERT_EQUAL(r.cost, r.breakdown.fresh + r.breakdown.replace + r.breakdown.trees + r.breakdown.groundobj)

	// the same as built
	ASSERT_EQUAL(command_x.build_road(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), rail, true, {}), null)
	ASSERT_EQUAL(((cash - pl.get_current_cash()) * 100 + 0.5).tointeger(), r.cost)
	foreach (t in r.tiles) {
		ASSERT_EQUAL(tile_x(t[0], t[1], t[2]).get_way_dirs(wt_rail), t[3])
	}
	ASSERT_TRUE(tile_x(5, 4, 0).find_object(mo_crossing) != null)
	ASSERT_TRUE(tile_x(3, 4, 0).find_object(mo_tree) == null)

	// again: the track is kept
	r = command_x.calc_way(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), rail, false, {})
	ASSERT_EQUAL(r.ok, true)
	foreach (t in r.tiles) {
		ASSERT_EQUAL(t[4] & 7, 4)
	}
	ASSERT_EQUAL(r.cost, 0)

	// no way to build: the error of build_road
	r = command_x.calc_way(pl, coord3d(2, 4, 0), coord3d(2, 4, 0), rail, false, {})
	ASSERT_EQUAL(r.ok, false)
	ASSERT_EQUAL(r.error, command_x.build_road(pl, coord3d(2, 4, 0), coord3d(2, 4, 0), rail, false, {}))
	ASSERT_EQUAL(r.length, 0)

	// the search runs out of steps
	r = command_x.calc_way(pl, coord3d(2, 8, 0), coord3d(12, 12, 0), rail, false, { max_steps = 3 })
	ASSERT_EQUAL(r.ok, false)
	ASSERT_EQUAL(r.aborted, true)
	r = command_x.calc_way(pl, coord3d(2, 8, 0), coord3d(12, 12, 0), rail, false, {})
	ASSERT_EQUAL(r.ok, true)
	ASSERT_EQUAL(r.aborted, false)

	// clean up
	ASSERT_EQUAL(remover.work(pl, coord3d(2, 4, 0), coord3d(8, 4, 0), "" + wt_rail), null)
	ASSERT_EQUAL(remover.work(public_pl, coord3d(5, 2, 0), coord3d(5, 6, 0), "" + wt_road), null)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_preview_calc_route_on_ways()
{
	local pl = player_x(0)
	local road = way_desc_x.get_available_ways(wt_road, st_flat)[0]
	local sign = sign_desc_x.get_available_signs(wt_road).filter(@(idx, sign) sign.is_one_way())[0]

	// a straight road and a loop beside it: (2,4)-(10,4), (3,4)-(3,6)-(8,6)-(8,4)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(10, 4, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 4, 0), coord3d(3, 6, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 6, 0), coord3d(8, 6, 0), road, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(8, 6, 0), coord3d(8, 4, 0), road, true), null)

	local r = command_x.calc_route_on_ways(pl, coord3d(2, 4, 0), coord3d(10, 4, 0), wt_road, 0)
	ASSERT_EQUAL(r.ok, true)
	ASSERT_EQUAL(r.tiles.len(), 9)
	ASSERT_EQUAL(r.tiles[0][3], dir.east)
	ASSERT_EQUAL(r.tiles[4][3], dir.eastwest)

	// a one-way sign on (5,4): one direction goes around the loop
	ASSERT_EQUAL(command_x.build_sign_at(pl, coord3d(5, 4, 0), sign), null)
	local masked = tile_x(5, 4, 0).get_way_dirs_masked(wt_road)
	ASSERT_TRUE(masked == dir.east  ||  masked == dir.west)
	local from = masked == dir.east ? coord3d(10, 4, 0) : coord3d(2, 4, 0)
	local to   = masked == dir.east ? coord3d(2, 4, 0)  : coord3d(10, 4, 0)
	r = command_x.calc_route_on_ways(pl, from, to, wt_road, 0)
	ASSERT_EQUAL(r.ok, true)
	ASSERT_EQUAL(r.tiles.len(), 13)
	// without the masks too: route_t never enters a tile against its one-way sign
	r = command_x.calc_route_on_ways(pl, from, to, wt_road, 1)
	ASSERT_EQUAL(r.ok, true)
	ASSERT_EQUAL(r.tiles.len(), 13)
	ASSERT_EQUAL(r.blocked_by_masks, false)

	// neighbours: the step against the sign goes around, the signal tool refuses that
	local n1 = masked == dir.east ? coord3d(6, 4, 0) : coord3d(4, 4, 0)
	r = command_x.calc_route_on_ways(pl, n1, coord3d(5, 4, 0), wt_road, 0)
	ASSERT_EQUAL(r.ok, true)
	ASSERT_TRUE(r.tiles.len() > 2)
	r = command_x.calc_route_on_ways(pl, n1, coord3d(5, 4, 0), wt_road, 2)
	ASSERT_EQUAL(r.ok, false)
	ASSERT_EQUAL(r.tiles.len(), 0)

	// not connected
	r = command_x.calc_route_on_ways(pl, coord3d(2, 4, 0), coord3d(2, 9, 0), wt_road, 0)
	ASSERT_EQUAL(r.ok, false)
	ASSERT_EQUAL(r.error, "Ways not connected")

	// clean up
	ASSERT_EQUAL(command_x(tool_remover).work(pl, coord3d(5, 4, 0)), null)
	way_preview_clear_area(pl, wt_road, 2, 4, 10, 6)
	RESET_ALL_PLAYER_FUNDS()
}


function test_way_preview_wayobj_on_route()
{
	local pl = player_x(0)
	local rail = way_desc_x.get_available_ways(wt_rail, st_flat)[0]
	local overhead_line = wayobj_desc_x.get_available_wayobjs(wt_rail).filter(@(idx, wobj) wobj.is_overhead_line())[0]

	// a straight track and a loop beside it: build_wayobj takes the straight track
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(2, 4, 0), coord3d(10, 4, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 4, 0), coord3d(3, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(3, 6, 0), coord3d(8, 6, 0), rail, true), null)
	ASSERT_EQUAL(command_x.build_way(pl, coord3d(8, 6, 0), coord3d(8, 4, 0), rail, true), null)

	local tiles = [[2, 4, 0], [3, 4, 0], [3, 5, 0], [3, 6, 0], [4, 6, 0], [5, 6, 0], [6, 6, 0], [7, 6, 0], [8, 6, 0], [8, 5, 0], [8, 4, 0], [9, 4, 0]]

	// refused: not neighbours, no track, empty
	foreach (bad in [ [[2, 4, 0], [4, 4, 0]], [[2, 4, 0], [2, 5, 0]], [], [[2, 4]] ]) {
		local error = null
		try {
			command_x.build_wayobj_on_route(pl, bad, overhead_line, 0)
		}
		catch (e) {
			error = e
		}
		ASSERT_TRUE(error != null)
	}
	ASSERT_EQUAL(tile_x(2, 4, 0).find_object(mo_wayobj), null)

	local cash = pl.get_current_cash()
	ASSERT_EQUAL(command_x.build_wayobj_on_route(pl, tiles, overhead_line, 0), null)
	ASSERT_TRUE(pl.get_current_cash() < cash)
	foreach (t in tiles) {
		ASSERT_TRUE(tile_x(t[0], t[1], t[2]).find_object(mo_wayobj) != null)
	}
	// not on the straight part between the ends of the loop
	for (local x = 4; x <= 7; x++) {
		ASSERT_EQUAL(tile_x(x, 4, 0).find_object(mo_wayobj), null)
	}

	// clean up
	way_preview_clear_area(pl, wt_rail, 2, 4, 10, 6)
	RESET_ALL_PLAYER_FUNDS()
}
