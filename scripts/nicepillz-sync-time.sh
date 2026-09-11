#!/bin/bash
# Sets the clock on the Nice Pillz display from this computer's local time.
#
# The keyboard exposes a small USB serial "time port". This script writes the
# local time to it and expects "OK HH:MM" back. With no argument it tries every
# serial port that belongs to a NicePillz keyboard (the Studio RPC port simply
# does not answer). Usage:
#
#   nicepillz-sync-time.sh              # find the port automatically
#   nicepillz-sync-time.sh /dev/ttyACM1 # use a specific port
#
# The clock runs from the keyboard's uptime after a sync, so it survives
# unplugging and Bluetooth use, but not a reset or deep sleep. Re-run this
# whenever the display shows "--:--" (a udev rule can do it on plug-in, see
# the README).

epoch_local=$(date -u -d "$(date '+%F %T')" +%s)

if [ $# -ge 1 ]; then
    ports=("$@")
else
    shopt -s nullglob
    ports=(/dev/serial/by-id/*NicePillz*-if0[0-9]* /dev/serial/by-id/*Nice_Pillz*-if0[0-9]*)
    shopt -u nullglob
fi

if [ ${#ports[@]} -eq 0 ]; then
    echo "no NicePillz serial port found (is the keyboard plugged in over USB?)" >&2
    exit 1
fi

for port in "${ports[@]}"; do
    [ -e "$port" ] || continue
    stty -F "$port" raw -echo 115200 2>/dev/null || continue
    exec 3<>"$port" || continue
    printf 'T%s\n' "$epoch_local" >&3
    if read -t 2 -r reply <&3 && [[ $reply == OK* ]]; then
        echo "time set via $port: $reply"
        exec 3>&-
        exit 0
    fi
    exec 3>&-
done

echo "no NicePillz time port answered on: ${ports[*]}" >&2
exit 1
