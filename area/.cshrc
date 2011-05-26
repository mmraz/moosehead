# InterAccess csh run commands file

umask 022
set history=40 noclobber notify filec
set prompt="interaccess:\!:%/% "

limit coredumpsize 0

if ( -e /usr/skel/system.cshrc ) then
    source /usr/skel/system.cshrc
endif
