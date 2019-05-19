# ME430 Final Project: The One Button Bandit

The One Button Bandit™ is a one button, simple slot machine, created by Peter Garnache and Eric Tu.

## Features and Implementation

| Feature                                                             | Implementation                                                                                                                       |
|---------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| Read each coin input/output and updates credit display  accordingly | IR sensor beneath coin slot that  tells PIC2 to update  the 7 segment display                                                        |
| Spins three wheels randomly  at the push of a button                | Button signals PIC1 to spin  the three motors behind the wheels                                                                      |
| Has a consistent/accurate  payout                                   | IR sensor that tells PIC2 to stop  running the DC motor that's dispensing  coins. PWM and a relay will be used to  run the DC Motor. |
| Buzz on Jackpot                                                     | PIC2 uses certain PWM Duty cycles  to activate the buzzer                                                                            |

The machine states are as follows

|State  |Action    | Trigger|
|-------|----------|--------|
|Ready| The machine is at a still state| Nothing|
|Busy| Wheels are spinning| Button press & coin input sensed|
|Result|Outputs payout accordingly|Wheel positioning|

## Wheel Orientation
Note before use, all wheels must be primed to index slot 0. Each wheel contains the following stickers: 

Cherry (X1), Watermelon (X3), Orange (X3), 7 (X1), Banana (X3), and Grape (X3)

|Slot Index |Wheel 1  |Wheel 2  |Wheel 3|
|---------|---------|---------|-------|
|0|7|7| 7 |
|1|Watermelon|Orange|Grape|
|2|Banana|Watermelon|Watermelon|
|3|Cherry| Grape      |Grape|
|4|Orange|Watermelon|Banana|
|5|Banana|Cherry| Watermelon |
|6|Grape|Grape| Orange     |
|7|Orange|Banana|Banana|
|8|Watermelon|Orange|Orange|
|9|Orange|Banana|Cherry|
|10|Grape| Watermelon |Watermelon|
|11|Watermelon|Orange| Grape      |
|12|Banana|Grape|Banana|
|13|Grape|Banana|Orange|
## Design Milestones
- Milestone 0: Design process
- Milestone 1: PIC Microcontroller and LN Driver can run the Nemu 17 
- Milestone 2: Button press causes wheels to spin and return to default position
- Milestone 3: Button press AND coin input causes wheels to spin
- Milestone 4: Spin produces coin output and buzzer sounds

## Journal

### 2/1/2019

- 3 wheels can't be simultaneously driven. Heats up the drivers and is too slow.
- TODO: Set up push button switch and acquire the female wires

### 2/5/2019
- Only 2/3 steppers work. Running both driver power sources at 5 v regulated makes the regulator hot

## Testing
### Wheel Calibration
Use handleReel(1, 2, 2). Expected coin slots are the following
|Press No.|Slot 1|Slot 2|Slot 3|Side effects|
|-------|------|----|----|----|
|1st|W|W|W|+ 1 Credit|
|2nd|B|W|B|NA|
|3rd|C|G|O|NA|
|4th|O|O|O|Dispense 5 coins|
|5th|B|W|W|NA|
|6th|G|G|B|NA|

Wheels should land on the following stickers with marginal precision.
