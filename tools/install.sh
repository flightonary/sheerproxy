#!/bin/sh
#
# This is an actually-safe install cmd which installs the new
# file atomically in the new location, rather than overwriting
# existing files.
#

usage() {
    printf "usage: %s [-D] [-m mode] src dest\n" "$0" 1>&2
    exit 1
}

mkdirp=
mode=755

while getopts Dm: name ; do
case "$name" in
    D) mkdirp=yes ;;
    m) mode=$OPTARG ;;
    ?) usage ;;
esac
done
shift $(($OPTIND - 1))

test "$#" -eq 2 || usage
src=$1
dst=$2
tmp="$dst.tmp.$$"

case "$dst" in
    */) printf "%s: %s ends in /\n", "$0" "$dst" 1>&2 ; exit 1 ;;
esac

set -C
set -e

if test "$mkdirp" ; then
    umask 022
    case "$2" in
        */*) mkdir -p "${dst%/*}" ;;
    esac
fi

trap 'rm -f "$tmp"' EXIT INT QUIT TERM HUP

umask 077

cat < "$1" > "$tmp"
chmod "$mode" "$tmp"

mv -f "$tmp" "$2"
test -d "$2" && {
    rm -f "$2/$tmp"
    printf "%s: %s is a directory\n" "$0" "$dst" 1>&2
    exit 1
}

exit 0

