#!/usr/bin/env python3
"""Print every text message the Heltec receives, with its sender."""
import sys, time
from pubsub import pub
import meshtastic.serial_interface
from ports import meshtastic_port

def on_receive(packet, interface):
    d = packet.get("decoded", {})
    if d.get("portnum") == "TEXT_MESSAGE_APP":
        print(f"TEXT from={packet.get('fromId')} '{d.get('text')}'", flush=True)
    else:
        # to= and req= are what separate "it never answered" from "it answered and we
        # missed it": a reply is addressed to the asker and echoes the request's id.
        extra = ""
        if d.get("requestId"):
            extra += f" req={d.get('requestId')}"
        if d.get("wantResponse"):
            extra += " want_resp"
        print(f"pkt  from={packet.get('fromId')} to={packet.get('toId')} "
              f"{d.get('portnum')}{extra}", flush=True)

pub.subscribe(on_receive, "meshtastic.receive")
# The node's port number changes when it is replugged, so take it as an argument.
port = sys.argv[2] if len(sys.argv) > 2 else meshtastic_port()
iface = meshtastic.serial_interface.SerialInterface(port)
print("listening", flush=True)
time.sleep(float(sys.argv[1]) if len(sys.argv) > 1 else 120)
iface.close()
