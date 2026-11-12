# Note from the developer:
#  Caution: This is a chatGPT-generated code. It may not be fully functional or secure.
# This code runs a simple development server for testing the web UI without an actual ESP32 device.
# Run this script in terminal:
#      python3 dev_testing_server.py 
# then access web UI on http://localhost:8000 
from http.server import HTTPServer, SimpleHTTPRequestHandler
import json
from datetime import datetime


# Mock ESP32 state
device_state = {
    "load_status": False,
    "wifi_status": True
}


class GeyserHandler(SimpleHTTPRequestHandler):

    def do_GET(self):

        # Mock ESP32 status API
        if self.path == "/api/status":

            response = {
                "esp32_time": datetime.now().strftime(
                    "%Y-%m-%d %H:%M:%S"
                ),
                "load_status": device_state["load_status"],
                "wifi_status": device_state["wifi_status"]
            }

            self.send_json_response(response)

            print(
                f"GET /api/status -> "
                f"load={'ON' if device_state['load_status'] else 'OFF'}"
            )

            return

        # Everything else is handled by the normal HTTP server
        super().do_GET()


    def do_POST(self):

        # Mock geyser control API
        if self.path == "/api/geyser":

            content_length = int(
                self.headers.get("Content-Length", 0)
            )

            body = self.rfile.read(content_length)

            try:
                request = json.loads(body)

            except json.JSONDecodeError:

                self.send_json_response(
                    {
                        "success": False,
                        "message": "Invalid JSON"
                    },
                    status=400
                )

                return

            # Check that state exists
            if "state" not in request:

                self.send_json_response(
                    {
                        "success": False,
                        "message": "Missing state"
                    },
                    status=400
                )

                return

            # Update mock geyser state
            device_state["load_status"] = bool(
                request["state"]
            )

            state_text = (
                "ON"
                if device_state["load_status"]
                else "OFF"
            )

            print(
                f"POST /api/geyser -> Geyser {state_text}"
            )

            self.send_json_response(
                {
                    "success": True,
                    "message":
                        f"Geyser {state_text} command accepted"
                }
            )

            return

        # Unknown POST endpoint
        self.send_error(404, "Not Found")


    def send_json_response(self, data, status=200):

        response = json.dumps(data).encode("utf-8")

        self.send_response(status)

        self.send_header(
            "Content-Type",
            "application/json"
        )

        self.send_header(
            "Content-Length",
            str(len(response))
        )

        self.end_headers()

        self.wfile.write(response)


if __name__ == "__main__":

    server_address = ("", 8000)

    httpd = HTTPServer(
        server_address,
        GeyserHandler
    )

    print()
    print("Smart Geyser development server")
    print("Listening on http://localhost:8000")
    print()
    print("Press Ctrl+C to stop")
    print()

    httpd.serve_forever()