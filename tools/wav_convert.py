#!/usr/bin/env python3
"""Convert a WAV file to PS2-friendly format: 22050 Hz, mono, 16-bit signed PCM."""

import struct
import sys
import os

TARGET_RATE = 11025
TARGET_CHANNELS = 1
TARGET_BITS = 16

def read_wav(path):
    with open(path, "rb") as f:
        riff = f.read(12)
        if riff[:4] != b"RIFF" or riff[8:12] != b"WAVE":
            raise ValueError("Not a RIFF/WAVE file")

        fmt_found = False
        data_found = False
        audio_fmt = channels = sample_rate = bits_per_sample = 0
        pcm_data = b""

        while True:
            chunk_hdr = f.read(8)
            if len(chunk_hdr) < 8:
                break
            chunk_id = chunk_hdr[:4]
            chunk_size = struct.unpack("<I", chunk_hdr[4:8])[0]

            if chunk_id == b"fmt ":
                fmt_raw = f.read(chunk_size)
                audio_fmt = struct.unpack("<H", fmt_raw[0:2])[0]
                channels = struct.unpack("<H", fmt_raw[2:4])[0]
                sample_rate = struct.unpack("<I", fmt_raw[4:8])[0]
                bits_per_sample = struct.unpack("<H", fmt_raw[14:16])[0]
                fmt_found = True
            elif chunk_id == b"data":
                pcm_data = f.read(chunk_size)
                data_found = True
            else:
                f.seek(chunk_size, 1)

            if chunk_size & 1:
                f.seek(1, 1)

        if not fmt_found or not data_found:
            raise ValueError("Missing fmt or data chunk")
        if audio_fmt != 1:
            raise ValueError(f"Not PCM (format={audio_fmt})")

        return sample_rate, channels, bits_per_sample, pcm_data


def samples_to_float(pcm_data, channels, bits):
    """Convert raw PCM bytes to list of float samples in [-1, 1]."""
    samples = []
    if bits == 8:
        for b in pcm_data:
            samples.append((b - 128) / 128.0)
    elif bits == 16:
        count = len(pcm_data) // 2
        for v in struct.iter_unpack("<h", pcm_data[:count * 2]):
            samples.append(v[0] / 32768.0)
    elif bits == 24:
        i = 0
        while i + 2 < len(pcm_data):
            val = pcm_data[i] | (pcm_data[i+1] << 8) | (pcm_data[i+2] << 16)
            if val & 0x800000:
                val -= 0x1000000
            samples.append(val / 8388608.0)
            i += 3
    elif bits == 32:
        count = len(pcm_data) // 4
        for v in struct.iter_unpack("<i", pcm_data[:count * 4]):
            samples.append(v[0] / 2147483648.0)
    else:
        raise ValueError(f"Unsupported bits: {bits}")
    return samples


def downmix_to_mono(samples, src_channels):
    """Downmix interleaved samples to mono."""
    if src_channels == 1:
        return samples
    mono = []
    i = 0
    while i + src_channels - 1 < len(samples):
        total = 0.0
        for c in range(src_channels):
            total += samples[i + c]
        mono.append(total / src_channels)
        i += src_channels
    return mono


def resample_linear(samples, src_rate, dst_rate):
    """Simple linear interpolation resampler."""
    if src_rate == dst_rate:
        return samples
    ratio = src_rate / dst_rate
    out_len = int(len(samples) / ratio)
    out = []
    for i in range(out_len):
        src_pos = i * ratio
        idx = int(src_pos)
        frac = src_pos - idx
        if idx + 1 < len(samples):
            val = samples[idx] * (1.0 - frac) + samples[idx + 1] * frac
        else:
            val = samples[idx] if idx < len(samples) else 0.0
        out.append(val)
    return out


def float_to_s16(samples):
    """Convert float samples to 16-bit signed PCM bytes."""
    out = bytearray(len(samples) * 2)
    for i, s in enumerate(samples):
        val = int(s * 32767.0)
        if val > 32767:
            val = 32767
        elif val < -32768:
            val = -32768
        struct.pack_into("<h", out, i * 2, val)
    return bytes(out)


def write_wav(path, sample_rate, channels, bits, pcm_data):
    """Write a standard PCM WAV file."""
    block_align = channels * (bits // 8)
    byte_rate = sample_rate * block_align
    data_size = len(pcm_data)
    fmt_chunk = struct.pack("<HHIIHH", 1, channels, sample_rate, byte_rate, block_align, bits)

    with open(path, "wb") as f:
        # RIFF header
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + data_size))
        f.write(b"WAVE")
        # fmt chunk
        f.write(b"fmt ")
        f.write(struct.pack("<I", len(fmt_chunk)))
        f.write(fmt_chunk)
        # data chunk
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        f.write(pcm_data)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} input.wav [output.wav]")
        sys.exit(1)

    in_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else in_path

    print(f"Reading: {in_path}")
    src_rate, src_ch, src_bits, pcm_data = read_wav(in_path)
    print(f"  Source: {src_rate} Hz, {src_ch} ch, {src_bits}-bit, {len(pcm_data)} bytes")

    # Decode to float
    samples = samples_to_float(pcm_data, src_ch, src_bits)
    print(f"  Decoded {len(samples)} samples")

    # Downmix to mono
    samples = downmix_to_mono(samples, src_ch)
    print(f"  Downmixed to mono: {len(samples)} samples")

    # Resample
    samples = resample_linear(samples, src_rate, TARGET_RATE)
    print(f"  Resampled to {TARGET_RATE} Hz: {len(samples)} samples")

    # Convert to 16-bit PCM
    pcm_out = float_to_s16(samples)
    print(f"  Output: {TARGET_RATE} Hz, {TARGET_CHANNELS} ch, {TARGET_BITS}-bit, {len(pcm_out)} bytes")

    # Back up original if overwriting
    if os.path.abspath(in_path) == os.path.abspath(out_path):
        bak = in_path + ".bak"
        if not os.path.exists(bak):
            os.rename(in_path, bak)
            print(f"  Backed up original to {bak}")

    write_wav(out_path, TARGET_RATE, TARGET_CHANNELS, TARGET_BITS, pcm_out)
    size_mb = len(pcm_out) / (1024 * 1024)
    print(f"  Written: {out_path} ({size_mb:.1f} MB)")


if __name__ == "__main__":
    main()
