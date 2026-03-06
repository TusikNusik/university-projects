#include <delay.h>
#include <gpio.h>
#include <stm32.h>
#include <stdbool.h>

// ------------------ PILOT ------------------
// Wartości w systemie dziesiętnym.
// ------------------ NUMERY GUZIKÓW ------------------
#define ONE_BUTTON 0
#define TWO_BUTTON 1
#define THREE_BUTTON 2
#define FOUR_BUTTON 3
#define FIVE_BUTTON 4
#define SIX_BUTTON 5
#define SEVEN_BUTTON 6
#define EIGHT_BUTTON 7
#define NINE_BUTTON 8
#define ZERO_BUTTON 9
#define UP_VOLUME_BUTTON 18
#define DOWN_VOLUME_BUTTON 19
#define UP_BUTTON 116
#define DOWN_BUTTON 117
#define UP_CHANNEL_BUTTON 16
#define DOWN_CHANNEL_BUTTON 17


#define RED_BUTTON 37
#define GREEN_BUTTON 38
#define BLUE_BUTTON 36
#define YELLOW_BUTTON 39


// ------------------ NUMERY URZĄDZEŃ ------------------
#define TV_NUMBER 1
#define OTHER_DEVICE_NUMBER 23

// ------------------ SIRC ------------------
// Time unit is 10µs (microseconds)
#define START_BIT_LOW 180       // Start bit = 2400µs
#define START_BIT_HIGH 300  
#define ONE_BIT_LOW 90          // One bit = 1200µs
#define ONE_BIT_HIGH 150
#define ZERO_BIT_LOW 30         // Zero bit = 600µs
#define ZERO_BIT_HIGH 90
#define START_BLOCK_HIGH 360    // Start block = 3000µs (Start bit + 600µs)
#define START_BLOCK_LOW 240
#define ONE_BLOCK_LOW 120       // One block  = 1800µs (One bit + 600µs)
#define ONE_BLOCK_HIGH 240
#define ZERO_BLOCK_LOW 80       // Zero block = 1200µs (Zero bit + 600µs)
#define ZERO_BLOCK_HIGH 160
#define SIRC_LENGTH 12

// ------------------ Licznik ------------------
#define SECOND_5 5000 
#define SECOND_10 10000
#define ONE_MILISECOND_TIMER 15999
#define TEN_MICROSECONDS_TIMER 159
#define NINETY_MILISECONDS_ARR 90
#define SECOND_QUARTER 250
// ------------------ Diody ------------------

#define RED_LED_GPIO GPIOA
#define GREEN_LED_GPIO GPIOA
#define BLUE_LED_GPIO GPIOB
#define GREEN2_LED_GPIO GPIOA
#define RED_LED_PIN 6
#define GREEN_LED_PIN 7
#define BLUE_LED_PIN 0
#define GREEN2_LED_PIN 5
#define RED_MAX_BRIGHTNESS 1000
#define RED_MIN_BRIGHTNESS 0
#define RED_START_BRIGHTNESS 500
#define BLUE_MAX_BRIGHTNESS 1000
#define BLUE_MIN_BRIGHTNESS 0
#define BLUE_START_BRIGHTNESS 500
#define GREEN_MAX_BRIGHTNESS 1000
#define GREEN_MIN_BRIGHTNESS 0
#define GREEN_START_BRIGHTNESS 500



#define RedLEDon() \
    RED_LED_GPIO->BSRR = 1 << (RED_LED_PIN + 16)
#define RedLEDoff() \
    RED_LED_GPIO->BSRR = 1 << RED_LED_PIN

#define GreenLEDon() \
    GREEN_LED_GPIO->BSRR = 1 << (GREEN_LED_PIN + 16)
#define GreenLEDoff() \
    GREEN_LED_GPIO->BSRR = 1 << GREEN_LED_PIN

#define BlueLEDon() \
    BLUE_LED_GPIO->BSRR = 1 << (BLUE_LED_PIN + 16)
#define BlueLEDoff() \
    BLUE_LED_GPIO->BSRR = 1 << BLUE_LED_PIN

#define Green2LEDon() \
    GREEN2_LED_GPIO->BSRR = 1 << GREEN2_LED_PIN
#define Green2LEDoff() \
    GREEN2_LED_GPIO->BSRR = 1 << (GREEN2_LED_PIN + 16)

// ------------------ PRZYCISKI ------------------
// ------------------ AKTYWNY 0 ------------------

#define USER_BTN_GPIO GPIOC
#define USER_BTN_PIN 13

#define LEFT_BTN_GPIO GPIOB
#define LEFT_BTN_PIN 3
#define RIGHT_BTN_GPIO GPIOB
#define RIGHT_BTN_PIN 4
#define UP_BTN_GPIO GPIOB
#define UP_BTN_PIN 5
#define DOWN_BTN_GPIO GPIOB
#define DOWN_BTN_PIN 6
#define ACTION_BTN_GPIO GPIOB
#define ACTION_BTN_PIN 10

// ------------------ AKTYWNY 1 ------------------

#define AT_BTN_GPIO GPIOA
#define AT_BTN_PIN 0

// ------------------ USART ------------------

#define USART_Mode_Rx_Tx (USART_CR1_RE | USART_CR1_TE)
#define USART_Enable USART_CR1_UE
#define USART_WordLength_8b 0x0000
#define USART_Parity_No 0x0000
#define USART_FlowControl_None 0x0000
#define USART_StopBits_1 0x0000     // Liczba bitów przerwy między komunikatami.
#define HSI_HZ 16000000U
#define PCLK1_HZ HSI_HZ             // Zmienne do ustalenia prędkości przesyłania danych.
#define BAUD 9600U

// ------------------ USART ------------------

// ------------------ RECEIVER ------------------

#define RECEIVER_GPIO GPIOA
#define RECEIVER_PIN 14

// ------------------ RECEIVER ------------------


#define BUFFOR_SIZE 10000

struct cyclic_buffer {
    char* start;
    char* end;
    char* head;
    char* tail;
    unsigned size;
};

typedef struct cyclic_buffer cyclic_buffer;


cyclic_buffer c;

void init(cyclic_buffer* c) {
    static char storage[BUFFOR_SIZE];  
    c->size = BUFFOR_SIZE;
    c->start = storage;
    c->end = c->start + c->size;
    c->head = c->start;
    c->tail = c->start;
}

bool is_empty(cyclic_buffer* c) {
    if(c->tail == c->head) {
        return true;
    }
    return false;
}

char* next_in_buffer(cyclic_buffer* c, char* pointer) {
    if(pointer + 1 == c->end) {
        return c->start;
    }
    return pointer + 1;
}

void push_back(cyclic_buffer* c, const char* s) {
    while(*s != '\0') {
        *(c->head) = *s;
        s++;
        char* next = next_in_buffer(c, c->head);
        if(next == c->tail) {
            break;
        }
        c->head = next;
    }
}

char pop_front(cyclic_buffer* c) {
    char rez = *(c->tail);
    c->tail = next_in_buffer(c, c->tail);
    return rez;
}

char* get_tail(cyclic_buffer* c) {
    return c->tail;
}

unsigned get_first_message_length(cyclic_buffer* c) {
    char* find_end = c->tail;
    unsigned length = 1;
    while(*find_end != '\n') {
        find_end = next_in_buffer(c, find_end);
        length++;
    }
    return length;
}

void pop_front_x_bytes(cyclic_buffer* c, unsigned pop_n) {
    for(unsigned i = 0; i < pop_n; i++) {
        pop_front(c);
    }
}

void send_message(cyclic_buffer* c) {
    DMA1_Stream6->M0AR = (uint32_t)get_tail(c);
    DMA1_Stream6->NDTR = get_first_message_length(c);
    DMA1_Stream6->CR |= DMA_SxCR_EN;
    
}

void DMA1_Stream6_IRQHandler() { 
    uint32_t isr = DMA1->HISR;         
    if (isr & DMA_HISR_TCIF6) {
        pop_front_x_bytes(&c, get_first_message_length(&c));

        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        
        if(!is_empty(&c)) {
            send_message(&c);
        }
    }
}

void check_if_available() {
    if((DMA1_Stream6->CR & DMA_SxCR_EN) == 0 && (DMA1->HISR & DMA_HISR_TCIF6) == 0) {
        send_message(&c);
    }
}

void unknown_command(cyclic_buffer* c) {
    push_back(c, "UNKNOWN COMMAND!\r\n");
}

char* encode_string(char* p, char* string) {
    while(*string) {
        *p++ = *string++;
    }
    return p;
}

char* decode_int_16(char *p, uint16_t v)
{
    char tmp[6];
    int i = 0;

    do {
        tmp[i] = '0' + (v % 10);
        i++;
        v /= 10;
    } while(v);

    i--;

    while(i >= 0) {
        *p++ = tmp[i];
        i--;
    }

    return p;
}

bool prevent_multiple_clicks(uint32_t last_command_time) {
    uint32_t difference = (uint32_t)(TIM2->CNT - last_command_time);
    if(difference > SECOND_QUARTER) {
        return false;
    }
    return true;
}

void handle_command(uint8_t command, uint8_t last_command, uint32_t last_command_time) {
    switch (command) {
        uint16_t current_brightness;
        case ONE_BUTTON:
            if((prevent_multiple_clicks(last_command_time) && last_command == command)) {
                break;
            }

            if(TIM3->CCR1 < RED_START_BRIGHTNESS) {
                TIM3->CCR1 = RED_MAX_BRIGHTNESS;
                push_back(&c, "RED ACTIVATED\r\n");
            }
            else  {
                TIM3->CCR1 = RED_MIN_BRIGHTNESS;
                push_back(&c, "RED DEACTIVATED\r\n");
            }
            check_if_available();
            break;

        case TWO_BUTTON:
            if((prevent_multiple_clicks(last_command_time) && last_command == command)) {
                break;
            }

            if(TIM3->CCR3 < BLUE_START_BRIGHTNESS) {    
                TIM3->CCR3 = BLUE_MAX_BRIGHTNESS;
                push_back(&c, "BLUE ACTIVATED\r\n");
            }
            else {
                TIM3->CCR3 = BLUE_MIN_BRIGHTNESS;
                push_back(&c, "BLUE DEACTIVATED\r\n");
            }
            check_if_available();
            break;

        case THREE_BUTTON:
            if((prevent_multiple_clicks(last_command_time) && last_command == command)) {
                break;
            }
            if(GREEN2_LED_GPIO->IDR & (1 << (GREEN2_LED_PIN))) {   
                Green2LEDoff();
                push_back(&c, "GREEN2 DEACTIVATED\r\n");
            }
            else {  
                Green2LEDon();
                push_back(&c, "GREEN2 ACTIVATED\r\n");
            }
            check_if_available();
            break;

        case FOUR_BUTTON:
            if((prevent_multiple_clicks(last_command_time) && last_command == command)) {
                break;
            }
            if(TIM3->CCR2 < GREEN_START_BRIGHTNESS) {  
                TIM3->CCR2 = GREEN_MAX_BRIGHTNESS;
                push_back(&c, "GREEN ACTIVATED\r\n");
            }
            else  {  
                TIM3->CCR2 = GREEN_MIN_BRIGHTNESS;
                push_back(&c, "GREEN DEACTIVATED\r\n");
            }
            check_if_available();
            break;

        case UP_VOLUME_BUTTON:
            current_brightness = TIM3->CCR1;
            if(current_brightness <= 989)   current_brightness += 10;
            TIM3->CCR1 = current_brightness;
            push_back(&c, "RED UP\r\n");
            check_if_available();
            break;

        case DOWN_VOLUME_BUTTON:
            current_brightness = TIM3->CCR1;
            if(current_brightness >= 10)    current_brightness -= 10;
            TIM3->CCR1 = current_brightness;
            push_back(&c, "RED DOWN\r\n");
            check_if_available();
            break;
        
        case UP_CHANNEL_BUTTON:
            current_brightness = TIM3->CCR2;
            if(current_brightness <= 989)   current_brightness += 10;
            TIM3->CCR2 = current_brightness;
            push_back(&c, "GREEN UP\r\n");
            check_if_available();
            break;
        
        case DOWN_CHANNEL_BUTTON:
            current_brightness = TIM3->CCR2;
            if(current_brightness >= 10)    current_brightness -= 10;
            TIM3->CCR2 = current_brightness;
            push_back(&c, "GREEN DOWN\r\n");
            check_if_available();
            break;

        case UP_BUTTON:
            current_brightness = TIM3->CCR3;
            if(current_brightness <= 989)   current_brightness += 10;
            TIM3->CCR3 = current_brightness;
            push_back(&c, "BLUE UP\r\n");
            check_if_available();
            break;

        case DOWN_BUTTON:
            current_brightness = TIM3->CCR3;
            if(current_brightness >= 10)    current_brightness -= 10;
            TIM3->CCR3 = current_brightness;
            push_back(&c, "BLUE DOWN\r\n");
            check_if_available();
            break;

        default:
            unknown_command(&c);
            check_if_available();
            break;
    }
}


uint8_t last_command = 127;
uint32_t last_time = 0;

void process_SIRC_signal(uint8_t string_frame) {
    uint8_t command = string_frame & 0x7F;          
    uint8_t address = (string_frame >> 7) & 0x1F;
    handle_command(command, last_command, last_time);

    char tx_buf[32];
    char *p = tx_buf;

    p = encode_string(p, "CMD=");
    p = decode_int_16(p, command);
    p = encode_string(p, " ADDR=");
    p = decode_int_16(p, address);
    p = encode_string(p, "\r\n");
    *p = '\0';
    
    push_back(&c, tx_buf);
    check_if_available();
    last_command = command;
    last_time = TIM2->CNT;
}


// Mierzę czasy zbocza opadającego, ile ono trwało, czyli fragmenty między dwoma sąsiednimi stanami wysokimi.
// Mierzę także czas całego "bit bloku", czyli czas między kolejnymi zboczami opadającymi.
uint16_t frame = 0;
uint8_t bit_cnt = 0;
bool transmission_started = false;
uint16_t last_low_state_time = 0;        // Czas od ostatniego startu stanu niskiego.
uint16_t last_low_block_duration = 0;   // Czas przez jaki trwał ostatni stan niski.
volatile bool data_ready = false;
volatile uint16_t string_frame = 0;

void reset_values() {
    bit_cnt = 0;
    frame = 0;
    transmission_started = false;
    last_low_block_duration = 0;
    last_low_state_time = 0;
}

void EXTI15_10_IRQHandler(void) {
    if(EXTI->PR & EXTI_PR_PR14) {
        EXTI->PR = EXTI_PR_PR14; 

        // Przerwanie wywołane zboczem opadającym.
        if(!((RECEIVER_GPIO->IDR & (1 << RECEIVER_PIN)))) {
            // Początek transmisji, pierwszy stan niski.
            if(!transmission_started && last_low_state_time == 0) {
                last_low_state_time = TIM4->CNT;
            }   // Drugi stan niski od początku transmisji, sprawdzam czy czas to mniej więcej 3000µs.
            else if(!transmission_started && last_low_state_time != 0) {
                uint16_t block_time = (uint16_t)(TIM4->CNT - last_low_state_time);
                // Bit startowy zaistniał
                if((block_time > START_BLOCK_LOW && block_time < START_BLOCK_HIGH) && (last_low_block_duration > START_BIT_LOW && last_low_block_duration < START_BIT_HIGH)) {
                    transmission_started = true;
                    last_low_state_time = TIM4->CNT;
                }   // Zły sygnał.
                else {
                    reset_values();
                }
            } // Kolejne stany niskie w transmiji, sprawdzam czy jednynka, zero, czy błąd.
            else if(transmission_started) {
                uint16_t block_time = (uint16_t)(TIM4->CNT - last_low_state_time);
                last_low_state_time = TIM4->CNT;
                if((block_time > ONE_BLOCK_LOW && block_time < ONE_BLOCK_HIGH) && (last_low_block_duration > ONE_BIT_LOW && last_low_block_duration < ONE_BIT_HIGH)) {
                    frame |= (1 << bit_cnt);
                    bit_cnt++;
                }
                else if((block_time > ZERO_BLOCK_LOW && block_time < ZERO_BLOCK_HIGH) && (last_low_block_duration > ZERO_BIT_LOW && last_low_block_duration < ZERO_BIT_HIGH)) {
                    bit_cnt++;
                }
                else {
                    reset_values();
                }
            }
        }   
        else {  // Przerwanie wywołane zboczem rosnącym.
            // Zaistniał stan niski, zapamiętuję jego czas.
            if(transmission_started || last_low_state_time != 0) {
                last_low_block_duration = TIM4->CNT - last_low_state_time;
            }
            // Protokół został w całości przesłany.
            if(bit_cnt == SIRC_LENGTH - 1) {
                if(last_low_block_duration > ONE_BIT_LOW && last_low_block_duration < ONE_BIT_HIGH) {
                    frame |= (1 << bit_cnt);
                }
                bit_cnt++;
                string_frame = frame;
                data_ready = true;
                reset_values();
                process_SIRC_signal(string_frame);
            }
        }
    }
}

int main() {

    init(&c);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
    RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_DMA1EN;

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // LICZNIK
    // Włączenie taktowania licznika.
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    

    __NOP();
    RedLEDoff();
    GreenLEDoff();
    BlueLEDoff();
    Green2LEDoff();

    GPIOoutConfigure(RED_LED_GPIO,
        RED_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);

    GPIOoutConfigure(GREEN_LED_GPIO,
        GREEN_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
    
    GPIOoutConfigure(BLUE_LED_GPIO,
        BLUE_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);
    
    GPIOoutConfigure(GREEN2_LED_GPIO,
        GREEN2_LED_PIN,
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL);

    GPIOafConfigure(GPIOA,
        2,
        GPIO_OType_PP,
        GPIO_Fast_Speed,
        GPIO_PuPd_NOPULL,
        GPIO_AF_USART2);
    
    GPIOafConfigure(GPIOA,
        3,
        GPIO_OType_PP,
        GPIO_Fast_Speed,
        GPIO_PuPd_UP,
        GPIO_AF_USART2); 

    GPIOafConfigure(GPIOA, 
        RED_LED_PIN, 
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL, 
        GPIO_AF_TIM3);
    
    
    GPIOafConfigure(GPIOA, 
        GREEN_LED_PIN, 
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL, 
        GPIO_AF_TIM3);
    
    
    GPIOafConfigure(GPIOB, 
        BLUE_LED_PIN, 
        GPIO_OType_PP,
        GPIO_Low_Speed,
        GPIO_PuPd_NOPULL, 
        GPIO_AF_TIM3);
    
    
    GPIOinConfigure(RECEIVER_GPIO, RECEIVER_PIN, GPIO_PuPd_UP, EXTI_Mode_Interrupt, EXTI_Trigger_Rising_Falling);
    
    USART2->CR1 = USART_Mode_Rx_Tx | USART_WordLength_8b | USART_Parity_No;
    USART2->CR2 = USART_StopBits_1;
    USART2->CR3 = USART_CR3_DMAT | USART_CR3_DMAR;
    USART2->BRR = (PCLK1_HZ + (BAUD / 2U)) / BAUD;

    DMA1_Stream6->CR = 4U << 25 |
        DMA_SxCR_PL_1 |
        DMA_SxCR_MINC |
        DMA_SxCR_DIR_0 |
        DMA_SxCR_TCIE;

    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;

    DMA1_Stream5->CR = 4U << 25 |
        DMA_SxCR_PL_1 |
        DMA_SxCR_MINC |
        DMA_SxCR_TCIE;
    
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;

    DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CTCIF5;

    EXTI->PR = EXTI_PR_PR14;

    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    USART2->CR1 |= USART_Enable;        

 
    TIM4->PSC = TEN_MICROSECONDS_TIMER; // 159 -> 16MHz/160 = 100kHz (1 cykl = 10us)
    TIM4->ARR = 0xFFFF;         // Maksymalna wartość, nie interesuja mnie przerwania zegara.
    TIM4->CR1 = 0;              // Wyłączony na start
    TIM4->CNT = 0;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR = 0;

    TIM4->CR1 |= TIM_CR1_CEN;
    

    TIM3->PSC = 15;
    TIM3->ARR = 999;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CCR1 = RED_START_BRIGHTNESS;
    TIM3->CCR2 = BLUE_START_BRIGHTNESS;
    TIM3->CCR3 = GREEN_START_BRIGHTNESS;

    // TIM2 -> timer który zapobiega wielokrotnym kliknięciom.
    TIM2->PSC = ONE_MILISECOND_TIMER;
    TIM2->ARR = 0xFFFFF;
    TIM2->CR1 = 0;
    TIM2->CNT = 0;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;

    // konfigurują kanały wejściowe i wyjściowe ich liczba zależy od konkretnego licznika (po dwa kanały w rejestrze)
    TIM3->CCMR1 = (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE) |
                  (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE);
    
    TIM3->CCMR2 =  (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE);
    // decyduje, czy kanał wyjściowy licznika steruje zewnętrznym wyprowadzeniem.

    // włączenie odpowiednich bitów aby podłączyć linię wyjściową do odpowiedniego wyprowadzenia
    TIM3->CCER = (TIM_CCER_CC1E | TIM_CCER_CC1P) |  // Czerwona dioda.
                 (TIM_CCER_CC2E | TIM_CCER_CC2P) |   // Zielona dioda.
                 (TIM_CCER_CC3E | TIM_CCER_CC3P);   // Niebieska dioda.

    TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

    return 0;
}