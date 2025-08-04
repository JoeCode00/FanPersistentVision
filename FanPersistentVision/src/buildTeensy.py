import os

def build_teensy():
    os.system("cd ~/Documents/GitHub/FanPersistentVision/FanPersistentVision; pio run -s -t upload > output.log 2>&1 & pio run -s -t upload > output.log 2>&1 & pio run -s -t upload > output.log 2>&1")
    print("Teensy attempted build")