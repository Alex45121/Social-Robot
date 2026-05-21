from HRC import detect_intent_voice, PROJECT_ID, SESSION_ID, LANGUAGE, resposne
"""from arduino_control import send_command"""
from huskylense import face_detection
import HRC

state = "IDLE"

while True:
    if state == "IDLE":
        if face_detection() == True:
            print("Face detected → start listening")
            state = "LISTENING"

    elif state == "LISTENING":
        detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)
        resposne(HRC.current_emotion)
        if face_detection() == False:  
            print("No person detected - Stops responding")    # person walked away
            state = "IDLE"