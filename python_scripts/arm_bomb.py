import time
import serial
import pymysql
import sys
import fcntl

from paho.mqtt.client import Client


# ---------- Single instance lock ----------
lockfile = open("/tmp/arm_bomb.lock", "w")

try:
    fcntl.flock(lockfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    print("arm_bomb.py already running, exiting.")
    sys.exit(1)


# ---------- MQTT ----------
MQTT_HOST = "PI_ADDRESS"
MQTT_PORT = 1883

TOPIC_DISTANCE = "cosplay/bomb/arm"
TOPIC_CMD = "cosplay/bomb/cmd"


# ---------- Serial ----------
SERIAL_PORT = "/dev/ttyACM0"
BAUD = 9600


# ---------- Logic ----------
THRESHOLD_CM = 10
COOLDOWN_S = 5
last_arm = 0


# ---------- DB ----------
DB_HOST = "localhost"
DB_USER = "sensor_writer"
DB_PASS = "rpi18"
DB_NAME = "cosplay"


try:
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=0.1)
except Exception as e:
    print(f"ERROR: cannot open serial {SERIAL_PORT}: {e}")
    sys.exit(1)


def db_insert(source: str, value: int | None, note: str | None):
    con = pymysql.connect(
        host=DB_HOST,
        user=DB_USER,
        password=DB_PASS,
        database=DB_NAME,
    )
    try:
        with con.cursor() as cur:
            cur.execute(
                "INSERT INTO armLog(ts, source, value, note) VALUES (NOW(), %s, %s, %s)",
                (source, value, note),
            )
        con.commit()
    finally:
        con.close()


def send_arduino(cmd: str, note: str | None = None):
    ser.write((cmd.strip() + "\n").encode("utf-8"))
    if note:
        print(f"to Arduino: {cmd} ({note})")
    else:
        print(f"to Arduino: {cmd}")


def on_connect(client, userdata, flags, rc):
    print("MQTT connected rc=", rc)
    client.subscribe(TOPIC_DISTANCE)
    client.subscribe(TOPIC_CMD)


def on_message(client, userdata, msg):
    global last_arm

    topic = msg.topic
    payload = msg.payload.decode("utf-8", errors="ignore").strip()

    # Web socket to MQTT to Arduino
    if topic == TOPIC_CMD:
        cmd = payload.upper()
        if cmd in ("ARM", "RESET"):
            send_arduino(cmd, note="from web cmd")
            db_insert("web", None, f"cmd={cmd}")
        else:
            print("Unknown cmd:", payload)
        return

    # Distance trigger with ultrasonci sensor
    if topic == TOPIC_DISTANCE:
        try:
            cm = int(payload)
        except ValueError:
            return

        now = time.time()
        if cm > 0 and cm < THRESHOLD_CM and (now - last_arm) > COOLDOWN_S:
            last_arm = now
            send_arduino("ARM", note=f"auto distance cm={cm}")
            db_insert("mqtt", cm, "auto-arm request by distance")


def poll_arduino_events():
    """
    Reads Arduino Serial println events:
      ARMED keypad
      ARMED serial
      BOOM
      RESET_DONE
    and logs them into DB.
    """
    try:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
    except Exception:
        return

    if not line:
        return

    print("from Arduino:", line)

    # Parse events
    if line.startswith("ARMED"):
        parts = line.split(maxsplit=1)
        src = parts[1] if len(parts) > 1 else "unknown"
        db_insert("arduino", None, f"armed via {src}")
    elif line == "BOOM":
        db_insert("arduino", None, "boom")
    elif line == "RESET_DONE":
        db_insert("arduino", None, "reset done")


client = Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_HOST, MQTT_PORT, 60)

print("Bridge running (MQTT <-> Serial + DB)... Ctrl+C to stop")

try:
    while True:
        client.loop(timeout=0.1)
        poll_arduino_events()
        time.sleep(0.02)
except KeyboardInterrupt:
    print("Stopping bridge...")
finally:
    try:
        client.disconnect()
    except Exception:
        pass
    try:
        ser.close()
    except Exception:
        pass