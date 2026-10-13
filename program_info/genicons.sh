#!/usr/bin/env bash
# https://github.com/PrismLauncher/PrismLauncher/blob/736213d92b554a26b5a1ac8d9b3046d8cd8b4d1c/program_info/genicons.sh

APP_ID="com.coolguy1842.CaptureCardRelay"
APP_COMMON_NAME="CaptureCardRelay"

svg2png() {
    input_file="$1"
    output_file="$2"
    width="$3"
    height="$4"

    inkscape -w "$width" -h "$height" -o "$output_file" "$input_file"
}

if command -v "inkscape" && command -v "icotool" && command -v "oxipng"; then
    # Windows ICO
    d=$(mktemp -d)

    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_16.png" 16 16
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_24.png" 24 24
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_32.png" 32 32
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_48.png" 48 48
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_64.png" 64 64
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_128.png" 128 128
    svg2png ${APP_ID}.svg "$d/CaptureCardRelay_256.png" 256 256

    oxipng --opt max --strip all --alpha --interlace 0 "$d/CaptureCardRelay_"*".png"

    icotool -o CaptureCardRelay.ico -c \
        "$d/CaptureCardRelay_256.png"  \
        "$d/CaptureCardRelay_128.png"  \
        "$d/CaptureCardRelay_64.png"   \
        "$d/CaptureCardRelay_48.png"   \
        "$d/CaptureCardRelay_32.png"   \
        "$d/CaptureCardRelay_24.png"   \
        "$d/CaptureCardRelay_16.png"
else
    echo "ERROR: Windows icons were NOT generated!" >&2
    echo "ERROR: requires inkscape, icotool and oxipng in PATH"
fi

if command -v "inkscape"; then
    svg2png ${APP_ID}.svg "${APP_ID}_256.png" 256 256
else
    echo "ERROR: The Linux icon was NOT generated!" >&2
    echo "ERROR: requires inkscape in PATH"
fi