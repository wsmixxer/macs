/* LED Patter
* === STARTUP ===
* red,green,red,green  -> i give you 10 sec to connect to me, before I start
* red on, green on -> I'm trying to connect to my server
*
* === UPDATE MODE ===
* green flashes 3 times (10Hz) -> I've found valid WiFi config 1 in EEPROM and now I'm scannning for it!
* green flashes 5 times (10Hz) -> I've found WiFi config 1 in scan results and try to conenct now!
* red flashes 3 times (10Hz) -> I've found valid WiFi config 2 in EEPROM and now I'm scannning for it!
* red flashes 5 times (10Hz) -> I've found WiFi config 2 in scan results and try to conenct now!
* red and green simulatnious, 2 fast blinks (10Hz) -> connected to wifi!
* red,green,red,greed fast (10sec, 20Hz) -> Waiting for the Wifi to save credentials AND give you time to add new credentials via serial
* red and green blink simultainous -> I'm ready for an update
*
* === MACS MODE (status can be combined) ===
* red and green simulatnious, flashes 3 times (10Hz) -> I've found valid WiFi config in EEPROM and now I'm scannning for it!
* red and green simulatnious, flashes 5 times (10Hz) -> I've found WiFi config in scan results and try to conenct now!
* red and green simulatnious, 2 fast flashes (10Hz)  -> connected to wifi!
* red blinking -> no connection to the MACS Server
* red solid -> card rejected
* green blinking -> connected to the MACS Server
* green solid -> card accepted
*/

// This #include statement was automatically added by the Particle IDE.
#include "application.h"
#include "stdint.h"
#include "config.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"
#include "Button.h"
#include "RDM6300_particle.h"
#include "WS2812FX.h"
#include "neopixel.h"

void console_debug(String message);
void goto_update_mode();
bool tag_fully_inserted();
bool access_test(uint32_t tag);
uint32_t relay(int8_t input);
bool validate_tag(uint8_t *buf,uint32_t *tag);
bool read_EEPROM();
bool update_ids(bool forced);
void create_report(uint8_t event,uint32_t badge,uint32_t extrainfo);
bool fire_report(uint8_t event,uint32_t badge,uint32_t extrainfo);
void set_connected(int status);
void set_connected(int status, bool force);
uint8_t get_my_id();

// network
IPAddress HOSTNAME(192,168,1,100);
uint32_t v=20160214;

uint8_t keys_available=0;
uint32_t keys[MAX_KEYS];

uint8_t connected=0;
uint32_t current_tag_id=0;
bool current_tag_used = false;
bool current_tag_removed = true;

uint8_t current_relay_state=RELAY_DISCONNECTED;
uint8_t id=-1; //255, my own id
uint8_t tagInRange=0;
uint32_t last_key_update=0;
uint32_t last_server_request=0;
uint32_t relay_open_timestamp=0;
uint32_t last_tag_read=0;
static unsigned long lastGeneralDebugRefreshTime = 0;
static unsigned long lastRFIDDebugRefreshTime = 0;
bool initialized = false;

LED db_led(DB_LED_AND_UPDATE_PIN,DB_LED_DELAY,1,1); // weak + inverse
LED red_led(RED_LED_PIN,RED_LED_DELAY,0,0);
LED green_led(GREEN_LED_PIN,GREEN_LED_DELAY,0,0);

BACKUP b; // report backup

// http server
HttpClient http; // Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {     { "Accept" , "*/*"},     { NULL, NULL } }; // NOTE: Always terminate headers will NULL
http_request_t request;
http_response_t response;

Rdm6300 rdm6300;
LiquidCrystal_I2C lcd(0x27,20,4); // Grab and run third party i2c scanning code if this bus address doesn't work for you
Button micro_switch(MICROSWITCH_PIN);

//Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, PIXEL_TYPE);

byte logo_r0_c0[] = {0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte logo_r0_c1[] = {0x00, 0x00, 0x00, 0x11, 0x19, 0x1D, 0x1F, 0x1D};
byte logo_r0_c2[] = {0x0E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
byte logo_r1_c0[] = {0x07, 0x13, 0x19, 0x1C, 0x1E, 0x1F, 0x1F, 0x00};
byte logo_r1_c1[] = {0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00};
byte logo_r1_c2[] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x00};

//////////////////////////////// SETUP ////////////////////////////////
void setup() {

    //FLASH_Lock();
    // set adress pins
    for(uint8_t i=10; i<=MAX_JUMPER_PIN+10; i++){   // A0..7 is 10..17, used to read my ID
       pinMode(i,INPUT_PULLUP);
    }
    pinMode(RELAY_PIN,OUTPUT);          // driver for the relay

    // the db led is a weak and inverterted LED on the same pin as the update_input, this will set the pin to input_pullup anyway //
    pinMode(DB_LED_AND_UPDATE_PIN,INPUT_PULLUP);
    Serial.begin(115200); // Serial to be used for debug output


    // give ourselves time to manually connect a serial monitor before spitting out serial messages
    #if defined DEBUG_JKW || defined DEBUG_JKW_MAIN || defined LOGGING || defined DEBUG_JKW_LED || defined DEBUG_JKW_WIFI
    for(int i=5;i>=1;i--){
        Serial.print(i);
        Serial.print("...");
        delay(1000);
    }
    Serial.println("Delay allowing manual connection of serial monitor has ended.");
    #endif

    rdm6300.init(); // RDM6300 is connected to  rx/tx pins, making use of Serial1
    micro_switch.init();
    lcd.init();
    ws2812fx.init();
    ws2812fx.setBrightness(255);
    ws2812fx.setColor(BLUE);
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.start();

    lcd.createChar(0, logo_r0_c0);
    lcd.createChar(1, logo_r0_c1);
    lcd.createChar(2, logo_r0_c2);
    lcd.createChar(3, logo_r1_c0);
    lcd.createChar(4, logo_r1_c1);
    lcd.createChar(5, logo_r1_c2);

    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.write(0);
    lcd.setCursor(1,0);
    lcd.write(1);
    lcd.setCursor(2,0);
    lcd.write(2);
    lcd.setCursor(0,1);
    lcd.write(3);
    lcd.setCursor(1,1);
    lcd.write(4);
    lcd.setCursor(2,1);
    lcd.write(5);

    lcd.setCursor(3,0);
    lcd.print("Initializing     ");
    lcd.setCursor(3,1);
    lcd.print("_________________");
    lcd.setCursor(15,0);
    lcd.blink();

    // setup https client
    request.ip = HOSTNAME;
    request.port = HOSTPORT;


    // read mode to starting with
    //if(digitalRead(DB_LED_AND_UPDATE_PIN)){

        // ############ MACS MODUS ############ //
        #ifdef DEBUG_JKW_MAIN
        Serial.println("- MACS -");
        #endif

        //red_led.on();
        //green_led.on();

        set_connected(0); // init as not connected
        if(update_ids(true)){ // true = force update, update_ids will initiate the connect
            set_connected(1);
        } else {
            set_connected(0,true); // force LED update for not connected
            read_EEPROM();
        }
        // ############ MACS MODUS ############ //

    //} else {
    //    goto_update_mode();
    //}

}
//////////////////////////////// SETUP ////////////////////////////////

// Log message to cloud,
void debug(String message) {
    Spark.publish("DEBUG", message);
}

 // ############ UPDATE MODUS ############ //
void goto_update_mode(){
    connected=0;
    red_led.on();
    green_led.on();
    db_led.on();

    #ifdef DEBUG_JKW_MAIN
    Serial.println("- Cloud -");
    #endif

    // satrt loop that will set wifi data and connect to cloud,
    // and if anything fails start again, until there is an update
    while(1){
        // set_update_login will return true, if we've read a valid config from
        // the EEPROM memory AND that WIFI was in range AND the module has saved the login

            Particle.connect();
            uint8_t i=0;

            // backup, if connect didn't work, repeat it
            while(!WiFi.ready()){
                Serial.print(".");
                Particle.connect();
            }

            // stay in update mode forever
            while(WiFi.ready()){
                if(i!=millis()/1000){

                    #ifdef DEBUG_JKW_MAIN
                    Serial.print(i);
                    Serial.print(": ");
                    #endif

                    if(Particle.connected()){
                        // as soon as we are connected, swtich to blink mode to make it visible
                        if(!connected){
                            red_led.blink();
                            green_led.blink();
                            db_led.blink();
                            connected=1;
                        } else {
                            Particle.process();
                        }

                        // keep blinking
                        red_led.check();
                        green_led.check();
                        db_led.check();

                        #ifdef DEBUG_JKW_MAIN
                        Serial.println("Photon connected");
                        #endif

                    } else {

                        #ifdef DEBUG_JKW_MAIN
                        Serial.println("Photon NOT connected");
                        #endif

                        // constant on == not yet connected
                        red_led.on();
                        green_led.on();
                        db_led.on();
                    }
                    i=millis()/1000;
                } // i!=millis()/1000
                delay(200); // don't go to high as blink will look odd
            } // end while(WiFi.ready())
            // reaching this point tells us that we've set the wifi login, tried to connect but lost the connection, as the wifi is not (longer) ready

    } // end while(1)
    // ############ UPDATE MODUS ############ //
}
// ############ UPDATE MODUS ############ //

//////////////////////////////// MAIN LOOP ////////////////////////////////
// woop woop main loop
void loop() {

    ws2812fx.service();

    if (!initialized) {
        lcd.setCursor(3,0);
        lcd.print("Insert ID Card   ");
        lcd.setCursor(17,0);
        initialized = true;
    }

    // Check if we found a tag
    if (rdm6300.update()) {
        current_tag_id = rdm6300.get_tag_id();
        lcd.setCursor(3,0);
        lcd.print("                 ");
        lcd.setCursor(3,0);
        lcd.blink();
        if (!tag_fully_inserted()) {
            lcd.setCursor(3,2);
            lcd.print("Please fully     ");
            lcd.setCursor(3,3);
            lcd.print("insert ID card   ");
            ws2812fx.setColor(GREEN);
            ws2812fx.setMode(FX_MODE_BLINK);
        }
        Serial.println("Found tag " + String(current_tag_id));
        current_tag_used = false;
    }

    if(tag_fully_inserted() && !current_tag_used && current_tag_id > 0) {

        current_tag_used = true;
        current_tag_removed = false;

        lcd.noBlink();
        lcd.setCursor(3,0);
        lcd.print("Card recognized  ");

        if(current_tag_id == UPDATECARD){
            goto_update_mode();
        }
        // if we found a tag, test it
        // if it works close relay,
        // if not - ask the server for an update and try again
        uint8_t tries=2; // two tries to check the card
        while(tries){
            // compares known keys, returns true if key is known
            if(access_test(current_tag_id)){
                relay(RELAY_CONNECTED);
                tries=0;

                lcd.noBlink();
                lcd.setCursor(3,2);
                lcd.print("Access granted   ");
                lcd.setCursor(3,3);
                lcd.print("to this machine  ");

                ws2812fx.setColor(GREEN);
                ws2812fx.setMode(FX_MODE_BREATH);

                // takes long
                create_report(LOG_RELAY_CONNECTED,current_tag_id,0);
                // 1. assuming that we are NOT connected, we reached this point and the create_report has success to reconnet than it will call set_connected()
                // this will turn red off (which is fine (was blinking = not connected)) and green to blink (ok), so we have to override it
                // 2. assuming that we are NOT connected, we reached this point and the create_report has NO success to reconnet than it will not call set_connected()
                // the red will keep blinking (ok) but we still want to show that this card was good, turn green on
                // 3. assuming that we are connected, we reached this point then create_report will not try to reconnect and the report is send just fine
                // the red will be off anywa (ok), we want to show that this card was good, turn green on
                // 4. assuming that we are connected, we reached this point then create_report will not try to reconnect, but the report failed, create_report will set us to not conneted
                // the red will be blinkin (ok), we want to show that this card was good, turn green on
                set_connected(connected,1); // force to resume LED pattern

            } else {
                // if we have a card that is not known to be valid we should maybe check our database
                if(tries>1){

                    lcd.setCursor(3,2);
                    lcd.print("Access pending   ");
                    lcd.setCursor(3,3);
                    lcd.print("Refreshing IDs   ");
                    lcd.setCursor(17,3);
                    lcd.blink();

                    ws2812fx.setColor(ORANGE);
                    ws2812fx.setMode(FX_MODE_BLINK);

                    #ifdef DEBUG_JKW_MAIN
                    Serial.println("Key not valid, requesting update from server");
                    #endif

                    update_ids(false); // unforced update

                    #ifdef DEBUG_JKW_MAIN
                    if(tries>0){
                        Serial.println("Trying once more if key is valid now");
                    };
                    #endif

                    tries-=1;
                } else {

                    lcd.noBlink();
                    lcd.setCursor(3,2);
                    lcd.print("Access denied    ");
                    lcd.setCursor(3,3);
                    lcd.print("ID not authorized");

                    ws2812fx.setColor(RED);
                    ws2812fx.setMode(FX_MODE_STATIC);

                    #ifdef DEBUG_JKW_MAIN
                    Serial.println("key still not valid. :P");
                    #endif

                    tries=0;
                    // takes long
                    create_report(LOG_LOGIN_REJECTED,current_tag_id,0);
                }
            }
        }
    }

    // card moved away
    // if serial1 has avaioable chars, it means that TAG IN RANGE had to be low at some point, its just a new card there now!
    //if((digitalRead(TAG_IN_RANGE_INPUT)==0 || Serial1.available()) && current_tag_id!=-1){
    if(!tag_fully_inserted()) {

        // open the relay as soon as the tag is gone
        if(current_relay_state==RELAY_CONNECTED){
            uint32_t open_time_sec=relay(RELAY_DISCONNECTED);
            green_led.resume();
            // last because it takes long
            create_report(LOG_RELAY_DISCONNECTED,current_tag_id,open_time_sec);
        }

        // Performed only once when  card is first removed
        if(!current_tag_removed) {
            current_tag_removed = true;
            lcd.setCursor(3,0);
            lcd.print("Insert ID card   ");
            lcd.setCursor(0,2);
            lcd.print("                    ");
            lcd.setCursor(0,3);
            lcd.print("                    ");
            lcd.setCursor(17,0);
            lcd.blink();
            ws2812fx.setColor(BLUE);
            ws2812fx.setMode(FX_MODE_BREATH);
        }

        set_connected(connected,1); // force to resume LED pattern
    }

    // time based update the storage from the server (every 10 min?)
    if(last_key_update+DB_UPDATE_TIME<(millis()/1000)){
        update_ids(false);  // unforced upate
    }


    // see if we should switch off the leds by now
    db_led.check();
    red_led.check();
    green_led.check();
}
//////////////////////////////// MAIN LOOP ////////////////////////////////

//////////////////////////////// TAG INSERTED TEST ////////////////////////////////
// called to test whether the card is still fully inserted
bool tag_fully_inserted() {
    return micro_switch.read();
}
//////////////////////////////// TAG INSERTED TEST ////////////////////////////////

//////////////////////////////// ACCESS TEST ////////////////////////////////
// callen from main loop as soon as a tag has been found to test if it matches one of the saved keys
bool access_test(uint32_t tag){
    #ifdef DEBUG_JKW_MAIN
    Serial.print("Tag ");
    Serial.print(tag);
    Serial.print(" found. Checking database (");
    Serial.print(keys_available);
    Serial.print(") for matching key");
    Serial.println("==============");
    #endif

    for(uint16_t i=0;i<MAX_KEYS && i<keys_available; i++){

        #ifdef DEBUG_JKW_MAIN
        Serial.print(i+1);
        Serial.print(" / ");
        Serial.print(keys_available);
        Serial.print(" Compare current read tag ");
        Serial.print(tag);
        Serial.print(" to stored key ");
        Serial.print(keys[i]);
        Serial.println("");
        #endif

        if(keys[i]==tag){

            #ifdef DEBUG_JKW_MAIN
            Serial.println("Key valid, closing relay");
            #endif

            return true;
        }
    }

    #ifdef DEBUG_JKW_MAIN
    Serial.println("==============");
    #endif

    return false;
}
//////////////////////////////// ACCESS TEST ////////////////////////////////

//////////////////////////////// DRIVE THE RELAY ////////////////////////////////
// hardware controll, writing to the pin and log times
uint32_t relay(int8_t input){
    if(input==RELAY_CONNECTED){

        #ifdef DEBUG_JKW_MAIN
        Serial.println("Connecting relay!");
        #endif

        digitalWrite(RELAY_PIN,HIGH);
        current_relay_state=RELAY_CONNECTED;
        relay_open_timestamp=millis()/1000;
    } else {

        #ifdef DEBUG_JKW_MAIN
        Serial.println("Disconnecting relay!");
        #endif

        digitalWrite(RELAY_PIN,LOW);
        current_relay_state=RELAY_DISCONNECTED;
        return ((millis()/1000) - relay_open_timestamp);
    }
}
//////////////////////////////// DRIVE THE RELAY ////////////////////////////////

//////////////////////////////// READ ID's FROM EEPROM ////////////////////////////////
// if we are running offline
bool read_EEPROM(){

    #ifdef DEBUG_JKW_MAIN
    Serial.println("-- This is EEPROM read --");
    #endif

    uint8_t temp;
    uint16_t num_keys=0;
    uint16_t num_keys_check=0;

    temp=EEPROM.read(KEY_NUM_EEPROM_HIGH);
    num_keys=temp<<8;
    temp=EEPROM.read(KEY_NUM_EEPROM_LOW);
    num_keys+=temp;



    #ifdef DEBUG_JKW_MAIN
    Serial.print("# of keys =");
    Serial.println(num_keys);
    #endif


    temp=EEPROM.read(KEY_CHECK_EEPROM_HIGH);
    num_keys_check=temp<<8;
    temp=EEPROM.read(KEY_CHECK_EEPROM_LOW);
    num_keys_check+=temp;

    #ifdef DEBUG_JKW_MAIN
    Serial.print("# of keys+1 =");
    Serial.println(num_keys_check);
    #endif

    if(num_keys_check==num_keys+1){
        keys_available=num_keys;
        for(uint16_t i=0;i<num_keys;i++){
            temp=EEPROM.read(i*4+0);
            keys[i]=temp<<24;
            temp=EEPROM.read(i*4+1);
            keys[i]+=temp<<16;
            temp=EEPROM.read(i*4+2);
            keys[i]+=temp<<8;
            temp=EEPROM.read(i*4+3);
            keys[i]+=temp;

            #ifdef DEBUG_JKW_MAIN
            Serial.print("Read key ");
            Serial.print(i);
            Serial.print("=");
            Serial.print(keys[i]);
            Serial.println(" from eeprom");
            #endif
        }
    }

    #ifdef DEBUG_JKW_MAIN
    Serial.println("-- End of EEPROM read --");
    #endif
}
//////////////////////////////// READ ID's FROM EEPROM ////////////////////////////////

//////////////////////////////// UPDATE ID's ////////////////////////////////
// sends a request to the amazon server, this server should be later changed to
// be the local Raspberry pi. It will call the get_my_id() function
// return true if http request was ok
// false if not - you might want to set a LED if it returns false
bool update_ids(bool forced){
    // avoid flooding
    if(last_key_update+MIN_UPDATE_TIME>millis()/1000 && last_key_update>0){

        #ifdef DEBUG_JKW_MAIN
        Serial.println("db read blocked, too frequent");
        #endif

        return false;
    }

    last_key_update=millis()/1000;
    db_led.on(); // turn the led on

    // request data
    uint32_t now=millis();
    request.path="/m2m.php?v="+String(v)+"&mach_nr="+String(get_my_id())+"&forced=";
    if(forced){
        request.path=request.path+"1";
    } else {
        request.path=request.path+"0";
    }

    Serial.println(request.path);

    green_led.on();
    red_led.on();


    // Get request
    http.get(request, response, headers);
    int statusCode = response.status;
    green_led.resume();
    red_led.resume();

    #ifdef DEBUG_JKW_MAIN
    Serial.print("db request took ");
    Serial.print(millis()-now);
    Serial.println(" ms");
    delay(1000);
    Serial.println("Requested:");
    Serial.println(request.path);
    delay(1000);
    Serial.println("Recevied:");
    Serial.println(response.body);
    #endif


    // check status
    if(statusCode!=200){
        db_led.off(); // turn the led off
        set_connected(0);

        #ifdef DEBUG_JKW_MAIN
        Serial.println("No response from server");
        #endif

        return false;
    }

    // check length
    if(response.body.length()==0){
        db_led.off(); // turn the led off

        #ifdef DEBUG_JKW_MAIN
        Serial.println("Empty response");
        #endif
    }

    // connection looks good
    set_connected(1,true); // force update LEDs as the reconnect might have overwritten the LED mode

    // check if we've received a "no update" message from the server
    // if we are unforced we'll just leave our EEPROM as is.
    // otherweise we'll go on
    //Serial.println("response length:");
    //Serial.println(response.length());
    //Serial.print(response.charAt(0));
    //Serial.println(response.charAt(1));

    if(!forced && response.body.length()>=2){
        if(response.body.charAt(0)=='n' && response.body.charAt(1)=='u'){
            // we received a 'no update'

            #ifdef DEBUG_JKW_MAIN
            Serial.println("No update received");
            #endif
            b.try_fire();
            db_led.off(); // turn the led off
            return true;
        }
    }

    // clear all existing keys and then, import keys from request
    keys_available=0;
    uint16_t current_key=0;
    for(uint16_t i=0;i<sizeof(keys)/sizeof(keys[0]);i++){
        keys[i]=0;
    }

    //FLASH_Unlock();
    for(uint16_t i=0;i<response.body.length();i++){
        Serial.print(response.body.charAt(i));

        if(response.body.charAt(i)==','){
            if(current_key<MAX_KEYS){
                Serial.print("write:");
                Serial.println(current_key*4+3);
                // store to EEPROM
                EEPROM.write(current_key*4+0, (keys[current_key]>>24)&0xff);
                EEPROM.write(current_key*4+1, (keys[current_key]>>16)&0xff);
                EEPROM.write(current_key*4+2, (keys[current_key]>>8)&0xff);
                EEPROM.write(current_key*4+3, (keys[current_key])&0xff);

                current_key++;
            }
        } else if(response.body.charAt(i)>='0' && response.body.charAt(i)<='9') { // zahl
            keys[current_key]=keys[current_key]*10+(response.body.charAt(i)-'0');
        }
    }
    keys_available=current_key;


    // log number of keys to the eeprom
    Serial.print("write:");
    Serial.println(KEY_NUM_EEPROM_LOW);

    EEPROM.write(KEY_NUM_EEPROM_HIGH,(keys_available>>8)&0xff);
    EEPROM.update(KEY_NUM_EEPROM_LOW,(keys_available)&0xff);
    // checksum
    Serial.print("write:");
    Serial.println(KEY_CHECK_EEPROM_LOW);
    EEPROM.write(KEY_CHECK_EEPROM_HIGH,((keys_available+1)>>8)&0xff);
    EEPROM.write(KEY_CHECK_EEPROM_LOW,((keys_available+1))&0xff);
    //FLASH_Lock();

    #ifdef DEBUG_JKW_MAIN
    Serial.print("Total received keys for my id(");
    Serial.print(get_my_id());
    Serial.print("):");
    Serial.println(keys_available);
    for(uint16_t i=0;i<keys_available;i++){
        Serial.print("Valid Database Key Nr ");
        Serial.print(i+1);
        Serial.print(": ");
        Serial.print(keys[i]);
        Serial.println("");
    };
    #endif

    b.try_fire(); // try to submit old reports
    db_led.off(); // turn the led off
    return true;
}
//////////////////////////////// UPDATE ID's ////////////////////////////////

//////////////////////////////// CREATE REPORT ////////////////////////////////
// create a log entry on the server for the action performed
void create_report(uint8_t event,uint32_t badge,uint32_t extrainfo){
    if(!fire_report(event, badge, extrainfo)){
        b.add(event,badge,extrainfo);
    } else {
        b.try_fire();
    }
}

bool fire_report(uint8_t event,uint32_t badge,uint32_t extrainfo){
    bool ret=true;

    db_led.on(); // turn the led on
    if(event==LOG_RELAY_CONNECTED){
        request.path = "/history.php?logme&badge="+String(badge)+"&mach_nr="+String(get_my_id())+"&event=Unlocked";
    } else if(event==LOG_LOGIN_REJECTED){
        request.path = "/history.php?logme&badge="+String(badge)+"&mach_nr="+String(get_my_id())+"&event=Rejected";
    } else if(event==LOG_RELAY_DISCONNECTED){
        request.path = "/history.php?logme&badge="+String(badge)+"&mach_nr="+String(get_my_id())+"&event=Locked&timeopen="+String(extrainfo);
    } else if(event==LOG_NOTHING){
        request.path = "/history.php";
    } else {
        return false;
    }

    #ifdef DEBUG_JKW_MAIN
    Serial.print("calling:");
    Serial.println(request.path);
    #endif

    uint32_t now=millis();

    green_led.on();
    red_led.on();
    http.get(request, response, headers);
    int statusCode = response.status;
    green_led.resume();
    red_led.resume();


    if(statusCode==200){
        set_connected(1);
    } else if(statusCode!=200){
        set_connected(0);
        ret=false;
    }

    #ifdef DEBUG_JKW_MAIN
    Serial.print("db request took ");
    Serial.print(millis()-now);
    Serial.println(" ms");
    #endif

    db_led.off(); // turn the led off
    return ret;
}
//////////////////////////////// CREATE REPORT ////////////////////////////////

//////////////////////////////// SET CONNECTED ////////////////////////////////
// set the LED pattern
void set_connected(int status){
    set_connected(status,false);
};

void set_connected(int status, bool force){
    if(status==1 && (connected==0 || force)){
        connected=1;
        green_led.blink();
        red_led.off();
    } else if(status==0 && (connected==1 || force)){
        connected=0;
        green_led.off();
        red_led.blink();
    }
}

//////////////////////////////// SET CONNECTED ////////////////////////////////

//////////////////////////////// GET MY ID ////////////////////////////////
// shall later on read the device jumper and return that number
// will only do the interation with the pins once for performance
uint8_t get_my_id(){
    if(id==(uint8_t)-1){
        id=0;

        #ifdef DEBUG_JKW_MAIN
        Serial.print("ID never set, reading");
        #endif

        for(uint8_t i=10+MAX_JUMPER_PIN; i>=10; i--){   // A0..7 is 10..17
            id=id<<1;
            if(!digitalRead(i)){
                id++;
            };
        }

        #ifdef DEBUG_JKW_MAIN
        Serial.print(" id for this device as ");
        Serial.println(id);
        #endif

    }
    return id;
}
//////////////////////////////// GET MY ID ////////////////////////////////
