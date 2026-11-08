#!/usr/bin/env python3
"""Teste real Linux/PipeWire: app + core + jogo, sem tocar nos alto-falantes.
Uso: python tests/audio_smoke.py /caminho/build/app /caminho/jogo.cue
Requer pactl e parec; o jogo e a BIOS não são distribuídos com o teste.
"""
import array
import json
import os
from pathlib import Path
import subprocess
import sys
import time

sink = f"texel_audio_test_{os.getpid()}"
module = subprocess.check_output([
    "pactl", "load-module", "module-null-sink", f"sink_name={sink}"
], text=True).strip()
app = recorder = None
try:
    env = dict(os.environ, SDL_AUDIO_DRIVER="pulseaudio",
               SDL_VIDEO_DRIVER="offscreen", PULSE_SINK=sink)
    app = subprocess.Popen([str(Path(sys.argv[1]).resolve()),
                            str(Path(sys.argv[2]).resolve())], env=env,
                           stdout=subprocess.DEVNULL)
    deadline = time.monotonic() + 10
    stream = None
    while time.monotonic() < deadline and app.poll() is None:
        inputs = json.loads(subprocess.check_output(
            ["pactl", "-f", "json", "list", "sink-inputs"], text=True))
        stream = next((x for x in inputs if str(x.get("properties", {}).get(
            "application.process.id")) == str(app.pid)), None)
        if stream:
            break
        time.sleep(0.2)
    assert stream is not None, "FAIL: programa não criou saída de áudio"
    # SDL pode escolher explicitamente o dispositivo padrão, ignorando
    # PULSE_SINK; roteamos apenas o stream deste processo para a saída de teste.
    subprocess.run(["pactl", "move-sink-input", str(stream["index"]), sink],
                   check=True)
    subprocess.run(["pactl", "set-sink-input-mute", str(stream["index"]), "0"],
                   check=True)
    recorder = subprocess.Popen([
        "parec", f"--device={sink}.monitor", "--format=s16le",
        "--rate=44100", "--channels=2", "--raw"
    ], stdout=subprocess.PIPE)
    try:
        # O boot PAL e os avisos iniciais podem ficar silenciosos por dezenas
        # de segundos; esperamos a introdução, não apenas a abertura da janela.
        data, _ = recorder.communicate(timeout=55)
    except subprocess.TimeoutExpired:
        recorder.terminate()
        data, _ = recorder.communicate(timeout=5)
    samples = array.array("h", data[:len(data) // 2 * 2])
    peak = max((abs(x) for x in samples), default=0)
    assert app.poll() is None, "FAIL: programa encerrou durante reprodução"
    assert peak > 0, "FAIL: saída existe, mas todas as amostras são silêncio"
    print(f"PASS: saída real, {len(samples)} amostras capturadas, pico={peak}")
finally:
    for process in (recorder, app):
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    subprocess.run(["pactl", "unload-module", module], check=True)
