# avionics_nova_stm32
==========================================
======NEOPIXEL LED STRIP TEST REPORT======
==========================================

* Mode 0 (OFF + BLUE): LEDS ARE BLUE.
    - System 0 or 2: Only 1st half of the strip lights up. [CORRECT]
    - System 1 or 3: Only 2nd half of the strip lights up. [CORRECT]
* Mode 1 (ON): 
    - System = 0 (NAV - PINK): 1st half of the strip lights up in pink. [CORRECT]
    - System = 1 (HD - YELLOW): 2nd half of the strip lights up in yellow. [CORRECT]
    - System = 2 (DRILL - GREEN): 1st half of the strip lights up in green. [CORRECT]
    - System = 3 (AVIONICS - TURQUOISE): 2nd half of the strip lights up in turquoise. [CORRECT]
* Mode 2 (Labeled "BLINK" in the comments, but I see an animation of a line of 4~6 LEDs going back and forth):
    - System = 0 (NAV - PINK): 1st half of the strip lights up in pink with animation. [APPROVED]
    - System = 1 (HD - YELLOW): 2nd half of the strip lights up in yellow with animation. [APPROVED]
    - System = 2 (DRILL - GREEN): 1st half of the strip lights up in green with animation. [APPROVED]
    - System = 3 (AVIONICS - TURQUOISE): 2nd half of the strip lights up in turquoise. [APPROVED]
* Mode 3 (Labeled "FAULT" - Each group of 3 consecutive LEDs blink successively one after another.)
    - Similiar color and segmentations for all 4 systems as the previous modes. [APPROVED]
* Mode 4 (EMERGENCY_MOTORS):
    - Bright AMBER color (feels like white) over the entire strip, regardless of the system and the emergency_motors flag. [NEEDS_APPROVAL]
    - Eliot's code: In main.cpp, the cmd.segment.low and cmd.segment.high are set to 0 and 50, respectively. However, the mode4 private function always sets colors for all pixels (start = 0, end = 100). This bypasses segmentation control from main.cpp.
    - Should put the behavior conditional of the emergency_motors flag? I think the mode can act as the flag itself from the command, no?
* Mode 5 (EMERGENCY_SHUTDOWN):
    - RED color over the entire strip, regardless of the system and the command emergency_global flag. [NEEDS_APPROVAL]
    - Same concern as mode 4.
* Mode 6 (ALL OFF): ALL LEDS OF THE STRIP ARE SET TO BLUE, SYSTEM AND FLAG COMMANDS ARE BYPASSED. [CORRECT]
