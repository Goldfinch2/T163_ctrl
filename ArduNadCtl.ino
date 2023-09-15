
#include <limits.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define START_FRAME    0x01
#define END_FRAME      0x02
#define CMD_QUERY      0x14
#define CMD_SET        0x15
#define CMD_IR         0x16

#define VAR_MODEL      0x14
#define VAR_POWER_MODE 0x15
#define VAR_VOL        0x17
#define VAR_MUTE       0x18
#define VAR_ZONE_VOL   0x1B
#define VAR_TAPE_MON   0x1c
#define VAR_TREBLE     0x20
#define VAR_BASS       0x21
#define VAR_AM         0x2C
#define VAR_FM         0x2D
#define VAR_INPUT      0x35


#define INPUT_TUNER    9
#define INPUT_AM       10
#define INPUT_FM       11

#define SCREEN_WIDTH   128 
#define SCREEN_HEIGHT  64 

#define OLED_DC        8
#define OLED_CS        10
#define OLED_RESET     9

#define ESC_CHAR      0x5E
#define OR_VALUE      0x40
#define AND_VALUE     0xBF
#define IS_CTL_CHARACTER(a) (((a) < 20) || ((a) == ESC_CHAR))


Adafruit_SSD1306       s_display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof((a)[0]))
#define MAX_VOLS       10

struct input_info_t {
    uint16_t ident;
    int      x;
    int      y;
    uint8_t  text_size;
    char     text[8];
};

struct comm_timeout_t {
    unsigned long inter_char_timeout;
    unsigned long total_timeout;
};

struct stats_t {
    unsigned int dbg_cnt;
    unsigned int tx_cnt;
    unsigned int rx_cnt;
    unsigned int rx_timeout;
    unsigned int rx_good_frame;
    unsigned int rx_bad_frame;      
};


struct amp_data_t {
    uint16_t     last_input;
    uint16_t     input;
    bool         input_set;
    int8_t       last_volume;
    int8_t       volume;
    bool         volume_set;
    bool         am;
    uint16_t     freq_am;
    uint16_t     freq_fm;
};

static comm_timeout_t s_ct;
static stats_t        s_stats;
static amp_data_t     s_amp;
static int            s_last_query;


static const PROGMEM input_info_t s_input_info[] = {

    {0, 2, 2, 1, "DVD"},
    {1, 44, 2, 1, "SAT"},
    {2, 68, 2, 1, "VCR"},
    {6, 92, 2, 1, "CD"},
    {3, 126 - 6 * 4, 16, 1, "VID4"},  
    {4, 126 - 6 * 4, 28, 1, "VID5"},
    {5, 126 - 6 * 4, 40, 1, "VID6"},
    {9, 15, 62 - 8, 1, "TUNER"},
    {8, 51, 62 - 8, 1, "EXT7.1"},
    {7, 93, 62 - 8, 1, "DISC"},
    {10, 86, 16, 1, "AM"},
    {11, 88, 28, 1, "FM"},
};

static int8_t s_vols[MAX_VOLS];



void display_text_in_rect(int x, int y, uint8_t text_size, const char* text) 
{

    int16_t x1, y1;
    uint16_t text_width, text_height;

    s_display.setTextSize(text_size);
    s_display.getTextBounds(text, x, y, &x1, &y1, &text_width, &text_height);
    s_display.drawRect(x - 2, y - 2, text_width + 4, text_height + 4, SSD1306_WHITE); 
    s_display.setCursor(x, y);
    s_display.setTextColor(SSD1306_WHITE);
    s_display.print(text);
}



void show_volume(char *volume) 
{
    s_display.setTextSize(3);
    s_display.setTextColor(SSD1306_WHITE);
    s_display.setCursor(0, 20);
    s_display.print(volume);

}

void set_comm_timeout(unsigned long inter_char, unsigned long total) 
{
    s_ct.inter_char_timeout = inter_char;
    s_ct.total_timeout = total;
}



size_t read_serial_with_timeout(uint8_t *buffer, size_t buffer_size)
{
    int index = 0;
    bool ended = false;
    int ch;
    unsigned long last_received_time = millis();
    unsigned long start_time = last_received_time;
    bool esc = false;

    while (index < buffer_size) {
        if (Serial.available()) {
            s_stats.rx_cnt++;
            ch = Serial.read();
            if (esc) {
                if (ch != ESC_CHAR) {
                    ch &= AND_VALUE;
                }
            }
            else if (ch == ESC_CHAR) {
                esc = true;
                continue;
            }
            buffer[index++] = ch;
            last_received_time = millis();
            if (ended) {
                break;
            }
            else if (!esc && (ch == END_FRAME)) {
                ended = true;
            }
            esc = false;
        }
        else {
            // Check inter-character timeout
            if (millis() - last_received_time > s_ct.inter_char_timeout) {
                break;
            }
            // Check total timeout
            if (millis() - start_time > s_ct.total_timeout) {
                break;
            }
        }

    }
    if (index == 0) {
        s_stats.rx_timeout++;
    }
    return index;
}




uint8_t compute_checksum(uint8_t* data, size_t length) 
{
    uint8_t checksum = 0;
    uint8_t val;
    bool    esc = false;

    for (int i = 1; i < length ; i++) {
        val = data[i];
        if (esc) {
            if (val != ESC_CHAR) {
                val &= AND_VALUE;
            }
        }
        else if (val == ESC_CHAR) {
            esc = true;
            continue;
        }
        esc = false;
        checksum += val;
    }
    return ~checksum;
}

bool is_valid_frame(uint8_t* frame, size_t length) 
{
    bool valid;


    valid = (length > 2) && (frame[0] == START_FRAME) && (frame[length - 2] == END_FRAME);
    if (valid) {
        valid = compute_checksum(frame, length - 2) == frame[length - 1];
    }
    if (valid) {
        s_stats.rx_good_frame++;
    }
    else {
        s_stats.rx_bad_frame++;
    }
    return valid;
}

void send_frame(uint8_t *frame, size_t len)
{
    s_stats.tx_cnt += len;
    Serial.write(frame, len);
}

bool receive_frame(uint8_t *frame, size_t length, size_t *lenRx) 
{
    size_t size;
    bool   valid = false;

    *lenRx = 0;
    size = read_serial_with_timeout(frame, length);
    if (size) {
        if (is_valid_frame(frame, size)) {
            *lenRx = size;
            valid = true;
        }
    }
    return valid;

}


uint8_t* set_frame(uint8_t command, uint8_t variable, uint8_t *value, size_t value_size, size_t *frame_size_out) 
{
    static uint8_t frame[64];  
    int cnt = 0, i;
    uint8_t chk;

    frame[cnt++] = START_FRAME;
    frame[cnt++] = command;
    frame[cnt++] = variable;
    for (i = 0; i < value_size; i++) {
        frame[cnt++] = value[i];
    }
    frame[cnt++] = END_FRAME;
    chk = compute_checksum(frame, cnt - 1); 
    if (IS_CTL_CHARACTER(chk)) {
        frame[cnt++] = ESC_CHAR;
        frame[cnt++] = (chk != ESC_CHAR) ? (chk | OR_VALUE) : chk;
    }
    else {
        frame[cnt++] = chk;
    }
    *frame_size_out = cnt;
    return frame;
}


bool volume_up()
{
    uint8_t *frame;
    size_t  frame_len;

    frame = set_frame(CMD_IR, 0x88, NULL, 0, &frame_len);
    send_frame(frame, frame_len);
    return true;
}



bool volume_down()
{
    uint8_t *frame;
    size_t   frame_len;

    frame = set_frame(CMD_IR, 0x8c, NULL, 0, &frame_len);
    send_frame(frame, frame_len);
    return true;
}

bool query_volume()
{

    uint8_t *frame;
    size_t  frame_len;

    frame = set_frame(CMD_QUERY, VAR_VOL, 0, 0, &frame_len);
    send_frame(frame, frame_len);

    return true;
}


bool query_input()
{

    uint8_t *frame;
    size_t  frame_len;

    frame = set_frame(CMD_QUERY, VAR_INPUT, 0, 0, &frame_len);
    send_frame(frame, frame_len);
    return true;
}


bool query_freq()
{

    uint8_t *frame;
    size_t  frame_len;

    frame = set_frame(CMD_QUERY, VAR_AM, 0, 0, &frame_len);
    send_frame(frame, frame_len);
    frame = set_frame(CMD_QUERY, VAR_FM, 0, 0, &frame_len);
    send_frame(frame, frame_len);
    return true;
}




bool set_volume(int8_t vol)
{
    uint8_t *frame;
    size_t  frame_len;
    static uint8_t buf[16];
    size_t cnt = 0;

    if (IS_CTL_CHARACTER(vol)) {
        buf[cnt++] = ESC_CHAR;
        buf[cnt++] = (vol != ESC_CHAR) ? (vol | OR_VALUE) : vol;
    }
    buf[cnt++] = vol;
    frame = set_frame(CMD_SET, VAR_VOL, buf, cnt, &frame_len);
    send_frame(frame, frame_len);
    return true;
}


bool parse_frame(uint8_t * frame, size_t len)
{
    int8_t value;
    if (frame[1] == CMD_QUERY) {
        value = (int8_t)frame[3];
        switch (frame[2]) {
        case VAR_INPUT:
            s_amp.input = (uint16_t)value;
            s_amp.input_set = true;
            break;
        case VAR_VOL:
            s_amp.volume = (int)value;
            s_amp.volume_set = true;
            break;
        case VAR_AM:
            s_amp.am = true;
            s_amp.freq_am = ((uint16_t)frame[4] << 8) | frame[3];
            break;
        case VAR_FM:
            s_amp.am = false;
            s_amp.freq_fm = ((uint16_t)frame[4] << 8) | frame[3];
            break;
        default:
            break;
        }
    }
}


#if 0

static void show_stats_on_lcd()
{
    uint16_t fg, bg, i;
    s_display.clearDisplay();
    static bool flip = false;
    unsigned int ch;

    s_display.setTextColor(SSD1306_WHITE);
    s_display.setCursor(0, 0);
    s_display.print("DBG        : "); s_display.println(s_stats.dbg_cnt, DEC);
    s_display.print("TX         : "); s_display.println(s_stats.tx_cnt, DEC);
    s_display.print("RX         : "); s_display.println(s_stats.rx_cnt, DEC);
    s_display.print("RX Timeout : "); s_display.println(s_stats.rx_timeout, DEC);
    s_display.print("RX Good Fr : "); s_display.println(s_stats.rx_good_frame, DEC);
    s_display.print("RX Bad Fr  : "); s_display.println(s_stats.rx_bad_frame, DEC);
    s_display.display();      

}

#endif


bool show_indicators(uint16_t ident)
{
    int i;
    bool found = false;
    input_info_t ii;

    for (i = 0; i < ARRAY_SIZE(s_input_info) ; i++) {
        memcpy_P(&ii, &s_input_info[i], sizeof(input_info_t));
        if (ii.ident == ident) {
            display_text_in_rect(ii.x, ii.y, ii.text_size, ii.text);
            found = true;
            break;
        }
    }
    return found;
}


uint8_t compute_crc8(int8_t *data, int len) 
{
    uint8_t crc = 0x00;
    for (int i = 0; i < len; i++) {
        uint8_t inByte = (uint8_t)data[i];
        crc ^= inByte;
        for (uint8_t j = 8; j; j--) {
            bool mix = (crc & 0x80);
            crc <<= 1;
            if (mix) {
                crc ^= 0x07; 
            }
        }
    }
    return crc;
}



void write_data(int start_address, uint8_t *data, int len)
{
    uint8_t crc;
    for (int i = 0; i < len; i++) {
        EEPROM.update(start_address++, data[i]);
    }
    crc = compute_crc8(data, len);
    EEPROM.update(start_address, crc);
}

bool read_data(int start_address, uint8_t *data, int len) 
{
    for (int i = 0; i < len; i++) {
        data[i] = EEPROM.read(start_address++);
    }
    uint8_t crc_stored = EEPROM.read(start_address);
    uint8_t crc_computed = compute_crc8(data, len);

    return crc_stored == crc_computed;
}

void show_data_on_display()
{
    static uint8_t buf[64];
    int text_size = 3;
    int y         = 20;

    s_display.clearDisplay();
    show_indicators(s_amp.input);
    if (s_amp.input == INPUT_TUNER) {
        if (s_amp.am) {
            show_indicators(INPUT_AM);
            snprintf((char *)buf, sizeof(buf) - 1, "%uK", s_amp.freq_am);
        }
        else {
            show_indicators(INPUT_FM);
            snprintf((char *)buf, sizeof(buf) - 1, "%u.%02uM", s_amp.freq_fm / 100, s_amp.freq_fm % 100 );
        }
        text_size = 2;
        y         = 30;
        s_display.setTextSize(text_size);
        s_display.setTextColor(SSD1306_WHITE);
        s_display.setCursor(0, 10);
        s_display.print((char *)buf);
    }
    snprintf((char *)buf, sizeof(buf) - 1, "%ddB", s_amp.volume);
    s_display.setTextSize(text_size);
    s_display.setTextColor(SSD1306_WHITE);
    s_display.setCursor(0, y);
    s_display.print((char *)buf);
    s_display.display();
}

void setup() 
{
    Serial.begin(9600);
    set_comm_timeout(20, 200);

    s_display.begin(SSD1306_SWITCHCAPVCC);
    s_display.clearDisplay();
    s_display.display();
    s_amp.input_set   = false;
    s_amp.volume_set  = false;
    s_amp.last_input  = MAX_VOLS;
    s_amp.last_volume = -127;

    if (!read_data(0, s_vols, ARRAY_SIZE(s_vols))) {
        memset(s_vols, 0, sizeof(s_vols));
        write_data(0, s_vols, ARRAY_SIZE(s_vols));
    }
}



void loop() 
{
    static uint8_t buf[64];
    static size_t len_rx;
    static int    ms, i;

    while ((receive_frame(buf, sizeof(buf), &len_rx))) {
        parse_frame(buf, len_rx);
    }
    ms = millis();
    if (s_amp.input_set == false || s_amp.volume_set == false) {
        if ((ms - s_last_query) > 1000) {
            s_last_query = ms;
            query_freq();
            query_input();
            query_volume();
        }
    }
    else {
        if (s_amp.last_input != s_amp.input) {
            s_amp.last_input = s_amp.input;
            set_volume(s_vols[s_amp.input]);
        }
        if (s_amp.last_volume != s_amp.volume) {
            s_vols[s_amp.input] = s_amp.volume;
            s_amp.last_volume = s_amp.volume;
            write_data(0, s_vols, ARRAY_SIZE(s_vols));
        }
        show_data_on_display();
    }
}








