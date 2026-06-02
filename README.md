# my-heart

A tiny Arduino Nano + DFPlayer Mini build: press one button, hear a random audio clip. The sketch uses a *shuffle bag* so every clip plays once before any repeats, and the first track of a new bag is never the same as the last track of the previous one.

## Hardware

- Arduino Nano (ATmega328P)
- DFPlayer Mini MP3 module
- microSD card (FAT32) with audio files named `0001.mp3`, `0002.mp3`, … in a folder named `mp3` (or in the SD root, the DFPlayer accepts both)
- Momentary push-button (normally open)
- Speaker, 4–8 Ω, ≤3 W
- 1 × 1 kΩ resistor (in series with DFPlayer RX — protects its 3.3 V logic from the Nano's 5 V TX)
- *(optional but recommended)* 100 nF ceramic + 10–100 µF electrolytic across DFPlayer VCC/GND for power smoothing

## Wiring

```
            Arduino Nano                  DFPlayer Mini
            ┌──────────┐                  ┌──────────┐
       5V ──┤ 5V       │──────────────────┤ VCC      │
      GND ──┤ GND      │──────────────────┤ GND      │
            │ D1 (TX)  │──[ 1kΩ ]─────────┤ RX       │
            │ D0 (RX)  │──────────────────┤ TX       │
            │          │                  │ SPK_1    ├──┐
            │          │                  │ SPK_2    ├──┤  Speaker (4–8 Ω)
            │          │                  │ GND_SPK  │  │  (use SPK_1 & SPK_2,
            │          │                  └──────────┘  │   not GND, for the
            │          │                                │   built-in amp)
            │          │   Push-button (momentary)
            │  D2 ─────┼────o   o──── GND
            └──────────┘
```

### Pin summary

| Arduino Nano | Connects to                       | Notes                                       |
|--------------|-----------------------------------|---------------------------------------------|
| `5V`         | DFPlayer `VCC`                    | DFPlayer accepts 3.3–5 V; 5 V gives more headroom for the onboard amp |
| `GND`        | DFPlayer `GND`, button GND leg    | Common ground                               |
| `D1` (TX)    | DFPlayer `RX` via **1 kΩ** resistor | Hardware-UART TX. Resistor drops the Nano's 5 V logic toward DFPlayer's 3.3 V level. **Shared with the USB-serial chip — see upload note below.** |
| `D0` (RX)    | DFPlayer `TX`                     | Hardware-UART RX. **Disconnect this wire before uploading firmware** (otherwise the upload fails with `not in sync`). |
| `D2`         | One leg of push-button            | Other leg of button to `GND`. Uses `INPUT_PULLUP` — no external resistor needed |
| DFPlayer `SPK_1` / `SPK_2` | Speaker terminals       | Don't connect the speaker to `GND` — use both SPK pins for the differential amp output |

## SD card layout

Format the card as **FAT32**. The DFPlayer reads files by their *FAT entry order*, not filename, so:

1. Format the card fresh.
2. Copy files in the order you want them indexed: `0001.mp3` first, then `0002.mp3`, etc.

A safe layout that always works:

```
/mp3/0001.mp3
/mp3/0002.mp3
/mp3/0003.mp3
...
```

## Build / upload

The DFPlayer talks to the Nano over the **hardware UART (D0/D1)**, which is the same serial line the USB-to-serial chip uses for flashing. Two consequences:

- **Disconnect the DFPlayer's wire from `D0` before every upload** — if the module is driving `D0` while avrdude tries to talk to the bootloader, the upload fails with `not in sync`. Reconnect it after flashing.
- **There is no USB serial monitor while running.** Init failure (bad wiring / missing SD) is reported by a **fast blink on the onboard LED (D13)** instead of a serial message.

Steps:

1. Install the **DFRobotDFPlayerMini** library via the Arduino IDE Library Manager (or `arduino-cli lib install "DFRobotDFPlayerMini"`).
2. Open `my-heart.ino`.
3. Edit the constants at the top:
   - `TRACK_COUNT` — set to the number of MP3 files on your card.
   - `VOLUME` — `0`–`30`.
4. **Unplug the DFPlayer wire from `D0`.**
5. Select **Tools → Board → Arduino Nano** and the correct serial port, then upload.
6. Reconnect the `D0` wire.

With `arduino-cli` (with the `D0` wire disconnected):

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 .
arduino-cli upload  --fqbn arduino:avr:nano:cpu=atmega328 -p /dev/ttyUSB0 .
```

(Use `cpu=atmega328old` if your Nano has the older bootloader.)

## How the no-repeat logic works

- On boot, the sketch fills an array with track numbers `1..TRACK_COUNT` and shuffles it (Fisher–Yates).
- Each button press plays the next track from the shuffled bag.
- When the bag is exhausted, it's reshuffled. If the first track of the new bag happens to equal the last track of the old bag, it's swapped with a random other slot — so you never hear the same clip twice in a row, even across bag boundaries.
- `randomSeed()` is fed from noise on `A0`/`A1` so the shuffle differs between power-ups.

## Troubleshooting

- **Onboard LED (D13) blinks rapidly** — DFPlayer init failed. Check TX/RX aren't swapped (DFPlayer `TX` → Nano `D0`, DFPlayer `RX` → Nano `D1`), confirm the SD card is FAT32, and that you have the 1 kΩ resistor on the Nano-`D1` → DFPlayer-`RX` line.
- **Upload fails with `not in sync` / `resp=0x00`** — the DFPlayer is still connected to `D0` and is corrupting the bootloader handshake. Unplug the `D0` wire, upload, then reconnect it.
- **Loud pop on power-up** — add the 10–100 µF cap across DFPlayer VCC/GND.
- **Wrong track plays** — the DFPlayer indexes by *write order*, not filename. Reformat the SD and copy files one at a time in numeric order.
- **Button triggers twice** — increase `DEBOUNCE_MS`.
