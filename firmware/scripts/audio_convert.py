import os
import sys
import soundfile as sf
import samplerate

# Adapted from https://github.com/rgrosset/pico-pwm-audio

print("loading file...")

soundfile = 'audio/output.wav'
data_in, datasamplerate = sf.read(soundfile)
# This means stereo so extract one channel 0
if len(data_in.shape)>1:
    data_in = data_in[:,0]

print("resampling...")

converter = 'sinc_best'  # or 'sinc_fastest', ...
desired_sample_rate = 11000.0
ratio = desired_sample_rate/datasamplerate
data_out = samplerate.resample(data_in, ratio, converter)

print("analyzing...")

maxValue = max(data_out)
minValue = min(data_out)
print("length", len(data_out))
print("max value", max(data_out))
print("min value", min(data_out))
vrange = (maxValue - minValue)
print("value range", vrange)

print("normalizing...")

# normalize to 0-1
normalized = [int((v-minValue)/vrange*255) for v in data_out]

print("generating header...")

m68code = "/*    File "+soundfile+ "\r\n *    Sample rate "+str(int(desired_sample_rate)) +" Hz\r\n */\r\n"
m68code += "#define WAV_DATA_LENGTH "+str(len(data_out))+" \r\n\r\n"
m68code += "static const uint8_t WAV_DATA[] = {\r\n    "

m68code += ','.join(str(v) for v in normalized)

# keep track of first and last values to avoid
# blip when the loop restarts.. make the end value
# the average of the first and last.
end_value = int( (normalized[0] + normalized[len(normalized) - 1]) / 2)
m68code+=","+str(end_value)+'\n};\n'

print("writing output...")

with open("src/audio_sample.h", "w") as f:
    f.write(m68code)

print("done!")

print("writing blob...")

with open("audio/audio.bin", "wb") as f:
    f.write(bytes(normalized))

print("done!")
