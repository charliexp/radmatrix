import os
import sys

def get_all_gfx():
    gfx = []
    for root, dirs, files in os.walk("gfx"):
        for file in files:
            if file.startswith("."):
                continue
            # file name without extention
            name = os.path.splitext(file)[0]
            gfx.append((name, os.path.join(root, file)))

    gfx.sort()
    return gfx

blob = bytes()
lengths = bytes()

gfx_files = get_all_gfx()
for (name, path) in gfx_files:
    with open(path, "rb") as f:
        data = f.read()
        size = len(data)
        blob += data
        lengths += size.to_bytes(2, byteorder="little")

# create the output directory if it doesn't exist
os.makedirs("gfx_output", exist_ok=True)

with open("gfx_output/gfx.bin", "wb") as f:
    f.write(blob)

with open("gfx_output/gfx_len.bin", "wb") as f:
    f.write(lengths)
