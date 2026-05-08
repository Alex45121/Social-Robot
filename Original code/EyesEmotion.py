import os
import speech_recognition as sr
from google.cloud import dialogflow
from dotenv import load_dotenv
import serial
import time
import sounddevice as sd
from scipy.io.wavfile import write

ser = serial.Serial('COM4', 115200)  # change COM port!
time.sleep(2)

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
    "exit_robot" : "QUIT"
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
# --- Voice input function ---
def detect_intent_voice(project_id, session_id, language_code):
    global current_emotion
    recognizer = sr.Recognizer()

    # --- record audio using sounddevice ---
    samplerate = 16000
    duration = 6

    print("🎤 Recording... speak now")
    audio = sd.rec(int(duration * samplerate),
                   samplerate=samplerate,
                   channels=1,
                   dtype='int16')
    sd.wait()

    wav_path = "temp.wav"
    write(wav_path, samplerate, audio)
    print("✅ Recording saved, processing...")

    # --- SpeechRecognition ---
    with sr.AudioFile(wav_path) as source:
        audio_data = recognizer.record(source)

    try:
        spoken_text = recognizer.recognize_google(audio_data)
        print(f"✅ You said: {spoken_text}")

        # Convert to lowercase for easier checking
        spoken_text_lower = spoken_text.lower()

        # Manual emotion overrides
        if "angry" in spoken_text_lower:
            current_emotion = "A"

        elif "happy" in spoken_text_lower:
            current_emotion = "H"

        elif "neutral" in spoken_text_lower:
            current_emotion = "N"

        elif "quit" in spoken_text_lower:
            current_emotion = "QUIT"

        else:
            # Use Dialogflow normally
            detect_intent_texts(project_id, session_id, [spoken_text], language_code)

        print(f"Emotion state: {current_emotion}")

        return spoken_text

    except sr.UnknownValueError:
        print("❌ Could not understand. Please speak louder and more clearly.")
        return None

    except sr.RequestError as e:
        print(f"❌ Google API error: {e}")
        return None


# --- RUN ---
print("Robot is ready! Start Speaking")

while True:
    spoken_text = detect_intent_voice(PROJECT_ID, SESSION_ID, LANGUAGE)

    if current_emotion != "QUIT":
        ser.write((current_emotion + "\n").encode())
        print("Arduino: ", current_emotion)