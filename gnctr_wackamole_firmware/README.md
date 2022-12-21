# gnctr_wackamole_firmware

This firmware is the firmware for the GNCTR wack-a-mole project for the 2023 competition. It is based off of the code in [a mediocre Instructable](https://www.instructables.com/Home-Made-Whack-a-Mole/).

## Development Setup
1. Install Arduino IDE from Microsoft Store or similar.
2. Clone git repo.
3. Open firmware in Arduino IDE.
4. Install required dependencies by going to Sketch > Include Library > Manage Libraries..., then searching for:
    * **TM1637** (v1.2.0, by Avishay Orpaz)
5. Build and run the code.

## Critical Notes
* Each powered relay uses a ton of current. To prevent brownouts, only 3ish lights/relays can be active at once. Do not ever turn on all lights at once.

## Current To-Do List
* Add a countdown timer at the end of the game.
* Add "dOnE" text when the game is done (or show all zeros).
