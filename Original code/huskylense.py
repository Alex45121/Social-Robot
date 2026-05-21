from huskylib import HuskyLensLibrary
import time

print("1. connecting...")
hl = HuskyLensLibrary("SERIAL", "COM8")
print("2. connected, knocking...")
result = hl.knock()
print("3. knock returned:", result)

def face_detection():
    data = hl.requestAll()

    return True if len(data) > 0 else False


if __name__ == "__main__":
    while True:
        print("yes" if face_detection() else "no")
        time.sleep(1)

