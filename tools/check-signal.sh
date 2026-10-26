#!/usr/bin/env bash
set -euo pipefail
binary=$1
work=$2
mkdir -p "$work"
rm -f "$work/readings.csv" "$work/events.csv" "$work/console.txt"
"$binary" --deterministic --count 20 --interval-ms 1000 --log-dir "$work" > "$work/console.txt" 2> "$work/stderr.txt" &
child=$!
trap 'kill -TERM "$child" 2>/dev/null || true' EXIT
# Poll an observable readiness condition, not a delay chosen to guess startup time.
# CTest supplies a ten-second process timeout if startup or shutdown hangs.
while :; do
    if [[ -f "$work/readings.csv" ]] && (( $(wc -l < "$work/readings.csv") >= 2 )); then break; fi
    if ! kill -0 "$child" 2>/dev/null; then wait "$child"; exit 1; fi
    sleep 0.01
done
kill -INT "$child"
wait "$child"
trap - EXIT
processed=$(sed -n 's/^Processed \([0-9]*\) readings\..*/\1/p' "$work/console.txt")
rows=$(($(wc -l < "$work/readings.csv")-1))
[[ $processed == "$rows" && $rows -gt 0 && $rows -lt 20 && ! -s "$work/stderr.txt" ]]
printf 'PASS SIGINT: exit=0 summary=%s CSV_rows=%s\n' "$processed" "$rows"
