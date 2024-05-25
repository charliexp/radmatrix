## convert video

automatically:

```sh
scripts/convert.sh ../badapple.webm
```

the output is in video_output

or manually:

```
ffmpeg -i ../badapple.webm -vf "fps=30,scale=40:40:force_original_aspect_ratio=increase,crop=40:40,format=gray" gfx/frame_%04d.png
```

move to `gfx` folder, then:

```
# old method
# python3 scripts/gfx_convert.py
python3 scripts/gfx_to_blob.py
```

convert audio

```
ffmpeg -i ../badapple.webm -ar 44000 audio/output.wav
```

move to `audio` folder, then:

```
python3 scripts/audio_convert.py
```

## play videos on device

videos/YOURNAME/files

(where files are audio.bin, gfx.bin, gfx_len.bin)

NOTE: yourname MUST be at most 8 characters long

also `videos/playlist.txt` with names of videos in order
