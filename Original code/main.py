import time
from HRC import detect_intent_voice, PROJECT_ID, SESSION_ID, LANGUAGE, resposne, is_face_detected
import HRC

print("Robot is ready! Listening for conversation...")

while True:
    spoken = detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)

    if spoken:
        resposne(HRC.current_emotion)
    else:
        resposne("Neutral")

    if HRC.current_emotion == "QUIT":
        print("Shutting down")
        break

    time.sleep(0.05)
