import sys
sys.stdout.reconfigure(encoding='utf-8')
import os
import speech_recognition as sr
from google.cloud import dialogflow
from dotenv import load_dotenv
import pygame
pygame.mixer.init()
import serial
import time
 
# CHANGED 1: import the beep synthesizer
from beep_speech import speak_beeps
 
COM_PORT = "COM3"
 
 
# --- Load settings from .env file ---
load_dotenv()
credential_path = os.getenv("CREDENTIAL_PATH")
PROJECT_ID = os.getenv("PROJECT_ID")
SESSION_ID = os.getenv("SESSION_ID")
LANGUAGE = os.getenv("LANGUAGE")
os.environ['GOOGLE_APPLICATION_CREDENTIALS'] = credential_path
 
try:
    ser = serial.Serial(COM_PORT, 115200)
    time.sleep(2)
    print(f"✅ Arduino connected on {COM_PORT}")
except:
    ser = None
    print("⚠️ Arduino not connected — running without hardware")
 
 
EMOTION_MAP = {
    "happy_response":       "Happy",
    "neutral_response":     "Neutral",
    "angry_response":       "Angry",
    "scared_response":      "Scared",
    "sad_response":         "Sad",
    "question_response":    "Question",
    "welcome_response":     "Welcome",
    "exit_robot":           "QUIT"
}
 
# CHANGED 2: map the USER's detected emotion to how the ROBOT should RESPOND.
# We don't mirror the user. If the user is angry/sad/scared, the robot
# responds in a comforting way instead. (You can tune this later.)
ROBOT_RESPONSE_EMOTION = {
    "Happy":    "happy",
    "Neutral":  "neutral",
    "Angry":    "comforting",   # user angry  -> robot comforts
    "Scared":   "comforting",   # user scared -> robot comforts
    "Sad":      "comforting",   # user sad    -> robot comforts
    "Question": "question",
    "Welcome":  "welcome",
    "QUIT":     "neutral"
}
 
# Stores current emotion state
current_emotion = "Neutral"
# CHANGED 2b: also store the fulfillment text so resposne() can use it.
# main.py doesn't need to know about this — resposne() reads it as a global.
current_fulfillment = ""

def is_face_detected():
    if not ser:
        return False
    latest = None
    while ser.in_waiting:
        line = ser.readline().decode(errors="ignore").strip()
        if line in ("FACE:1", "FACE:0"):
            latest = line
    if latest == "FACE:1":
        return True
    elif latest == "FACE:0":
        return False
    return False  # no new data this cycle
 
# Run this once to see all microphones
"""for index, name in enumerate(sr.Microphone.list_microphone_names()):
    print(f"{index}: {name}")"""
 
 
def resposne(emotion):
    # NOTE: signature is unchanged — main.py still calls resposne(HRC.current_emotion).
    # The fulfillment text is read from the module global current_fulfillment.
 
    # the Arduino still gets the same single-letter command as before
    arduino_emotion = {
    "Happy":    "HAPPY",
    "Neutral":  "NEUTRAL",
    "Angry":    "ANGRY",
    "Scared":   "SUPRISED",   # arduino has no "scared" → use SUPRISED
    "Sad":      "SAD",
    "Question": "NEUTRAL",    # arduino has no "question" → fallback
    "Welcome":  "HAPPY",      # arduino has no "welcome" → use HAPPY
    "QUIT":     "NEUTRAL"
    }
 
    # CHANGED 3: instead of loading a pre-recorded mp3, generate beeps
    # from the fulfillment text using the comforting/robot emotion.
    robot_emotion = ROBOT_RESPONSE_EMOTION.get(emotion, "neutral")
    if current_fulfillment and current_fulfillment.strip():
        speak_beeps(current_fulfillment, robot_emotion, out_path="robot_reply.wav")
    else:
        print("⚠️ No fulfillment text to speak")
 
    emo = arduino_emotion.get(emotion, "NEUTRAL")
    if ser:
        message = f"90,20,{emo},\n"
        ser.write(message.encode())
        print(f"📡 Sent to Arduino: {message.strip()}")
 
# --- Text input function ---
def detect_intent_texts(project_id, session_id, texts, language_code):
    global current_emotion, current_fulfillment   # CHANGED: also set fulfillment
    session_client = dialogflow.SessionsClient()
    session = session_client.session_path(project_id, session_id)
    print("Session path: {}\n".format(session))
 
    for text in texts:
        text_input = dialogflow.TextInput(text=text, language_code=language_code)
        query_input = dialogflow.QueryInput(text=text_input)
        response = session_client.detect_intent(
            request={"session": session, "query_input": query_input}
        )
        intent_name = response.query_result.intent.display_name
        confidence = response.query_result.intent_detection_confidence
        current_emotion = EMOTION_MAP.get(intent_name, "Neutral")
        current_fulfillment = response.query_result.fulfillment_text  # CHANGED
 
        print("=" * 20)
        print("Query text: {}".format(response.query_result.query_text))
        print(f"Detected intent: {intent_name} (confidence: {confidence})\n")
        print(f"Emotion state: {current_emotion}")
        print("Fulfillment text: {}\n".format(current_fulfillment))
 
 
# --- Voice input function ---
def detect_intent_voice(project_id, session_id, language_code):
    global current_fulfillment
    # clear last turn's text: if this listen fails, resposne() will see ""
    # and skip instead of replaying the previous sentence.
    current_fulfillment = ""
 
    recognizer = sr.Recognizer()
    with sr.Microphone(device_index=1) as source:
        print("🔧 Adjusting for background noise... please wait")
        recognizer.adjust_for_ambient_noise(source, duration=1)
        print("🎤 Speak now clearly...")
        try:
            audio = recognizer.listen(source, timeout=5, phrase_time_limit=7)
            print("Processing your speech...")
        except sr.WaitTimeoutError:
            print("❌ No speech detected. Try again.")
            return None
 
    try:
        spoken_text = recognizer.recognize_google(audio)
        print(f"✅ You said: {spoken_text}")
        detect_intent_texts(project_id, session_id, [spoken_text], language_code)
        return spoken_text
    except sr.UnknownValueError:
        print("❌ Could not understand. Please speak louder and more clearly.")
        return None
    except sr.RequestError as e:
        print(f"❌ Google API error: {e}")
        return None
 
 

 
if __name__ == "__main__":
    print("Robot is ready! Start Speaking")
    while True:
        spoken_text = detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)
        resposne(current_emotion)
        if current_emotion == "QUIT":
            print("Shutting down")
            break