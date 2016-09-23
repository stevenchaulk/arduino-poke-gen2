# arduino-poke-gen2
Thanks to pepijndevos for initially setting up self trading with generation 1 games. This code is really just a modified version of that state machine so that it will work with generation 2 games. His code can be found here: https://github.com/pepijndevos/arduino-boy

Allows someone to trade pokemon with themselves, or spoof in a user created pokemon defined in file. This code should work for any generation 2 game (i.e. Gold, Silver, Crystal), although I have only tested it on a Crystal cartridge. Due to the way the second generation games transfer their data, nothing has to be stored in EEPROM. The arduino simply yells back whatever data it recives, therefore copying the currently held party. Simple, but effective.
