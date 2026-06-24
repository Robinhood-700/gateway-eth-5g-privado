#!/usr/bin/env python3
"""USB debug helper for ESP32 Ethernet <-> DECT gateway.

Subcommands:
- cmd: send a single gwdbg command over USB serial.
- e2e: run end-to-end debug scenario (Ethernet ingress + DECT ingress simulation).
"""

from __future__ import annotations

import argparse
import json
import queue
import random
import re
import socket
import threading
import time
from dataclasses import dataclass

try:
    import serial  # type: ignore
except Exception as exc:  # pragma: no cover
    raise SystemExit(
        "pyserial is required. Install with: pip install pyserial"
    ) from exc

PROMPT = "gwdbg> "


@dataclass
class BridgeStats:
    eth_rx: int = 0
    eth_to_dect: int = 0
    eth_err: int = 0
    dect_rx: int = 0
    dect_to_eth: int = 0
    dect_err: int = 0


class UsbDebug:
    def __init__(self, port: str, baud: int, timeout: float = 0.2) -> None:
        self.ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)

    def close(self) -> None:
        self.ser.close()

    def _read_until_prompt(self, timeout_s: float = 2.0) -> str:
        end = time.time() + timeout_s
        out = ""
        while time.time() < end:
            chunk = self.ser.read(self.ser.in_waiting or 1)
            if chunk:
                out += chunk.decode(errors="replace")
                if PROMPT in out:
                    break
            else:
                time.sleep(0.05)
        return out

    def command(self, cmd: str, timeout_s: float = 2.0) -> str:
        self.ser.reset_input_buffer()
        self.ser.write((cmd + "\n").encode())
        self.ser.flush()
        return self._read_until_prompt(timeout_s)


class UdpReceiver:
    def __init__(self, bind_port: int) -> None:
        self.bind_port = bind_port
        self.q: "queue.Queue[str]" = queue.Queue()
        self.stop_evt = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self.thread.start()

    def stop(self) -> None:
        self.stop_evt.set()
        self.thread.join(timeout=1.0)

    def _run(self) -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("0.0.0.0", self.bind_port))
        sock.settimeout(0.2)
        try:
            while not self.stop_evt.is_set():
                try:
                    data, _ = sock.recvfrom(4096)
                except socket.timeout:
                    continue
                self.q.put(data.decode(errors="replace"))
        finally:
            sock.close()


def parse_bridge_stats(output: str) -> BridgeStats:
    pat = re.compile(
        r"eth_rx=(\d+)\s+eth_to_dect=(\d+)\s+eth_err=(\d+)\s+"
        r"dect_rx=(\d+)\s+dect_to_eth=(\d+)\s+dect_err=(\d+)"
    )
    m = pat.search(output)
    if not m:
        return BridgeStats()
    vals = [int(x) for x in m.groups()]
    return BridgeStats(*vals)


def local_ip_towards(remote_ip: str) -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect((remote_ip, 9))
        return s.getsockname()[0]
    finally:
        s.close()


def send_random_udp(gateway_ip: str, gateway_port: int, count: int) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        for i in range(1, count + 1):
            payload = {
                "src": "pc_eth",
                "seq": i,
                "rnd": random.randint(1, 2**31 - 1),
                "ts_ms": int(time.time() * 1000),
            }
            raw = json.dumps(payload).encode()
            sock.sendto(raw, (gateway_ip, gateway_port))
            time.sleep(0.01)
    finally:
        sock.close()


def run_single_cmd(args: argparse.Namespace) -> int:
    dbg = UsbDebug(args.serial, args.baud)
    try:
        out = dbg.command(args.command, timeout_s=args.timeout)
        print(out, end="")
    finally:
        dbg.close()
    return 0


def run_e2e(args: argparse.Namespace) -> int:
    dbg = UsbDebug(args.serial, args.baud)
    rx = UdpReceiver(args.remote_port)
    rx.start()

    try:
        print(dbg.command("status", timeout_s=3.0), end="")
        print(dbg.command("at", timeout_s=3.0), end="")
        print(dbg.command("poe", timeout_s=2.0), end="")
        print(dbg.command("reset_stats", timeout_s=2.0), end="")

        local_ip = local_ip_towards(args.gateway_ip)
        print(dbg.command(f"set_peer {local_ip} {args.remote_port}", timeout_s=2.0), end="")

        # Ethernet -> DECT path: inject from PC over Ethernet UDP.
        send_random_udp(args.gateway_ip, args.listen_port, args.count)
        time.sleep(1.0)
        stats_out_1 = dbg.command("bridge_stats", timeout_s=2.0)
        print(stats_out_1, end="")
        st1 = parse_bridge_stats(stats_out_1)

        # DECT -> Ethernet path: inject from USB debug command.
        print(dbg.command(f"rand_dect {args.count}", timeout_s=4.0), end="")

        deadline = time.time() + args.rx_wait
        received = []
        while time.time() < deadline and len(received) < args.count:
            try:
                received.append(rx.q.get(timeout=0.2))
            except queue.Empty:
                pass

        stats_out_2 = dbg.command("bridge_stats", timeout_s=2.0)
        print(stats_out_2, end="")
        st2 = parse_bridge_stats(stats_out_2)

        print("\nE2E summary")
        print(f"- ETH->DECT forwarded (bridge counter): {st1.eth_to_dect}/{args.count}")
        print(f"- DECT->ETH UDP received on PC: {len(received)}/{args.count}")
        print(f"- Final dect_to_eth counter: {st2.dect_to_eth}")

        ok_eth = st1.eth_to_dect >= args.count
        ok_dect = len(received) >= args.count

        if ok_eth and ok_dect:
            print("RESULT: PASS")
            return 0
        print("RESULT: FAIL")
        return 1
    finally:
        rx.stop()
        dbg.close()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Gateway USB debug helper")
    sub = p.add_subparsers(dest="sub", required=True)

    p_cmd = sub.add_parser("cmd", help="send one command over USB debug")
    p_cmd.add_argument("--serial", required=True, help="USB serial port (e.g. /dev/ttyUSB0)")
    p_cmd.add_argument("--baud", type=int, default=115200)
    p_cmd.add_argument("--timeout", type=float, default=2.0)
    p_cmd.add_argument("command", help="gwdbg command (example: status)")
    p_cmd.set_defaults(fn=run_single_cmd)

    p_e2e = sub.add_parser("e2e", help="run Ethernet<->DECT E2E debug scenario")
    p_e2e.add_argument("--serial", required=True, help="USB serial port")
    p_e2e.add_argument("--baud", type=int, default=115200)
    p_e2e.add_argument("--gateway-ip", required=True, help="gateway Ethernet IPv4")
    p_e2e.add_argument("--listen-port", type=int, default=5000, help="gateway UDP listen port")
    p_e2e.add_argument("--remote-port", type=int, default=5000, help="PC UDP receive port")
    p_e2e.add_argument("--count", type=int, default=20, help="packets per direction")
    p_e2e.add_argument("--rx-wait", type=float, default=6.0, help="seconds to wait for DECT->ETH UDP")
    p_e2e.set_defaults(fn=run_e2e)

    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.fn(args)


if __name__ == "__main__":
    raise SystemExit(main())
