/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Native color picker for Linux (SDL2 backend). There is no single toolkit
 * available on every distro, so this shells out to whichever color picker
 * dialog is installed (zenity, then kdialog) rather than linking against
 * GTK/Qt. Counterpart to the Windows ChooseColor() implementation in
 * simsys_w.cc.
 */

#include "simsys.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>


// The dialog runs as a child process so the game loop is not blocked while
// it is open; its stdout (a single "#rrggbb" line on success) is collected
// through a non-blocking pipe and picked up later via dr_pick_color_poll(),
// called once per frame from the GUI while a pick is pending.
static pid_t color_pick_pid = -1;
static int color_pick_fd = -1;


bool dr_pick_color_start(uint8 r, uint8 g, uint8 b)
{
	if(  color_pick_pid != -1  ) {
		return false;
	}

	int pipefd[2];
	if(  pipe(pipefd) != 0  ) {
		return false;
	}

	char hex[8];
	snprintf( hex, sizeof(hex), "#%02x%02x%02x", r, g, b );

	char zenity_color[16];
	snprintf( zenity_color, sizeof(zenity_color), "--color=%s", hex );

	const pid_t pid = fork();
	if(  pid < 0  ) {
		close( pipefd[0] );
		close( pipefd[1] );
		return false;
	}

	if(  pid == 0  ) {
		// child: stdout -> pipe, stdin/stderr silenced, then try zenity, then kdialog
		close( pipefd[0] );
		dup2( pipefd[1], STDOUT_FILENO );
		close( pipefd[1] );

		const int devnull = open( "/dev/null", O_RDWR );
		if(  devnull != -1  ) {
			dup2( devnull, STDIN_FILENO );
			dup2( devnull, STDERR_FILENO );
			close( devnull );
		}

		execlp( "zenity", "zenity", "--color-selection", zenity_color, (char*)NULL );
		execlp( "kdialog", "kdialog", "--getcolor", "--default", hex, (char*)NULL );
		_exit( 127 );
	}

	// parent
	close( pipefd[1] );
	fcntl( pipefd[0], F_SETFL, O_NONBLOCK );
	color_pick_pid = pid;
	color_pick_fd  = pipefd[0];
	return true;
}


color_pick_result_t dr_pick_color_poll(uint8 &r, uint8 &g, uint8 &b)
{
	if(  color_pick_pid == -1  ) {
		return COLOR_PICK_NONE;
	}

	int status;
	const pid_t res = waitpid( color_pick_pid, &status, WNOHANG );
	if(  res == 0  ) {
		return COLOR_PICK_RUNNING;
	}

	// child exited (or waitpid failed, e.g. ECHILD): collect whatever it wrote
	char buf[64];
	int len = 0;
	if(  color_pick_fd != -1  ) {
		const ssize_t n = read( color_pick_fd, buf, sizeof(buf) - 1 );
		if(  n > 0  ) {
			len = (int)n;
		}
		close( color_pick_fd );
		color_pick_fd = -1;
	}
	buf[len] = '\0';
	color_pick_pid = -1;

	const bool exited_ok = res > 0  &&  WIFEXITED(status)  &&  WEXITSTATUS(status) == 0;

	unsigned int ir, ig, ib;
	if(  exited_ok  &&  sscanf( buf, "#%2x%2x%2x", &ir, &ig, &ib ) == 3  ) {
		r = (uint8)ir;
		g = (uint8)ig;
		b = (uint8)ib;
		return COLOR_PICK_OK;
	}
	return COLOR_PICK_CANCELLED;
}
