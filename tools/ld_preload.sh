#!/bin/sh

usage() {
    printf "usage: %s <command> <library path>\n" "$0" 1>&2
    printf "command:\n"
    printf "  install     Configure /etc/ld.so.preload.\n"
    printf "  uninstall   Remove configuration in ld.so.preload.\n"
    exit 1
}

remove_space() {
    sed -i 's/[ \t\n]\+/ /g' $1
    sed -i 's/^[ \t\n]*//g' $1
    sed -i 's/[ \t\n]*$//g' $1
}

preload=/etc/ld.so.preload

cmd=$1
libpath=$2

case "$cmd" in
    install)
        if [ -z "$libpath" ]; then
            usage
        fi
        printf " %q " "$libpath" >> $preload
        ;;
    uninstall)
        sed -i -e "s/${libpath//\//\\/}/ /g" $preload
        ;;
    *)
        usage
esac

remove_space $preload
