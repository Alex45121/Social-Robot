import serial
import time

ser = serial.Serial('COM4', 115200, timeout=1)
time.sleep(2)

while True:
    msg = "TEST\n"
    ser.write(msg.encode())
    print("Sent:", msg.strip())

    response = ser.readline().decode().strip()
    if response:
        print("Arduino says:", response)

    time.sleep(1)