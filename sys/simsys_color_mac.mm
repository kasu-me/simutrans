/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Native color picker for macOS (SDL2 backend), using NSColorPanel.
 * Counterpart to the Windows ChooseColor() implementation in simsys_w.cc.
 */

#include "simsys.h"

#import <Cocoa/Cocoa.h>
#include <cmath>


// NSColorPanel has no OK/Cancel step: it is a live, non-modal panel that
// reports every change via the target/action, and the user just closes it
// when done. So "OK" here means "the panel was closed after the color was
// touched at least once"; closing without ever changing the color is
// treated the same as a Windows Cancel.
@interface SimutransColorPickerTarget : NSObject
{
@public
	uint8 r, g, b;
	bool changed;
	bool closed;
}
- (void)colorChanged:(id)sender;
- (void)panelWillClose:(NSNotification*)note;
@end

@implementation SimutransColorPickerTarget

- (void)colorChanged:(id)sender
{
	NSColor* c = [[sender color] colorUsingColorSpace: [NSColorSpace deviceRGBColorSpace]];
	if(  c  ) {
		r = (uint8)lround( [c redComponent]   * 255.0 );
		g = (uint8)lround( [c greenComponent] * 255.0 );
		b = (uint8)lround( [c blueComponent]  * 255.0 );
		changed = true;
	}
}

- (void)panelWillClose:(NSNotification*)note
{
	closed = true;
}

@end


static SimutransColorPickerTarget* color_pick_target = nil;
static bool color_pick_running = false;


bool dr_pick_color_start(uint8 r, uint8 g, uint8 b)
{
	if(  color_pick_running  ) {
		return false;
	}

	@autoreleasepool {
		if(  color_pick_target == nil  ) {
			color_pick_target = [[SimutransColorPickerTarget alloc] init];
		}
		color_pick_target->r = r;
		color_pick_target->g = g;
		color_pick_target->b = b;
		color_pick_target->changed = false;
		color_pick_target->closed = false;

		NSColorPanel* panel = [NSColorPanel sharedColorPanel];
		[panel setShowsAlpha: NO];
		[panel setMode: NSColorPanelModeRGB];
		[panel setColor: [NSColor colorWithSRGBRed: r / 255.0 green: g / 255.0 blue: b / 255.0 alpha: 1.0]];
		[panel setTarget: color_pick_target];
		[panel setAction: @selector(colorChanged:)];

		[[NSNotificationCenter defaultCenter] addObserver: color_pick_target
		                                          selector: @selector(panelWillClose:)
		                                              name: NSWindowWillCloseNotification
		                                            object: panel];

		[panel makeKeyAndOrderFront: nil];
		[NSApp activateIgnoringOtherApps: YES];
	}

	color_pick_running = true;
	return true;
}


color_pick_result_t dr_pick_color_poll(uint8 &r, uint8 &g, uint8 &b)
{
	if(  !color_pick_running  ) {
		return COLOR_PICK_NONE;
	}
	if(  !color_pick_target->closed  ) {
		return COLOR_PICK_RUNNING;
	}

	[[NSNotificationCenter defaultCenter] removeObserver: color_pick_target];
	color_pick_running = false;

	if(  color_pick_target->changed  ) {
		r = color_pick_target->r;
		g = color_pick_target->g;
		b = color_pick_target->b;
		return COLOR_PICK_OK;
	}
	return COLOR_PICK_CANCELLED;
}
