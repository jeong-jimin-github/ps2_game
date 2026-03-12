#!/usr/bin/env python3
"""
WAV → PS2 SPU2 ADPCM (.adpcm) 변환 도구

사용법:
  python wav_to_adpcm.py assets/sfx_jump.wav
  python wav_to_adpcm.py --all assets     (assets/ 내 sfx_*.wav 전부 변환)

내부 동작:
  1. WAV 읽기 → 48000 Hz 모노 16-bit signed LE PCM으로 리샘플
  2. ps2adpcm.exe 로 SPU2 ADPCM 변환
  3. 결과를 같은 디렉터리에 .adpcm 확장자로 저장
"""
import sys, os, struct, wave, tempfile, subprocess, glob

# PS2 SPU2 기본 재생 레이트 (pitch=0x1000 일 때)
TARGET_RATE = 48000

def find_ps2adpcm():
    """ps2adpcm.exe 를 PATH 또는 PS2SDK/bin 에서 찾기"""
    # 환경변수
    ps2sdk = os.environ.get("PS2SDK", "")
    candidates = [
        os.path.join(ps2sdk, "bin", "ps2adpcm.exe"),
        os.path.join(ps2sdk, "bin", "ps2adpcm"),
    ]
    # 하드코딩 fallback
    candidates.append(r"C:\Users\jm\Documents\ps2dev\ps2sdk\bin\ps2adpcm.exe")
    candidates.append("/c/Users/jm/Documents/ps2dev/ps2sdk/bin/ps2adpcm.exe")

    for c in candidates:
        if os.path.isfile(c):
            return c
    # PATH 에서 찾기
    import shutil
    found = shutil.which("ps2adpcm") or shutil.which("ps2adpcm.exe")
    if found:
        return found
    return None

def resample_linear(samples, src_rate, dst_rate):
    """간단한 선형 보간 리샘플링"""
    if src_rate == dst_rate:
        return samples
    ratio = src_rate / dst_rate
    out_len = int(len(samples) / ratio)
    out = []
    for i in range(out_len):
        pos = i * ratio
        idx = int(pos)
        frac = pos - idx
        s0 = samples[idx] if idx < len(samples) else 0
        s1 = samples[idx + 1] if (idx + 1) < len(samples) else s0
        val = int(s0 * (1.0 - frac) + s1 * frac)
        if val > 32767: val = 32767
        if val < -32768: val = -32768
        out.append(val)
    return out

def load_wav(path):
    """WAV 파일 읽어서 (samples_list, sample_rate, channels, bits) 반환"""
    with wave.open(path, 'rb') as wf:
        ch = wf.getnchannels()
        sw = wf.getsampwidth()
        rate = wf.getframerate()
        n = wf.getnframes()
        raw = wf.readframes(n)

    if sw == 1:
        # unsigned 8-bit → signed 16-bit
        samples = [(b - 128) * 256 for b in raw]
        per_frame = ch
    elif sw == 2:
        fmt = '<' + 'h' * (len(raw) // 2)
        samples = list(struct.unpack(fmt, raw))
        per_frame = ch
    else:
        raise ValueError(f"Unsupported sample width: {sw}")

    # 스테레오 → 모노 변환
    if ch == 2:
        mono = []
        for i in range(0, len(samples), 2):
            l = samples[i]
            r = samples[i + 1] if (i + 1) < len(samples) else l
            val = (l + r) // 2
            if val > 32767: val = 32767
            if val < -32768: val = -32768
            mono.append(val)
        samples = mono

    return samples, rate

def convert_wav_to_adpcm(wav_path, ps2adpcm_exe):
    """WAV 파일 → .adpcm 변환"""
    samples, src_rate = load_wav(wav_path)

    # 48kHz 로 리샘플
    samples = resample_linear(samples, src_rate, TARGET_RATE)

    # raw PCM (signed 16-bit LE) 임시 파일
    raw_pcm = struct.pack('<' + 'h' * len(samples), *samples)

    base = os.path.splitext(wav_path)[0]
    adpcm_path = base + ".adpcm"

    # 임시 파일에 raw PCM 쓰기
    with tempfile.NamedTemporaryFile(suffix='.pcm', delete=False) as tmp:
        tmp.write(raw_pcm)
        tmp_path = tmp.name

    try:
        # ps2adpcm 은 임시 파일에 raw ADPCM 출력
        raw_adpcm_path = tmp_path + ".adpcm"
        cmd = [ps2adpcm_exe, tmp_path, raw_adpcm_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  ERROR: ps2adpcm failed for {wav_path}")
            print(f"  stderr: {result.stderr}")
            return False

        # raw ADPCM 데이터 읽기
        with open(raw_adpcm_path, 'rb') as f:
            raw_adpcm = bytearray(f.read())
        os.unlink(raw_adpcm_path)

        # SPU2 ADPCM 종료 플래그 패치
        # 각 16바이트 블록의 byte[1]이 flags: 0x01 = end(mute)
        # ps2adpcm.exe는 end flag를 안 넣으므로, 마지막 블록에 수동 설정
        if len(raw_adpcm) >= 16:
            last_block = len(raw_adpcm) - 16
            raw_adpcm[last_block + 1] = 0x01  # end flag

        raw_adpcm = bytes(raw_adpcm)

        # audsrv 16-byte 헤더 생성
        # buffer[0] = raw adpcm size (unused by audsrv, informational)
        # buffer[1] = (loop << 16) | (channels << 8)
        # buffer[2] = pitch (0x1000 = 48kHz)
        # buffer[3] = 0 (unused)
        pitch = 0x1000  # 48kHz
        channels = 1    # mono
        loop = 0        # no loop
        header = struct.pack('<IIII',
            len(raw_adpcm),
            (loop << 16) | (channels << 8),
            pitch,
            0)

        # 헤더 + raw ADPCM 을 최종 파일에 쓰기
        with open(adpcm_path, 'wb') as f:
            f.write(header)
            f.write(raw_adpcm)
    finally:
        if os.path.isfile(tmp_path):
            os.unlink(tmp_path)

    sz = os.path.getsize(adpcm_path)
    dur_ms = len(samples) * 1000 // TARGET_RATE
    print(f"  {os.path.basename(wav_path)} -> {os.path.basename(adpcm_path)} "
          f"({sz} bytes, ~{dur_ms}ms, +16byte header)")
    return True

def main():
    if len(sys.argv) < 2:
        print("Usage: wav_to_adpcm.py <file.wav>")
        print("       wav_to_adpcm.py --all <directory>")
        sys.exit(1)

    ps2adpcm_exe = find_ps2adpcm()
    if not ps2adpcm_exe:
        print("ERROR: ps2adpcm.exe not found. Set PS2SDK env variable.")
        sys.exit(1)
    print(f"Using: {ps2adpcm_exe}")

    if sys.argv[1] == '--all':
        d = sys.argv[2] if len(sys.argv) > 2 else '.'
        wavs = sorted(glob.glob(os.path.join(d, 'sfx_*.wav')))
        if not wavs:
            print(f"No sfx_*.wav files in {d}")
            sys.exit(0)
        ok = 0
        for w in wavs:
            if convert_wav_to_adpcm(w, ps2adpcm_exe):
                ok += 1
        print(f"Converted {ok}/{len(wavs)} SFX files.")
    else:
        if not convert_wav_to_adpcm(sys.argv[1], ps2adpcm_exe):
            sys.exit(1)

if __name__ == '__main__':
    main()
