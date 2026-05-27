"""
beep_speech.py
Converts DialogFlow fulfillment text into a pitch-modulated "beep speech" WAV.
 
Takes the data your pipeline already has:
  - Fulfillment text  (e.g. "Hello! How can I help you?")
  - Emotion state     (e.g. "Neutral", "Happy", "Angry"...)
 
It does NOT speak words. It maps each word to a beep and shapes the pitch
contour using punctuation, word position and emotion, so the result sounds
like the *intonation* of the sentence ("beeep, bop bep bop?").
 
No internet, no TTS needed. Uses numpy + scipy to make the sound, and
pygame to play it (same audio library your main.py already uses).
"""
 
import re
import os
import numpy as np
from scipy.io import wavfile
 
SAMPLE_RATE = 22050
 
# --- Emotion -> sound parameters -------------------------------------------
# base_pitch : centre frequency in Hz (higher = more excited / lighter)
# pitch_range: how much pitch moves across the sentence (expressiveness)
# tempo      : seconds per beep (lower = faster, more energetic)
# waveform   : 'square' = harsh/robotic, 'sine' = soft, 'saw' = buzzy
#
# NOTE: these are the emotions the ROBOT expresses. If the user is angry,
# you will want to send "Comforting" here (a soft, slow, low profile) rather
# than "Angry". The mapping from user-emotion -> robot-emotion is done in
# main.py; this file just produces whatever emotion you ask for.
EMOTION_PROFILES = {
    "neutral":    dict(base_pitch=320, pitch_range=120, tempo=0.16, waveform="square"),
    "happy":      dict(base_pitch=440, pitch_range=260, tempo=0.12, waveform="square"),
    "sad":        dict(base_pitch=220, pitch_range=70,  tempo=0.24, waveform="sine"),
    "angry":      dict(base_pitch=300, pitch_range=200, tempo=0.10, waveform="saw"),
    "scared":     dict(base_pitch=480, pitch_range=180, tempo=0.09, waveform="saw"),
    "question":   dict(base_pitch=340, pitch_range=160, tempo=0.15, waveform="square"),
    "welcome":    dict(base_pitch=400, pitch_range=220, tempo=0.13, waveform="square"),
    # a soft, warm, slow profile for comforting an upset user
    "comforting": dict(base_pitch=250, pitch_range=90,  tempo=0.22, waveform="sine"),
}
 
 
def _osc(freq, duration, waveform):
    """Generate one beep of a given frequency, duration and timbre."""
    n = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, n, endpoint=False)
    phase = 2 * np.pi * freq * t
    if waveform == "sine":
        wave = np.sin(phase)
    elif waveform == "square":
        wave = np.sign(np.sin(phase))
    elif waveform == "saw":
        wave = 2 * (t * freq - np.floor(0.5 + t * freq))
    else:
        wave = np.sin(phase)
    # short attack/decay envelope so beeps don't click
    env = np.ones(n)
    edge = max(1, int(0.01 * SAMPLE_RATE))
    env[:edge] = np.linspace(0, 1, edge)
    env[-edge:] = np.linspace(1, 0, edge)
    return wave * env * 0.6
 
 
def _silence(duration):
    return np.zeros(int(SAMPLE_RATE * duration))
 
 
def text_to_beeps(text, emotion="neutral"):
    """
    Convert fulfillment text into a beep-speech waveform (numpy array).
 
    The pitch CONTOUR follows the sentence:
      - rises toward the end if it's a question ('?')
      - falls toward the end for a statement ('.')
      - longer words get slightly longer beeps
      - emotion sets the overall pitch, range, tempo and timbre
    """
    profile = EMOTION_PROFILES.get(str(emotion).lower(), EMOTION_PROFILES["neutral"])
 
    is_question = "?" in text
    if is_question and str(emotion).lower() == "neutral":
        profile = EMOTION_PROFILES["question"]
 
    tokens = re.findall(r"[A-Za-z']+|[,.!?]", text)
    words = [tok for tok in tokens if tok.isalpha()]
    if not words:
        return _silence(0.2)
 
    n = len(words)
    audio = []
    word_index = 0
 
    for tok in tokens:
        if tok in ",.!?":
            audio.append(_silence(0.18 if tok == "," else 0.30))
            continue
 
        pos = word_index / max(1, n - 1)
 
        if is_question:
            # rising contour: low start, high end (question intonation)
            contour = -0.4 + 1.0 * pos
        else:
            # gentle arc: rise then fall, ending lower (statement intonation)
            contour = np.sin(np.pi * pos) * 0.6 - pos * 0.5
 
        freq = profile["base_pitch"] + contour * profile["pitch_range"]
        freq = max(120, freq)
 
        dur = profile["tempo"] * (0.7 + 0.5 * min(len(tok) / 6, 1.5))
 
        audio.append(_osc(freq, dur, profile["waveform"]))
        audio.append(_silence(profile["tempo"] * 0.35))
        word_index += 1
 
    return np.concatenate(audio)
 
 
def save_wav(waveform, path):
    """Write a numpy waveform to a 16-bit WAV file."""
    data = np.clip(waveform, -1, 1)
    wavfile.write(path, SAMPLE_RATE, (data * 32767).astype(np.int16))
 
 
def speak_beeps(text, emotion="neutral", out_path="robot_reply.wav"):
    """
    Generate beep speech from text + emotion, save it, and play it
    through pygame (the same mixer your HRC.py already initialised).
 
    Call this instead of playing a pre-recorded mp3.
    """
    import pygame  # already initialised in HRC.py via pygame.mixer.init()
 
    # nothing to say -> don't crash, just skip
    if not text or not str(text).strip():
        print("⚠️ speak_beeps: no text given, skipping")
        return
 
    # On Windows, pygame keeps the WAV file locked after playing it.
    # Unload it BEFORE writing a new one, or save_wav() gets PermissionError.
    try:
        pygame.mixer.music.unload()
    except Exception:
        pass  # unload() not available on very old pygame; ignore
 
    wave = text_to_beeps(text, emotion)
 
    try:
        save_wav(wave, out_path)
    except PermissionError:
        # file still locked for some reason -> use a fallback filename
        out_path = "robot_reply_alt.wav"
        save_wav(wave, out_path)
 
    if os.path.exists(out_path):
        pygame.mixer.music.load(out_path)
        pygame.mixer.music.play()
        # wait until the beeps finish so it doesn't get cut off
        while pygame.mixer.music.get_busy():
            pygame.time.Clock().tick(10)
    else:
        print(f"⚠️ Could not create beep file at {out_path}")
 
 
# --- demo (only runs if you run THIS file directly) ------------------------
if __name__ == "__main__":
    examples = [
        ("Hello! How can I help you?",            "neutral"),
        ("Hi! How are you doing?",                 "happy"),
        ("Good day! What can I do for you today?", "neutral"),
        ("It is okay, take your time.",            "comforting"),
    ]
    for i, (text, emo) in enumerate(examples, 1):
        wave = text_to_beeps(text, emo)
        out = f"beep_demo_{i}_{emo}.wav"
        save_wav(wave, out)
        print(f"[{emo:10s}] {text!r:45s} -> {out}  ({len(wave)/SAMPLE_RATE:.2f}s)")