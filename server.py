from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
import os
import subprocess
import json
import re
import tempfile

ROOT = Path(__file__).resolve().parent
SCANNER = ROOT / "build" / ("lex_style_scanner.exe" if os.name == "nt" else "lex_style_scanner")
HTML = ROOT / "web" / "index.html"

class Handler(BaseHTTPRequestHandler):
    def send_data(self, status, data, content_type="application/json; charset=utf-8"):
        if isinstance(data, bytes):
            body = data
        elif isinstance(data, str):
            body = data.encode("utf-8")
        else:
            body = json.dumps(data).encode("utf-8")

        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self.send_data(200, HTML.read_bytes(), "text/html; charset=utf-8")
        else:
            self.send_data(404, {"error": "Not found"})

    def do_POST(self):
        if self.path != "/run":
            self.send_data(404, {"error": "Not found"})
            return

        input_path = None
        try:
            length = int(self.headers.get("Content-Length", "0"))
            data = json.loads(self.rfile.read(length).decode("utf-8"))
            code = data.get("code", "")

            if not isinstance(code, str) or len(code) > 50000:
                self.send_data(400, {"error": "Input must be text under 50,000 characters."})
                return

            if not SCANNER.exists():
                self.send_data(500, {"error": "Scanner executable not found."})
                return

            with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", suffix=".txt",
                delete=False, dir=ROOT
            ) as f:
                f.write(code)
                input_path = Path(f.name)

            result = subprocess.run(
                [str(SCANNER), str(input_path)],
                capture_output=True, text=True, timeout=10,
                cwd=str(ROOT), errors="replace"
            )

            lines = result.stdout.splitlines()
            tokens = []
            errors = []

            for line in lines:
                if line.startswith("LEXICAL_ERROR:"):
                    errors.append(line)
                else:
                    match = re.match(r"^([A-Z_]+): (.*)$", line)
                    if match:
                        tokens.append({
                            "type": match.group(1),
                            "lexeme": match.group(2)
                        })

            self.send_data(200, {
                "tokens": tokens,
                "errors": errors,
                "returncode": result.returncode,
                "stderr": result.stderr
            })

        except subprocess.TimeoutExpired:
            self.send_data(500, {"error": "Scanner timed out."})
        except Exception as exc:
            self.send_data(500, {"error": str(exc)})
        finally:
            if input_path and input_path.exists():
                input_path.unlink()

    def log_message(self, format, *args):
        pass

if __name__ == "__main__":
    if not HTML.exists():
        raise SystemExit("UI file missing: web/index.html")
    print("LEX Tool running on port", os.environ.get("PORT", "8000"))
    print("Open this address in your browser. Press Ctrl+C to stop.")
    HTTPServer(("0.0.0.0", int(os.environ.get("PORT", "8000"))), Handler).serve_forever()
