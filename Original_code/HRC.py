import os
import speech_recognition as sr
from google.cloud import dialogflow
from dotenv import load_dotenv

# --- Load settings from .env file ---
load_dotenv()
credential_path = os.getenv("CREDENTIAL_PATH")
PROJECT_ID = os.getenv("PROJECT_ID")
SESSION_ID = os.getenv("SESSION_ID")
LANGUAGE = os.getenv("LANGUAGE")
os.environ['GOOGLE_APPLICATION_CREDENTIALS'] = credential_path


EMOTION_MAP = {
    "happy_response": "HAPPY",
    "neutral_response": "NEUTRAL",
    "angry_response": "ANGRY",
    "Default Welcome Intent": "HAPPY",
    "Default Fallback Intent": "NEUTRAL",
}

# Stores current emotion state
current_emotion = "NEUTRAL"

# Run this once to see all microphones
"""for index, name in enumerate(sr.Microphone.list_microphone_names()):
    print(f"{index}: {name}")"""

# --- Text input function ---
def detect_intent_texts(project_id, session_id, texts, language_code):
    global current_emotion
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
        current_emotion = EMOTION_MAP.get(intent_name, "NEUTRAL")

        print("=" * 20)
        print("Query text: {}".format(response.query_result.query_text))
        print(f"Detected intent: {intent_name} (confidence: {confidence})\n")
        print(f"Emotion state: {current_emotion}")
        print("Fulfillment text: {}\n".format(response.query_result.fulfillment_text))

# --- Voice input function ---
def detect_intent_voice(project_id, session_id, language_code):
    recognizer = sr.Recognizer()
    with sr.Microphone(device_index=1) as source:
        print("🔧 Adjusting for background noise... please wait")
        recognizer.adjust_for_ambient_noise(source, duration=2)
        print("🎤 Speak now clearly...")
        try:
            audio = recognizer.listen(source, timeout=5, phrase_time_limit=7)
            print("Processing your speech...")
        except sr.WaitTimeoutError:
            print("❌ No speech detected. Try again.")
            return

    try:
        spoken_text = recognizer.recognize_google(audio)
        print(f"✅ You said: {spoken_text}")
        detect_intent_texts(project_id, session_id, [spoken_text], language_code)
    except sr.UnknownValueError:
        print("❌ Could not understand. Please speak louder and more clearly.")
    except sr.RequestError as e:
        print(f"❌ Google API error: {e}")

# --- RUN ---
print("Choose test mode:")
print("1 - Type text")
print("2 - Use microphone")
choice = input("Enter 1 or 2: ")

if choice == "1":
    text = input("Type something: ")
    detect_intent_texts(PROJECT_ID, SESSION_ID, [text], LANGUAGE)
elif choice == "2":
    detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)