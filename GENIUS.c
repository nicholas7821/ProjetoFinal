#include "pico/stdlib.h"
#include "inc/JoystickPi.h"
#include "inc/ButtonPi.h"
#include "inc/BuzzerPi.h"
#include "inc/melody.h"
#include <stdio.h>
#include <math.h>

// Definições de Hardware
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define BUZZER_PIN 21
#define UPDATE_MS 100

// Estruturas de Dados
typedef struct {
    int current_note;
    bool is_playing;
    absolute_time_t next_note_time;
    float freq_mult;
    int current_freq;
} PlayerState;

typedef struct {
    int *melody;
    int *durations;
    int length;
    const char *name;
} Melody;

// Variáveis Globais
const Melody melodies[] = {
    {AsaBrancaMelody, AsaBrancaDurations, sizeof(AsaBrancaMelody)/sizeof(int), "Asa Branca"},
    {ForEliseMelody, ForEliseDurations, sizeof(ForEliseMelody)/sizeof(int), "Für Elise"},
    {CanoninDMelody, CanoninDurations, sizeof(CanoninDMelody)/sizeof(int), "Canon in D"}
};

volatile struct {
    int index;
    bool a_pressed;
    bool b_pressed;
} buttons = {0};

PlayerState player = {0};

// Protótipos
void init_hardware();
void handle_input();
void update_sound();
void show_status();

// Callbacks estáticos para os botões
static void btn_a_callback() { buttons.a_pressed = true; }
static void btn_b_callback() { buttons.b_pressed = true; }

int main() {
    stdio_init_all();
    init_hardware();

    printf("=== Instrumento Musical ===\n");
    printf("Controles:\n");
    printf("A: Proxima musica | B: Play/Pause\n");

    while(true) {
        handle_input();
        update_sound();
        show_status();
        sleep_ms(10);
    }
    return 0;
}

void init_hardware() {
    joystickPi_init();
    initialize_pwm(BUZZER_PIN);
    
    ButtonPi btn_a, btn_b;
    ButtonPi_init(&btn_a, BUTTON_A_PIN);
    ButtonPi_init(&btn_b, BUTTON_B_PIN);
    
    ButtonPi_attach_callback(&btn_a, btn_a_callback);
    ButtonPi_attach_callback(&btn_b, btn_b_callback);
}

void handle_input() {
    // Botão A: Troca de música
    if(buttons.a_pressed) {
        buttons.index = (buttons.index + 1) % (sizeof(melodies)/sizeof(Melody));
        printf("\nMusica selecionada: %s\n", melodies[buttons.index].name);
        buttons.a_pressed = false;
        player.is_playing = false;
    }
    
    // Botão B: Play/Pause
    if(buttons.b_pressed) {
        player.is_playing = !player.is_playing;
        player.current_note = 0;
        buttons.b_pressed = false;
    }
}

void update_sound() {
    if(!player.is_playing) {
        pwm_set_gpio_level(BUZZER_PIN, 0);
        return;
    }

    // Atualiza frequência pelo joystick
    joystick_state_t js = joystickPi_read();
    player.freq_mult = 0.5f + (js.x / 4095.0f);

    // Toca próxima nota
    if(time_reached(player.next_note_time)) {
        if(player.current_note >= melodies[buttons.index].length) {
            player.is_playing = false;
            return;
        }

        int original = melodies[buttons.index].melody[player.current_note];
        int duration = melodies[buttons.index].durations[player.current_note];

        if(original > 0) {
            player.current_freq = original * player.freq_mult;
            play_tone(BUZZER_PIN, player.current_freq, duration);
        } else {
            player.current_freq = 0;
            sleep_ms(duration);
        }

        player.current_note++;
        player.next_note_time = make_timeout_time_ms(duration);
    }
}

void show_status() {
    static absolute_time_t last = 0;
    if(time_reached(last)) {
        joystick_state_t js = joystickPi_read();
        printf("\rX: %-4d | Y: %-4d | Freq: %-4d Hz   ", 
              js.x, js.y, player.current_freq);
        fflush(stdout);
        last = make_timeout_time_ms(UPDATE_MS);
    }
}