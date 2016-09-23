#include <EEPROM.h>
#include "pokemon.h"
#include "output.h"

#define MOSI_ 10  //blue wire - pin 3
#define MISO_ 11  //green wire - pin 2
#define SCLK_ 12  //white wire - pin 5

const int MODE = 0; //mode=0 will transfer pokemon data from pokemon.h
                    //mode=1 will copy pokemon party data being received

uint8_t shift = 0;
uint8_t in_data = 0;
uint8_t out_data = 0;

connection_state_t connection_state = NOT_CONNECTED;
trade_centre_state_gen_II_t trade_centre_state_gen_II = INIT;
int counter = 0;

int trade_pokemon = -1;

unsigned long last_bit;

void setup() {
    Serial.begin(115200);
    pinMode(SCLK_, INPUT);
    pinMode(MISO_, INPUT);
    pinMode(MOSI_, OUTPUT);
    
    Serial.print("FEED ME POKEMON, I HUNGER!\n");
    
    digitalWrite(MOSI_, LOW);
    out_data <<= 1;

}

void loop() {
    last_bit = micros();
    while(digitalRead(SCLK_)) {
      if (micros() - last_bit > 1000000) {    //show when the clock is inactive
        Serial.print("idle\n");
        last_bit = micros();
        shift = 0;
        in_data = 0;
        
      }
    }
    transferBit();
}

void transferBit(void) {
    in_data |= digitalRead(MISO_) << (7-shift);

    if(++shift > 7) {
        shift = 0;
        out_data = handleIncomingByte(in_data);
        Serial.print(trade_centre_state_gen_II);
        Serial.print(" ");
        Serial.print(connection_state);
        Serial.print(" ");
        Serial.print(in_data, HEX);
        Serial.print(" ");
        Serial.print(out_data, HEX);
        Serial.print("\n");
        in_data = 0;
    }
    
    while(!digitalRead(SCLK_));   //no activity when clock is low
    
    digitalWrite(MOSI_, out_data & 0x80 ? HIGH : LOW);
    out_data <<= 1;
}

byte handleIncomingByte(byte in) {
  byte send = 0x00;

  switch(connection_state) {
  case NOT_CONNECTED:
    if(in == PKMN_MASTER)
      send = PKMN_SLAVE;
    else if(in == PKMN_BLANK)
      send = PKMN_BLANK;
    else if(in == PKMN_CONNECTED_II) {
      send = PKMN_CONNECTED_II;
      connection_state = CONNECTED;
    }
    break;

  case CONNECTED:
    if(in == PKMN_CONNECTED_II)   //acknowledge connection
      send = PKMN_CONNECTED_II;
    else if(in == GEN_II_CABLE_TRADE_CENTER){   //acknowledge trade center selection
      connection_state = TRADE_CENTRE;
      send = GEN_II_CABLE_TRADE_CENTER;
    }
    else if(in == GEN_II_CABLE_CLUB_COLOSSEUM){   //acknowledge colosseum selection
      connection_state = COLOSSEUM;
      send = GEN_II_CABLE_CLUB_COLOSSEUM;
    }
    else {
      send = in;
    }
    break;

  case TRADE_CENTRE:
    if(trade_centre_state_gen_II == INIT && in == 0x00) {
        trade_centre_state_gen_II = READY_TO_GO;
        send = 0x00;
    } else if(trade_centre_state_gen_II == READY_TO_GO && in == 0xFD) {
        trade_centre_state_gen_II = SEEN_FIRST_WAIT;
        send = 0xFD;
    } else if(trade_centre_state_gen_II == SEEN_FIRST_WAIT && in != 0xFD) {
        // random data of slave is ignored.
        send = in;
        trade_centre_state_gen_II = SENDING_RANDOM_DATA;
    } else if(trade_centre_state_gen_II == SENDING_RANDOM_DATA && in == 0xFD) {
        trade_centre_state_gen_II = WAITING_TO_SEND_DATA;
        send = 0xFD;
    } else if(trade_centre_state_gen_II == WAITING_TO_SEND_DATA && in != 0xFD) {
        counter = 0;
        // send first byte
        switch(MODE){
          case 0:
            send = pgm_read_byte(&(DATA_BLOCK_GEN_II[counter]));
            INPUT_BLOCK_GEN_II[counter] = in;
            break;
          case 1:
            send = in;
            break;
          default:
            send = in;
            break;
        }
        counter++;
        trade_centre_state_gen_II = SENDING_DATA;
    } else if(trade_centre_state_gen_II == SENDING_DATA) {
        switch(MODE){
          case 0:
            send = pgm_read_byte(&(DATA_BLOCK_GEN_II[counter]));
            INPUT_BLOCK_GEN_II[counter] = in;
            break;
          case 1:
            send = in;
            break;
          default:
            send = in;
            break;
        }
        counter++;
        if(counter == PLAYER_LENGTH_GEN_II) {
          trade_centre_state_gen_II = SENDING_PATCH_DATA;
        }
    } else if(trade_centre_state_gen_II == SENDING_PATCH_DATA && in == 0xFD) {
        counter = 0;
        send = 0xFD;
    } else if(trade_centre_state_gen_II == SENDING_PATCH_DATA && in != 0xFD) {
        send = in;
        trade_centre_state_gen_II = MIMIC;
    } else if(trade_centre_state_gen_II == MIMIC){
        send = in;




      
    } else if(trade_centre_state_gen_II == TRADE_PENDING && (in & 0x60) == 0x60) {
      if (in == 0x6f) {
        trade_centre_state_gen_II = READY_TO_GO;
        send = 0x6f;
      } else {
        send = 0x60; // first pokemon
        trade_pokemon = in - 0x60;
      }
    } else if(trade_centre_state_gen_II == TRADE_PENDING && in == 0x00) {
      send = 0;
      trade_centre_state_gen_II = TRADE_CONFIRMATION;
    } else if(trade_centre_state_gen_II == TRADE_CONFIRMATION && (in & 0x60) == 0x60) {
      send = in;
      if (in  == 0x61) {
        trade_pokemon = -1;
        trade_centre_state_gen_II = TRADE_PENDING;
      } else {
        trade_centre_state_gen_II = DONE;
      }
    } else if(trade_centre_state_gen_II == DONE && in == 0x00) {
      send = 0;
      trade_centre_state_gen_II = INIT;
    } else {
      send = in;
    }
    break;

  default:
    send = in;
    break;
  }

  return send;
}
