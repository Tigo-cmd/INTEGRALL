#!/usr/bin/env python3
"""
Register and manage Integrall devices from the terminal (replaces the retired
React dashboard's device page; OpenRemote is the UI for everything else).

  python cloud/tools/integrall_devices.py --email you@example.com signup   # first time only
  python cloud/tools/integrall_devices.py add     INT_A1B2C3D4E5F6 "Garage pump"
  python cloud/tools/integrall_devices.py list
  python cloud/tools/integrall_devices.py rotate  INT_A1B2C3D4E5F6
  python cloud/tools/integrall_devices.py remove  INT_A1B2C3D4E5F6

`add`/`rotate` print the device token ONCE: put it in the sketch
(DEVICE_TOKEN in examples/CloudMQTT). The device then appears in OpenRemote
automatically. Options: --api http://localhost:8000 (or env INTEGRALL_API),
--email (or env INTEGRALL_EMAIL); the password is asked for interactively
(or env INTEGRALL_PASSWORD). Standard library only.
"""
import argparse
import getpass
import json
import os
import sys
import urllib.error
import urllib.request


def call(api, method, path, body=None, token=None):
    req = urllib.request.Request(api.rstrip("/") + path, method=method,
                                 data=json.dumps(body).encode() if body is not None else None,
                                 headers={"Content-Type": "application/json",
                                          **({"Authorization": f"Bearer {token}"} if token else {})})
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            raw = r.read()
            return json.loads(raw) if raw else None
    except urllib.error.HTTPError as e:
        try:
            detail = json.loads(e.read()).get("detail")
        except Exception:
            detail = e.reason
        sys.exit(f"error {e.code}: {detail}")
    except urllib.error.URLError as e:
        sys.exit(f"cannot reach {api}: {e.reason}")


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--api", default=os.environ.get("INTEGRALL_API", "http://localhost:8000"))
    p.add_argument("--email", default=os.environ.get("INTEGRALL_EMAIL"))
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("signup")
    sub.add_parser("list")
    a = sub.add_parser("add"); a.add_argument("id"); a.add_argument("name")
    for n in ("rotate", "remove"):
        sub.add_parser(n).add_argument("id")
    args = p.parse_args()

    email = args.email or input("email: ")
    password = os.environ.get("INTEGRALL_PASSWORD") or getpass.getpass("password: ")

    if args.cmd == "signup":
        call(args.api, "POST", "/api/auth/register", {"email": email, "password": password})
        print("account created")
        return
    tok = call(args.api, "POST", "/api/auth/login", {"email": email, "password": password})["access_token"]

    if args.cmd == "list":
        for d in call(args.api, "GET", "/api/devices", token=tok):
            print(f"{d['id']}  {'online ' if d['online'] else 'offline'}  {d['name']}")
    elif args.cmd == "add":
        d = call(args.api, "POST", "/api/devices", {"id": args.id.upper(), "name": args.name}, token=tok)
        print(f"registered {d['id']} ({d['name']})\ndevice token (shown once): {d['token']}")
    elif args.cmd == "rotate":
        d = call(args.api, "POST", f"/api/devices/{args.id}/rotate-token", token=tok)
        print(f"new device token (shown once): {d['token']}")
    elif args.cmd == "remove":
        call(args.api, "DELETE", f"/api/devices/{args.id}", token=tok)
        print(f"removed {args.id} (and its OpenRemote asset)")


if __name__ == "__main__":
    main()
