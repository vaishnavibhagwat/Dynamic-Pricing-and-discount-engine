"""Tiny web frontend: serves index.html and drives the real C++ CLI over stdin.
Run: python web/server.py   then open http://localhost:8000
ponytail: spawns one pricing_engine per request and shows its text output;
add a --json mode to the C++ app if structured output is needed.
"""
import json, re, subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / "pricing_engine.exe"
DATA = ROOT / "data"
ANSI = re.compile(r"\x1b\[[0-9;]*m")


def run(menu_script):
    p = subprocess.run([str(EXE), "--data", str(DATA)], input=menu_script,
                       capture_output=True, text=True, encoding="utf-8", timeout=20)
    parts = ANSI.sub("", p.stdout + p.stderr).split("Choose an option: ")
    # parts[4] = what the chosen action printed (after customer, cart, coupon steps)
    text = parts[4] if len(parts) > 4 else "".join(parts)
    text = text.split("\n------------------------------------------------------------------------\nDate ")[0]
    return text[text.find("\n===") + 1:] if "\n===" in text else text


def checkout_script(d):
    s = [f"3\n{d['customer']}"]
    s.append("2\nc")  # clear cart
    for pid, qty in d["cart"].items():
        if qty > 0:
            s.append(f"a\n{pid}\n{qty}")
    s.append("b")
    s.append(f"4\n{d.get('coupon', '')}\n{d.get('bank', '')}")
    s.append(f"{d.get('action', '5')}\n\n\n0\n")
    return "\n".join(s) + "\n"


class H(BaseHTTPRequestHandler):
    def _send(self, body, ctype):
        b = body if isinstance(body, bytes) else body.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", ctype + "; charset=utf-8")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_GET(self):
        if self.path == "/api/data":
            cat = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
            cus = json.loads((DATA / "customers.json").read_text(encoding="utf-8"))
            self._send(json.dumps({**cat, **cus}), "application/json")
        else:
            self._send((Path(__file__).parent / "index.html").read_bytes(), "text/html")

    def do_POST(self):
        d = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        self._send(json.dumps({"output": run(checkout_script(d))}), "application/json")


if __name__ == "__main__":
    print("Open http://localhost:8000")
    HTTPServer(("localhost", 8000), H).serve_forever()
