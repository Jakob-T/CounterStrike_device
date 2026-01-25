import os
import json
import time
import signal

import tornado.ioloop
import tornado.web
import tornado.websocket
import paho.mqtt.client as mqtt

MQTT_HOST = "PI_ADRESS"
MQTT_PORT = 1883
TOPIC_CMD = "cosplay/bomb/cmd"

clients = set()

m = mqtt.Client()
m.connect(MQTT_HOST, MQTT_PORT, 60)
m.loop_start()


def broadcast(obj: dict):
    payload = json.dumps(obj)
    dead = []

    for c in clients:
        try:
            c.write_message(payload)
        except Exception:
            dead.append(c)

    for c in dead:
        clients.discard(c)


class MainHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html")


class WSHandler(tornado.websocket.WebSocketHandler):
    def open(self):
        clients.add(self)
        self.write_message(json.dumps({"type": "hello", "ts": time.time()}))

    def on_close(self):
        clients.discard(self)

    def on_message(self, message):
        msg = message.strip()

        if msg.startswith("{"):
            try:
                obj = json.loads(msg)
                msg = str(obj.get("cmd", "")).strip()
            except Exception:
                self.write_message(json.dumps({"type": "error", "msg": "bad json"}))
                return

        cmd = msg.upper()

        if cmd in ("ARM", "RESET"):
            m.publish(TOPIC_CMD, cmd)
            broadcast({"type": "cmd_sent", "cmd": cmd, "ts": time.time()})
        else:
            self.write_message(json.dumps({"type": "error", "msg": "unknown cmd"}))


def make_app():
    base = os.path.dirname(__file__)
    return tornado.web.Application(
        [(r"/", MainHandler), (r"/ws", WSHandler)],
        template_path=base,
        static_path=base,
        debug=True,
    )


def shutdown():
    print("Shutting down cleanly...")
    try:
        m.loop_stop()
        m.disconnect()
    except Exception:
        pass
    tornado.ioloop.IOLoop.current().stop()


signal.signal(signal.SIGTERM, lambda s, f: shutdown())
signal.signal(signal.SIGINT, lambda s, f: shutdown())


if __name__ == "__main__":
    app = make_app()
    app.listen(8890)
    print(f"Web: http://{MQTT_HOST}:8890")

    tornado.ioloop.IOLoop.current().start()
