# Robot Rabbit

A social robot that helps reduce anxiety for patients before and after surgery. Built by a team of four for the Human-Robot Communication course at the University of Twente (2026).

Real pets are not allowed in hospitals because of hygiene rules, but research shows that animals help people feel calmer. Robot Rabbit tries to give some of that comfort in a form that can be used in a clinic. It does not talk or look human. It communicates the way an animal would: with its head, ears, eyes, a soft vibration ("purring") and short beeps instead of words.

<p align="center">
  <img src="media/bunny.jpg" width="45%" alt="The finished robot">
  <img src="media/face_tracking.gif" width="45%" alt="The camera neck following a face">
</p>

## How it works

1. The robot records what the person says and turns it into text with Google Speech Recognition.
2. Dialogflow detects what the person means and how they feel.
3. The robot chooses its own reaction instead of copying the user. If someone is sad or scared, it comforts them and then guides them through a short breathing exercise.
4. The answer is played as beeps (`beep_speech.py`). Every word becomes one beep. Pitch, speed and type of sound depend on the emotion, and the pitch follows the sentence: it rises at the end of a question and falls at the end of a statement. You can hear examples in the `beep_demo_*.wav` files.
5. Python sends the emotion to the Arduino over serial, and the Arduino drives the eyes, ears, head and vibration.

## Emotions

| Robot state | Eyes | Movement |
|---|---|---|
| Neutral | pale blue, blinking | idle |
| Happy | yellow | quick head wiggle |
| Sad | blue | slow, drooping head, then Comfort |
| Comfort | green | ears relax, then Breathing |
| Breathing | blue, pulsing in and out | guides the user's breathing |
| Questioning | orange | |
| Frown (when the user is angry) | red | ears down, head tilted down |

A scared user also gets Comfort followed by Breathing.

## Hardware

- Arduino UNO with a Grove shield
- HuskyLens AI camera on a pan-tilt servo head
- Servos for the ears
- NeoPixel LED matrices for the eyes
- Vibration motor for purring
- 3D-printed head and a fabric body

<p align="center"><img src="media/hardware.jpg" width="60%" alt="Wiring of the hardware"></p>

## Results

We tested the robot in a live demo with 5 to 6 people, who rated it on a 1 to 5 scale. All scores were between 3 and 5. People found the robot cute and calming ("feel at ease, all anxiety left me"), and liked that the beeps matched the emotion of the conversation.

What did not work well:
- People could not always tell what the robot was communicating, especially the breathing exercise.
- One ear was too heavy for its mount and kept falling off during the demo.
- The Frown state felt scary up close, which does not suit anxious patients.
- Because of time limits we could not make an opening for the camera in the face. The face tracking itself works: the camera sits in the neck and the pan-tilt neck follows a person's face (see the GIF above). But the head's own movements in the final demo were pre-programmed animations.

## What we would change

- Move the camera from the neck into the face, so the whole head follows the person, the robot makes real eye contact, and it only starts talking once it sees someone.
- Make the ears lighter and stronger, and motorise both of them.
- Add a soft "belly" that inflates and deflates during the breathing exercise, so it can be felt as well as seen.
- Use a small local language model to understand patients better, while keeping the beeps as the robot's voice.
- Never rely on colour alone, since colour meanings differ between people and colour-blind users can miss them.

## Running it

```
pip install SpeechRecognition google-cloud-dialogflow python-dotenv pygame pyserial numpy scipy
```

Create a `.env` file with your own Dialogflow settings. Do not commit this file.

```
CREDENTIAL_PATH=path/to/your-service-account.json
PROJECT_ID=your-dialogflow-project
SESSION_ID=any-session-id
LANGUAGE=en
```

Upload the Arduino sketch to the board, set the COM port in `HRC.py`, and run `python main.py`.

## Team

Alexander, Emma, Anna and Madeleine.

My part (Alexander Kralev): the speech and sound side (speech recognition, linking Dialogflow emotions to the robot's reactions, and the beep speech), the camera and face tracking, and work on the Arduino code and the hardware, including the pan-tilt head.

`huskylib.py` is DFRobot's HuskyLens Python library and is included unchanged.
