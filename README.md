# NAD T163 Amplifier Input Volume Memory with OLED Display

This Arduino program, designed for the Arduino UNO board, interfaces with the NAD T163 amplifier via its serial port. The system recalls and sets the volume level for each selected input based on its previous volume. An integral part of this setup is a 0.96 inch SPI OLED display, which provides real-time feedback about the selected input, its volume setting, and if applicable, the AM/FM station frequency.

## Features:

1. **Monitor Input Changes:** Actively tracks the currently selected input.
2. **Volume Memory:** Remembers the previous volume setting for each input.
3. **Auto Volume Adjust:** On selecting an input, the volume is auto-adjusted to its last-known setting.
4. **OLED Display Feedback:** The OLED screen presents real-time updates about the selected input, its volume, and the AM/FM station frequency (when relevant).

## Setup:

### Hardware Requirements:

- Arduino UNO board
- RS232 Shield for Arduino (Product Code: RB-Dfr-480)
- RS232 serial cable (male to female DB9)
- DB9 Null Modem
- 0.96 inch SPI OLED display module

### Software Requirements:

- Arduino IDE
- Provided Arduino code for this project
- OLED library: [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- Graphics library: [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)

### Steps:

1. **Connect the Hardware:**
   - Mount the RS232 Shield on the Arduino UNO board.
   - Using the RS232 serial cable, establish a connection between the RS232 Shield and the NAD T163 amplifier. Implement the DB9 null modem in the connection setup.
   - Attach the 0.96 inch SPI OLED display to the Arduino UNO following the [guide](https://electropeak.com/learn/interfacing-0-96-inch-spi-oled-display-module-with-arduino/).

2. **Install Required Libraries:**
   - Launch the Arduino IDE.
   - Add the `Adafruit_SSD1306` library from [here](https://github.com/adafruit/Adafruit_SSD1306).
   - Incorporate the `Adafruit-GFX-Library` from [here](https://github.com/adafruit/Adafruit-GFX-Library).
   
3. **Upload the Code:**
   - Load the provided Arduino code into the Arduino IDE.
   - Ensure the Arduino UNO board and the appropriate COM port are selected.
   - Deploy the code to the Arduino board.

4. **Operation:**
   - Connect the serial cable to the back of the amplifier.  Connect the other end to the Arduino serial shield with the null modem.
   - Power up the NAD T163 amplifier and the Arduino UNO. The OLED screen will illuminate.
   - As inputs on the amplifier are toggled, the Arduino will adjust the volume to the last set level for that specific input. The OLED will display the current input, volume, and if relevant, the AM/FM frequency.
   - The Arduino consistently scans for changes in the input to adjust and display in real time.

## Usage:

Once set up, operate your amplifier as always. The Arduino program will work in the background, ensuring every time an input is chosen, the volume aligns with its last set level. The OLED display provides a prompt visual update of the selections, including input, volume, and AM/FM frequency (if applicable).

## Troubleshooting:

1. **Volume Doesn't Change:** Confirm the serial connection between the Arduino's RS232 shield and the amplifier, with the DB9 null modem, is correctly established.
2. **Incorrect Volume or Display Info:** Check for interference from other devices. A reset of the Arduino may be necessary.
3. **Input Not Recognized or Displayed:** Verify the serial connection, confirm the OLED's attachment, and ensure the program runs correctly on the Arduino.

## Future Enhancements:

- **Volume Fade-In:** Consider implementing a fade-in effect when adjusting volume for a smoother experience.
- **Remote Monitoring:** Ponder the addition of a wireless module for off-site observation and volume level management.

## Contributing:

Open for contributions! For enhancements or fixes, please create a pull request or issue on our [GitHub repository](#).

## License:

The project is open source and available under the [MIT License](https://opensource.org/licenses/MIT).



