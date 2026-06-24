import time
from HRC import detect_intent_voice, PROJECT_ID, SESSION_ID, LANGUAGE, resposne, is_face_detected
import HRC

state = "IDLE"
print("Robot is ready! Looking for a face...")

while True:
    if state == "IDLE":
        if is_face_detected():
            print("👀 Face detected → start listening")
            state = "LISTENING"

    elif state == "LISTENING":
        detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)
        resposne(HRC.current_emotion)

        if HRC.current_emotion == "QUIT":
            print("Shutting down")
            break

        if not is_face_detected():
            print("🚶 Person left → back to idle")
            state = "IDLE"

    time.sleep(0.05)
