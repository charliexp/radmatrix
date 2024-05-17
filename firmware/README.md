convert video

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

on the SD card, create a folder named `badapple` and inside, add `audio.bin` from `audio` and `gfx.bin` and `gfx_len.bin` from `gfx_output`.
