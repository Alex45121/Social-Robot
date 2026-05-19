from HRC import detect_intent_voice, PROJECT_ID, SESSION_ID, LANGUAGE, current_emotion
from arduino_control import send_command

print("🤖 Robot is ready! Start speaking")

while True:
    spoken_text = detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)
    
    if current_emotion == "QUIT":
        print("👋 Shutting down robot...")
        break