#!/bin/tclsh

package require Tk


ttk::frame .c
ttk::label .c.title -text "Key Value Client"
ttk::label .c.keylbl -text Key:
ttk::label .c.vallbl -text Value:
ttk::label .c.servlbl -text Server:
ttk::entry .c.key
ttk::entry .c.val
ttk::entry .c.serv
ttk::button .c.fetch -text Fetch -command query
tk::listbox .c.resf -width 50 -height 10

grid .c -column 0 -row 0 -pady 5 -padx 5
grid .c.title -column 0 -row 0 -columnspan 3 -pady 5
grid .c.keylbl -column 0 -row 2 -pady 5 -padx 5
grid .c.key -column 1 -row 2 -padx 5
grid .c.vallbl -column 2 -row 2 -pady 5 -padx 5
grid .c.val -column 3 -row 2 -padx 5
grid .c.servlbl -column 0 -row 1 -padx 5
grid .c.serv -column 1 -row 1 -padx 5
grid .c.fetch -column 0 -columnspan 2 -row 3 -pady 5
grid .c.resf -column 0 -row 4 -columnspan 4

.c.serv insert 0 localhost:59000

proc query { } {
	global history
	set lst [split [.c.serv get] :]
	set host [lindex $lst 0]
	set port [lindex $lst 1]
	if {[string length [.c.val get]] == 0} {
		set desc "GET [.c.key get]"
		set cmd [.c.key get]
	} else {
		set desc "SET [.c.key get]:[.c.val get]"
		set cmd "[.c.key get]:[.c.val get]"
	}
	set sock [socket $host $port]
	puts $sock $cmd
	flush $sock
	gets $sock res
	close $sock
	.c.resf insert 0 "$desc => $res"
	
	
}
