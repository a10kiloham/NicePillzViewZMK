# Nice Pillz View - key reference

Top right row, after F12: **F13 | F14 | F15 | F16 | Program**

| Key | Does |
| --- | --- |
| F13 | Unlock ZMK Studio |
| F14 | F14. Shift + F14: output Bluetooth <-> USB |
| F15 | Print Screen |
| F16 | Layer: next layer in the loop 1 > 2 > 3 > 4 > 5 > 1 |
| Program | SYSTEM layer while held |
| Function (right thumb, left of Right Ctrl) | Held with another key: that key on the FN layer. Tapped alone: same as F16 |

## Program + key (hold Program)

| Keys | Does |
| --- | --- |
| Program + F1, F2, F3, F4 | Bluetooth device 1, 2, 3, 4 |
| Program + F8 | Forget all Bluetooth pairings |
| Program + Esc or = | Flash mode: the keyboard restarts as the NICENANO drive, copy the .uf2 onto it |
| Program + Caps Lock key | Caps Lock |
| Program + left Shift | Leader key, then type a sequence |

Flash mode without the keymap: double-tap the reset button on the board. For Bluetooth trouble use
Program + F8 first; the `_reset` firmware erases all settings and is only needed as a last resort.

## Other keys

| Key | Does |
| --- | --- |
| Caps Lock key (left of A) | F5 / refresh |
| Key under X, left of Left arrow | Win |
| Left thumb | Ctrl, Alt / Backspace, Delete |
| Right thumb | Function, Ctrl / Enter, Space |

## Layers

F16, or a tap of the Function thumb key, steps to the next layer. The filled dot on the display
shows which one is on. Layers 2 to 5 stay on until you step back to 1.

| Dot | Layer | What changes |
| --- | --- | --- |
| 1 | DEFAULT | Normal typing |
| 2 | LOWER | F1/F2 previous/next desktop, F3 task view, F4 new desktop, F8 input language, F9/F10 window to left/right monitor, F11 show desktop, F12 close desktop, H J K L arrows, A Z X C V with Ctrl |
| 3 | FN | F6 mute, F7 volume down, F8 volume up, F9 previous, F10 play/pause, F11 next, F12 stop. Also reached by holding the Function thumb key |
| 4 | SYSTEM | The Program + key table above, without holding Program. Careful: Esc and = are flash mode here |
| 5 | BLUETOOTH | F1 to F4 Bluetooth device 1 to 4, F8 forget all pairings |

Holding the Function thumb key gives the FN layer from any layer, so Function + F10 is always
play/pause.

## Pairing a new PC

1. Program + F8 to forget old pairings, and remove `NicePillz` from the PC's Bluetooth list.
2. Program + F1 (or F2 to F4) to pick the slot.
3. Pair from the PC. No PIN.
4. If it connects but nothing types, Shift + F14 (the display shows a USB symbol while USB is selected).

Edit keys: https://zmk.studio, plug in over USB and press F13.
