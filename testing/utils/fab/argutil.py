# Some argument parsing functions.
#
# Copyright (C) 2015 Andrew Cagney <cagney@gnu.org>
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

import sys

def timeout(arg):
    arg = arg.lower()
    if arg == "none" or arg == "infinite":
        return None
    v = float(arg)
    if v < 0:
        return None
    else:
        return v

def stdout_or_open_file(arg):
    if arg == "-":
        return sys.stdout
    elif arg.endswith("+"):
        return open(arg[:-1], "a")
    elif arg.startswith("-"):
        return open(arg[1:], "a")
    else:
        return open(arg, "w")
