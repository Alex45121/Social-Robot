"""
beep_speech.py
Converts DialogFlow fulfillment text into a pitch-modulated "beep speech" WAV.
 
 
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
    "neutral": dict(
        base_pitch=320,
        pitch_range=120,
        tempo=0.16,
        waveform="square"
    ),

    "happy": dict(
        base_pitch=520,
        pitch_range=420,
        tempo=0.09,
        waveform="square"
    ),

    "sad": dict(
        base_pitch=180,
        pitch_range=60,
        tempo=0.28,
        waveform="sine"
    ),

    "angry": dict(
        base_pitch=280,
        pitch_range=260,
        tempo=0.08,
        waveform="saw"
    ),

    "scared": dict(
        base_pitch=650,
        pitch_range=380,
        tempo=0.07,
        waveform="saw"
    ),

    "question": dict(
        base_pitch=340,
        pitch_range=260,
        tempo=0.13,
        waveform="square"
    ),

    "welcome": dict(
        base_pitch=480,
        pitch_range=320,
        tempo=0.11,
        waveform="square"
    ),

    "comforting": dict(
        base_pitch=220,
        pitch_range=80,
        tempo=0.24,
        waveform="sine"
    ),
}
 
 
def _osc(freq, duration, waveform,
         end_freq=None,
         vibrato_rate=0,
         vibrato_depth=0):

    n = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, n, endpoint=False)

    if end_freq is None:
        end_freq = freq

    # Frequency sweep
    freqs = np.linspace(freq, end_freq, n)

    # Vibrato
    if vibrato_rate > 0 and vibrato_depth > 0:
        freqs *= (
            1
            + vibrato_depth
            * np.sin(2 * np.pi * vibrato_rate * t)
        )

    phase = np.cumsum(2 * np.pi * freqs / SAMPLE_RATE)

    if waveform == "sine":
        wave = np.sin(phase)

    elif waveform == "square":
        wave = np.sign(np.sin(phase))

    elif waveform == "saw":
        wave = 2 * ((phase / (2 * np.pi)) % 1) - 1

    else:
        wave = np.sin(phase)

    # smoother attack/release
    env = np.ones(n)

    attack = max(1, int(0.02 * SAMPLE_RATE))
    release = max(1, int(0.03 * SAMPLE_RATE))

    env[:attack] = np.linspace(0, 1, attack)
    env[-release:] = np.linspace(1, 0, release)

    return wave * env * 0.6
 
 
def _silence(duration):
    return np.zeros(int(SAMPLE_RATE * duration))
 
 
def text_to_beeps(text, emotion="neutral"):

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

        dur = profile["tempo"] * (
            0.7 + 0.5 * min(len(tok) / 6, 1.5)
        )

        emotion_name = emotion.lower()

        end_freq = freq
        vibrato_rate = 0
        vibrato_depth = 0

        # HAPPY
        if emotion_name == "happy":
            end_freq = freq * 1.45

        # QUESTION
        elif emotion_name == "question":
            end_freq = freq * 1.6

        # SCARED
        elif emotion_name == "scared":
            end_freq = freq * 1.25
            vibrato_rate = 14
            vibrato_depth = 0.10

        # SAD
        elif emotion_name == "sad":
            end_freq = freq * 0.75

        # COMFORTING
        elif emotion_name == "comforting":
            end_freq = freq * 0.90

        # WELCOME
        elif emotion_name == "welcome":
            end_freq = freq * 1.30

        # ANGRY
        elif emotion_name == "angry":
            end_freq = freq * 0.85

        # Last word boost for questions
        if is_question and word_index == n - 1:
            end_freq *= 1.4

        audio.append(
            _osc(
                freq,
                dur,
                profile["waveform"],
                end_freq=end_freq,
                vibrato_rate=vibrato_rate,
                vibrato_depth=vibrato_depth
            )
        )
        audio.append(_silence(profile["tempo"] * 0.35))
        word_index += 1
 
    return np.concatenate(audio)
 
 
def save_wav(waveform, path):
    """Write a numpy waveform to a 16-bit WAV file."""
    data = np.clip(waveform, -1, 1)
    wavfile.write(path, SAMPLE_RATE, (data * 32767).astype(np.int16))
 
 
def speak_beeps(text, emotion="neutral", out_path="robot_reply.wav"):
  
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