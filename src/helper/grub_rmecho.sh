#!/bin/sh
. /etc/default/grub
if [[ "z$GRUB_NOECHO" = "ztrue" && -f "$@" ]]
then
    sed -i -E '/^\s*echo\s+.*$/d' "$@"
fi
