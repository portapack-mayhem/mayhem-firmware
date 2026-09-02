#!/usr/bin/env python3
"""One preset, both directions, against a real Meshtastic node.

  sweep_one.py <PRESET> <tag>

Sets the preset on both radios, sends a text each way and reports what actually
arrived. The Heltec is the ground truth in both directions: it decodes what we
send, and what it sends is what a stock node really puts on the air.

Assumes the PortaPack is showing the Meshtastic app's Chat tab.
"""
import subprocess, sys, time, serial
import os
from ports import portapack_port, meshtastic_port

PP   = portapack_port()
HEL  = meshtastic_port()
HERE = os.path.dirname(os.path.abspath(__file__))

def pp(cmd, wait=2.2):
    with serial.Serial(PP, 115200, timeout=0.2) as s:
        s.write((cmd + "\r\n").encode()); s.flush(); time.sleep(wait); s.read(16384)

def run(*args, timeout=240):
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout).stdout

def chat_text(tag="sw"):
    subprocess.run([sys.executable, f"{HERE}/shot.py", f"/tmp/{tag}.png"], capture_output=True)
    return run(sys.executable, f"{HERE}/ocr.py", f"/tmp/{tag}.png", "8x16", "0", "320", "8")

def main(preset, tag):
    slow = preset in ("LONG_SLOW", "VERY_LONG_SLOW", "LONG_MODERATE")
    wait_rx = 75 if slow else 45

    out = run(sys.executable, "-m", "meshtastic", "--port", HEL,
              "--set", "lora.modem_preset", preset)
    if "Set lora.modem_preset" not in out:
        print(f"{preset}: the node would not take this preset"); return 2
    time.sleep(22)                                    # it reboots

    # PortaPack: Setup tab -> Radio -> preset -> back -> Chat tab
    pp("touch 200 28"); pp("touch 120 121", 3.0)
    r = run(sys.executable, f"{HERE}/preset_set.py", preset)
    if "set to" not in r and "already there" not in r:
        print(f"{preset}: could not set on the PortaPack:\n{r}"); return 2
    pp("touch 10 5", 3.0)                             # leave Radio: applies + retunes
    pp("touch 22 28", 2.5)                            # Chat tab

    # --- receive ---------------------------------------------------------
    run(sys.executable, "-m", "meshtastic", "--port", HEL, "--sendtext", tag)
    time.sleep(wait_rx)
    screen = chat_text(f"sw_{tag}")
    rx = tag in screen
    print(f"{preset:15} RX {'yes' if rx else 'NO '}   (looking for {tag!r})")

    # --- transmit --------------------------------------------------------
    # The app answers a received text with an echo carrying the signal report, so the
    # node's own log proves our transmitter without anyone opening the on-screen
    # keyboard - worth avoiding: that takes 4.8 kB of core and the margin left after
    # the app is under two.
    log = f"/tmp/hl_{tag}.log"
    lis = subprocess.Popen([sys.executable, f"{HERE}/hlisten.py", "150"],
                           stdout=open(log, "w"), stderr=subprocess.STDOUT)
    time.sleep(13)
    pp("touch 135 28", 2.5)                           # Data tab
    pp("touch 150 305", 3.0)                          # "Info" - send NodeInfo now
    time.sleep(70 if slow else 45)
    pp("touch 22 28", 2.0)                            # back to Chat
    lis.terminate(); time.sleep(2)
    # Any frame of ours the node decoded proves the transmitter on this preset.
    # Whatever this PortaPack calls itself. Reading it from the device beats writing a
    # node id in here: the id is derived from the board, so it differs per unit.
    me = os.environ.get("PORTAPACK_NODE_ID", "").lower().lstrip("!")
    ours = [l.strip() for l in open(log)
            if (f"from=!{me}" in l if me else ("from=!" in l and "TEXT" in l))]
    print(f"{preset:15} TX {'yes' if ours else 'NO '}   {ours[-1] if ours else ''}")
    return 0 if (rx and ours) else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
