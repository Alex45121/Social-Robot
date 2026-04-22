import serial
import time
import speech_recognition as sr

ser = serial.Serial('COM4', 115200, timeout=1)
time.sleep(2)

recognizer = sr.Recognizer()

print("Ready")

while True:
    with sr.Microphone() as source:
        print("🎤 Say something...")
        recognizer.adjust_for_ambient_noise(source, duration=0.2)
        audio = recognizer.listen(source)

    try:
        text = recognizer.recognize_google(audio)
        print("You said:", text)

        if text:
            ser.reset_input_buffer()   # 👈 IMPORTANT: clears old junk
            ser.write(b"B\n")
            ser.flush()

            # read Arduino response safely
            time.sleep(0.1)
            response = ser.readline().decode().strip()
            if response:
                print("Arduino:", response)

    except Exception as e:
        print("Didn't catch that:", e)