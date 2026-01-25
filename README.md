# CounterStrike_deviceCosplay Bomb – IoT Control & Monitoring System

This project implements an event-driven IoT system for controlling and monitoring a “cosplay bomb” using MQTT, Raspberry Pi, Arduino, a WebSocket-based web UI, and a MySQL/MariaDB database.

--------------------------------------------------
SYSTEM ARCHITECTURE
--------------------------------------------------

Web Browser
  > WebSocket (Tornado)
  > Raspberry Pi (ws_bomb.py)
  > MQTT publish (cosplay/bomb/cmd)
  > MQTT Broker (Mosquitto)
  > MQTT subscribe
  > Raspberry Pi (arm_bomb.py)
  > Serial / USB (/dev/ttyACM0)
  > Arduino (keypad + ultrasonic sensor)

Event sources:
- Web UI (ARM / RESET)
- Ultrasonic distance sensor (auto-arm trigger)
- Arduino keypad (manual ARM)

All events are forwarded to the Arduino and logged into the database as the system ground truth.

--------------------------------------------------
PROJECT STRUCTURE
--------------------------------------------------

ws_bomb.py        Tornado WebSocket server (Web UI -> MQTT)
arm_bomb.py       MQTT <-> Serial bridge + database logging
start_all.sh      Bash script to start and stop all services
requirements.txt  Python dependencies
index.html        Web UI
README.txt        This document

--------------------------------------------------
COMPONENT DESCRIPTION
--------------------------------------------------

ws_bomb.py
- Tornado HTTP + WebSocket server
- Receives ARM / RESET commands from the browser
- Publishes MQTT messages to cosplay/bomb/cmd
- Does NOT access the serial port directly

arm_bomb.py
- Subscribes to MQTT command and distance topics
- Handles serial communication with Arduino
- Logs all events to MySQL/MariaDB
- Implements cooldown and distance threshold logic
- Uses a single-instance lock (/tmp/arm_bomb.lock)

start_all.sh
- Starts ws_bomb.py and arm_bomb.py
- Catches Ctrl+C (SIGINT / SIGTERM)
- Cleanly shuts down all processes

--------------------------------------------------
TECHNOLOGIES USED
--------------------------------------------------

Python 3
Tornado (WebSocket server)
MQTT (Mosquitto + paho-mqtt)
Arduino + Serial communication (pyserial)
MySQL / MariaDB (pymysql)
Linux / Bash / system signals

--------------------------------------------------
INSTALLATION
--------------------------------------------------

1. System packages (Raspberry Pi):

sudo apt update
sudo apt install mosquitto mariadb-server python3 python3-venv

2. Python virtual environment:

python3 -m venv env
source env/bin/activate
pip install -r requirements.txt

3. Database setup:

CREATE DATABASE cosplay;

USE cosplay;

CREATE TABLE armLog (
  id INT AUTO_INCREMENT PRIMARY KEY,
  ts DATETIME NOT NULL,
  source VARCHAR(32) NOT NULL,
  value INT NULL,
  note VARCHAR(255) NULL
);

GRANT INSERT, SELECT ON cosplay.*
TO 'sensor_writer'@'localhost' IDENTIFIED BY 'rpi18';
FLUSH PRIVILEGES;

--------------------------------------------------
RUNNING THE SYSTEM
--------------------------------------------------

Recommended:

./start_all.sh

This starts all required services and allows a single Ctrl+C to shut down the entire system cleanly.

Manual (for debugging):

python3 arm_bomb.py
python3 ws_bomb.py

--------------------------------------------------
WEB UI
--------------------------------------------------

URL: http://<PI_IP>:8890
WebSocket: ws://<PI_IP>:8890/ws

Available commands:
- ARM
- RESET

--------------------------------------------------
DATABASE LOGGING
--------------------------------------------------

All events are stored in the armLog table:
- Web ARM / RESET commands
- MQTT auto-arm events (distance-based)
- Arduino keypad actions
- BOOM / RESET events

Example query:

SELECT * FROM armLog ORDER BY ts DESC;

--------------------------------------------------
USED COMPONENTS
--------------------------------------------------

The following hardware components were used to build and integrate the system:

Microcontroller & Power

- Arduino – main hardware controller

- Protoboard / Breadboard – circuit prototyping

- Breadboard Power Supply MB102 – regulated power supply for components

Input Devices

- 4x4 Matrix Keypad – manual ARM / RESET input

- HC-SR04 Ultrasonic Distance Sensor – proximity-based auto-arm trigger

Output & Feedback

- I2C LCD display – system status display

- Piezo Buzzer – audio feedback and alarm signaling

- LED Diodes – visual state indicators

Connectivity & Wiring

- Jumper Wires

These components together enable manual and automated control, real-time feedback, and reliable tracking of all system events.

--------------------------------------------------
STABILITY AND SAFETY
--------------------------------------------------

- Single-instance protection using file locking
- Cooldown logic to prevent command spam
- Only one process accesses the serial port
- Clean shutdown via signal handling

--------------------------------------------------
SUMMARY
--------------------------------------------------

This project demonstrates:
- Distributed IoT architecture
- Event-driven system design
- Bridging network communication with microcontrollers
- Real-time web-based control
- Reliable event logging

Suitable for CVs, demos, cosplay projects, and educational use.
