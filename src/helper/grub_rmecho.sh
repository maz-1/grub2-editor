#!/bin/bash

argument () {
  opt=$1
  shift
  if test $# -eq 0; then
      exit 1
  fi
  echo $1
}

SCRIPT=$(realpath -s $0)
SCRIPT_ESCAPED=$(echo $SCRIPT|sed -e 's/[\/&]/\\&/g')
PARENT_FULLCMD=$(ps -o args= $PPID)
eval set -- $PARENT_FULLCMD
shift 2
while test $# -gt 0
do
    option=$1
    shift

    case "$option" in
    -o | --output)
        grub_cfg=`argument $option "$@"`; shift;;
    --output=*)
        grub_cfg=`echo "$option" | sed 's/--output=//'`
        ;;
    -*)
        ;;
    esac
done
if test -z "$grub_cfg"
then
    grub_cfg=$(readlink -f /proc/$PPID/fd/1)
fi
. /etc/default/grub

if [ "z$GRUB_NOECHO" = "ztrue" ] && test -f $grub_cfg
then
    echo "Remove echo commands from \"$grub_cfg\"" >&2
    echo "while kill -0 $PPID 2> /dev/null; do sleep 0.2; done;sed -i -E -e '/^\s*echo\s+.*$/d' -e '/^\s*#+\s*BEGIN $SCRIPT_ESCAPED/d' -e '/^\s*#+\s*END $SCRIPT_ESCAPED/d' '$grub_cfg' 2>&1; rm -f /tmp/rmecho.$PPID" >/tmp/rmecho.$PPID
    nohup bash /tmp/rmecho.$PPID >/dev/null 2>&1 &
fi
