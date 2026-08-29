import json
import os
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data"))
latest_data = {
    "name": "เจ้าหน้าที่อุทยาน01",
    "spo2": 0,
    "hr": 0,
    "status": 0,
    "lat": 0,
    "lon": 0,
    "hasData": False,
    "ageSec": 0,
    "spo2History": [],
    "hrHistory": [],
    "updatedAt": 0,
}


def number_or(value, fallback):
    try:
        n = float(value)
        return n if n == n and n != float("inf") and n != float("-inf") else fallback
    except (TypeError, ValueError):
        return fallback


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=ROOT, **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        if self.path == "/api/data":
            self._handle_api()
            return
        super().do_GET()

    def do_POST(self):
        if self.path == "/api/data":
            self._handle_api()
            return
        self.send_error(404, "File not found")

    def _handle_api(self):
        global latest_data

        if self.command == "POST":
            length = int(self.headers.get("Content-Length", "0"))
            raw = self.rfile.read(length) if length > 0 else b""
            try:
                body = json.loads(raw.decode("utf-8")) if raw else {}
            except json.JSONDecodeError:
                body = {}
            spo2 = number_or(body.get("spo2"), latest_data["spo2"])
            hr = number_or(body.get("hr"), latest_data["hr"])
            latest_data = {
                "name": latest_data.get("name", "เจ้าหน้าที่อุทยาน01"),
                "spo2": spo2,
                "hr": hr,
                "status": max(0, min(3, int(number_or(body.get("status"), 0)))),
                "lat": number_or(body.get("lat"), latest_data["lat"]),
                "lon": number_or(body.get("lon"), latest_data["lon"]),
                "hasData": True,
                "ageSec": 0,
                "spo2History": (latest_data["spo2History"] + [spo2])[-20:],
                "hrHistory": (latest_data["hrHistory"] + [hr])[-20:],
                "updatedAt": __import__("time").time() * 1000,
            }

        age_sec = 0
        if latest_data.get("updatedAt"):
            age_sec = int((__import__("time").time() * 1000 - latest_data["updatedAt"]) / 1000)

        payload = {**latest_data, "ageSec": age_sec}
        body = json.dumps(payload).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


if __name__ == "__main__":
    port = 5501
    print(f"Local preview server running at http://localhost:{port}")
    print(f"API: http://localhost:{port}/api/data")
    httpd = ThreadingHTTPServer(("0.0.0.0", port), Handler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()
