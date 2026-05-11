import os
import time
import numpy as np
import librosa
import speech_recognition as sr
import sounddevice as sd
from scipy.io.wavfile import write
from google.cloud import dialogflow
from dotenv import load_dotenv
import serial

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
    "exit_robot": "QUIT"
}

# Stores current emotion state
current_emotion = "NEUTRAL"

# Run this once to see all microphones
"""for index, name in enumerate(sr.Microphone.list_microphone_names()):
    print(f"{index}: {name}")"""


# --- Voice tone analysis using librosa ---
def analyse_voice_emotion(wav_path):
    y, sr_rate = librosa.load(wav_path, sr=None)

    # Pitch — higher pitch = more emotional (happy/angry)
    pitches, magnitudes = librosa.piptrack(y=y, sr=sr_rate)
    pitch_values = pitches[magnitudes > np.median(magnitudes)]
    avg_pitch = float(np.mean(pitch_values)) if len(pitch_values) > 0 else 0

    # Energy — how loud/intense the speech is
    rms = librosa.feature.rms(y=y)
    avg_energy = float(np.mean(rms))

    # Speech rate — faster = more agitated
    onset_frames = librosa.onset.onset_detect(y=y, sr=sr_rate)
    speech_rate = len(onset_frames) / (len(y) / sr_rate)

    print(f"[Voice Analysis] Pitch: {avg_pitch:.1f} | Energy: {avg_energy:.4f} | Rate: {speech_rate:.1f}")

    # --- Decision logic (tune these thresholds to your microphone!) ---
    if avg_energy > 0.05 and avg_pitch > 200 and speech_rate > 4:
        return "ANGRY"
    elif avg_energy > 0.03 and avg_pitch > 150:
        return "HAPPY"
    else:
        return "NEUTRAL"


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
        print(f"Emotion state (from text): {current_emotion}")
        print("Fulfillment text: {}\n".format(response.query_result.fulfillment_text))


# --- Voice input function ---
def detect_intent_voice(project_id, session_id, language_code):
    global current_emotion
    recognizer = sr.Recognizer()

    # --- Record audio using sounddevice ---
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

        spoken_text_lower = spoken_text.lower()

        # Manual keyword overrides (highest priority)
        if "quit" in spoken_text_lower:
            current_emotion = "QUIT"

        else:
            # --- Always run both detectors ---
            voice_emotion = analyse_voice_emotion(wav_path)

            detect_intent_texts(project_id, session_id, [spoken_text], language_code)
            text_emotion = current_emotion

            print(f"[Voice emotion]: {voice_emotion} | [Text emotion]: {text_emotion}")

            if voice_emotion == text_emotion:
                # Both agree — high confidence
                current_emotion = voice_emotion
                print("✅ Both signals agree!")
            else:
                # They disagree — voice is primary, text is secondary
                print(f"⚠️ Mismatch! Voice said {voice_emotion}, text said {text_emotion} → using voice")
                current_emotion = voice_emotion

        print(f"Final emotion state: {current_emotion}")
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

    if current_emotion == "QUIT":
        print("Shutting down")
        break

    ser.write((current_emotion + "\n").encode())
    print("Arduino: ", current_emotion)