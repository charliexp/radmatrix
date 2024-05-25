#!/usr/bin/env bash
set -e
set -x

echo "converting $1"
rm -fr gfx
rm -fr video_output
rm -fr audio
mkdir gfx
ffmpeg -i "$1" -vf "fps=30,scale=40:40:force_original_aspect_ratio=increase,crop=40:40,format=gray" gfx/frame_%04d.png
python3 scripts/gfx_to_blob.py
mkdir audio
ffmpeg -i "$1" -ar 44000 audio/output.wav
python3 scripts/audio_convert.py
open video_output
