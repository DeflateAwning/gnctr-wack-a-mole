# PCB V1 Notes

## Errata
The following are errors on the V1 PCB, which should be fixed if a V2 is ordered.

* The label on 7-seg display 1 on the silkscreen is "(1)Display" instead of "(1)Clock".
* Not all 3 7-seg displays fit. They must be moved apart.
* Should have added a cutout under the Arduino to press the reset button, see debug lights, etc.
* Should have added a voltage selector for the 12V rail into the relay in case we want the buttons on 5V in the future.
* The reset button doesn't work (probably active-high vs. active-low).

## Extended Ideas
* Get rid of the relay board and use MOSFETs instead.
