; Główne przeznaczenia rejestrów: r8 - przetrzymuje file descriptor pliku podanego na wejściu, 
; r9 - głownie odpowiadąjący za interację po pętlach lub przetrzymywanie wartości z/do wywołań systemowych,
; r10 - przetrzywuje offset pliku, który pozwala zidentyfikować koniec czytania danych,
; rsi - przechowywanie napisów,  rax - rejestr używany do wywołań funkcji systemowych i sprawdzania
; ich poprawności, przechowuje potęgi dwójki potrzebne do sprawdzania zapaleń bitów,
; rdi - zawiera aktualną wartość wielomianu crc,
; rcx, rdx, r11 - brak określonego zastosowania.

; Pisząć WIELOMIAN WEJŚCIOWY mam na myśli wielomian podany na wejściu bez współczynnika przy najw. potędze.

; Stałe które są konieczne do załadowania do rejestru rax aby wywołac określoną funkcję systemową.
SYS_READ equ 0
SYS_WRITE equ 1
SYS_OPEN equ 2
SYS_CLOSE equ 3
SYS_EXIT equ 60
SYS_LSEEK equ 8

DUZY equ 0x8000000000000000   ; 2^63

section .bss
	BUFOR: resw 1              ; Przechowuje 16-bitową liczbę reprezentującą dlugość danych w fragmencie.
	DANE: resq 512             ; Ten rozmiar bufora ma tyle bajtów co ma jedna strona, szybsze działanie prog. z mniejszą ilościa danych.
	SHIFT: resd 1              ; Przechowuje 32-bitowe przesunięcie offsetu pliku.
	RESZTA: resq 1             ; Przechowuje 64-bitowa liczbę, slużącą do wyznaczenia crc.
	WIELOMIAN: resb 65         ; Wielomian wejściowy, maksymalna dlugość wielomianu + znak końca napisu (1 bajt).
	WARTOSC: resq 1            ; Liczba 64-bitowa reprezentująca wartość wielomianu podanego na wejściu.
	POTEGA:  resq 1            ; 64-bitowa potęga dwójki indykujaca najwiekszy stopien wielomianu.
	DLUGOSC: resb 1            ; Przechowuje dlugość wielomianu wejściowego, max 64.
section .text
global _start

_start:
	                           ; Wierzchołek stosu przechowuje liczbe argumentów podaną na wejściu, sprawdzam czy wynosi 2.
	mov rax, [rsp]
	cmp rax, 3                 ; Błąd - niepoprawna liczba argumentów.
	jne .error_no_file

	mov rsi, [rsp + 24]        ; Czwartym argumentem na stosie powinnien być wielomian, który jest napisem.
	xor rdx, rdx

.dlugosc:                          ; Obliczam dlugość wielomianu podanego na wejściu.
	cmp byte [rsi + rdx], 0    ; Znak końca napisu.
        je .dlugosc_koniec
        inc rdx
        jmp .dlugosc
.dlugosc_koniec:

	mov [DLUGOSC], dl

	test rdx, rdx             ; Błąd - wielomian stały.
	jz .error_no_file

	cmp rdx, 64               ; Błąd - podany wielomian jest zbyt dużego stopnia.
	jg .error_no_file

        mov rcx, rdx
        mov rdi, WIELOMIAN        ; Kopiuje wielomian z wejścia do bufora WIELOMIAN.
        rep movsb

	movzx r9, byte [DLUGOSC]  ; Przygotowywuję rejestry potrzebne do wykonania pętli wyznacz_potęgę oraz wyznacz_wartosc.
        xor r10, r10
        xor rdi, rdi

	mov rax, 1
	mov rcx, rdx              ; Przygotywuję dane do mnożenie rax przez dwójkę, 'rdx' razy.
.wyznacz_potege:                  ; Wyznaczam potęgę dwójki, której współczynnik odpowiada stopniu wielomianu (uzwzględnia niepodany współczynnik).
	shl rax, 1
	jo .pocz_wyjatku          ; Jeżeli wielomian jest stopnia 65 to tutaj występuje overflow.
	loop .wyznacz_potege

	mov [POTEGA], rax
.wyznacz_wartosc:                            ; Pętle wyznacza wartość liczbową wielomianu podanego na wejściu oraz sprawdza poprawność danych.
	shr rax, 1
	jmp .koniec_wyjatku
.pocz_wyjatku:                               ; Jeżeli wystąpił ten wyjątek to wielomian jest 65 stopnia, czyli porównuję bity od największej potęgi.
	mov rax, DUZY
.koniec_wyjatku:
	cmp byte [WIELOMIAN + rdi], '1'
	jg .error_no_file                    ; Błąd - kod ASCII znaku większy od kodu '1'.
	jne .pomin
	add r10, rax                         ; Dodaję do wyniku odpowiednie potęgi dwójki.
.pomin:
	cmp byte [WIELOMIAN + rdi], '0'      ; Błąd - wielomian składa sie z innych znaków niż '0' lub '1'.
	jl .error_no_file
	inc rdi
	cmp rdi, r9
	jl .wyznacz_wartosc

	mov [WARTOSC], r10

	mov rax, SYS_OPEN
	mov rdi, [rsp + 16]                  ; Trzecim parametrem od wierzchołka stosu, powinna być ścieżka do pliku.
	xor rsi, rsi                         ; Plik otwieram tylko z zamiarem czytania.
	mov rdx, 0444o
	syscall

	test rax, rax                        ; Błąd - SYS_OPEN zakończyło się kodem mniejszym od zera, plik otwarty niepoprawnie.
	js .error

	mov r8, rax                          ; Zapisuję file_descriptor w rejestrze r8.
	xor rax, rax
.poczatek:
	mov r10, rax                         ; Zapisuje początkową pozycję file offsetu w r10.

	mov rdi, r8
	mov rax, SYS_READ
	mov rsi, BUFOR                       ; Długość danych wpisuję do bufora BUFOR.
	mov rdx, 2                           ; Czytam tylko 2 bajty.
	syscall

	test rax, rax                        ; Błąd - SYS_READ zakończyło się kodem mniejszym lub równym zeru, niepoprawny odczyt.
	js .error
	jz .error

	xor rdx, rdx
	xor rcx, rcx

	movzx  r9, word [BUFOR]              ; Odczytuję długość danych z bufora.
	cmp r9, 0
	je .koniec_podzialu_danych           ; Ifuję przypadek, gdy dane mają długość zero.

	mov rcx, r9
	shr rcx, 12                          ; Wyznaczam wynik dzielenia całkowitego długości danych przez rozmiar bufora DANE.
	mov rax, rcx
	shl rax, 12                          ; Wyznaczam resztę z dzielenia długości danych przez rozmiar bufora DANE.
	sub r9, rax
	inc rcx

	; Pętla czyta dane w blokach których rozmiar jest <= 4096.
.podziel_dane:
	push rcx
	mov rax, SYS_READ
	mov rsi, DANE                        ; Dane zostaną wczytane do bufora DANE.
	mov rdi, r8
	mov rdx, r9
	syscall
	pop rcx

	test rax, rax                        ; Błąd - SYS_READ zakończyło się kodem mniejszym lub równym zeru.
        js .error
        jz .error


	xor rdi, rdi
	xor r11, r11

	mov rdi, [RESZTA]                    ; Bufor RESZTA zawiera wartość wielomianu crc z poprzedniej iteracji.
	xor rdx, rdx
	xor rax, rax
	mov rsi, DUZY

	; Zadaniem tej pętli jest wyznaczanie kolejnych wartości wielomianu crc na podstawie otrzymanych danych.
	; Otrzymany blok danych przetwarzam bajt po bajcie, dla każdej liczby 8-bitowej sprawdzam jej zapis binarny.
	; Wartość w rejestrze rdi jest wielomianem crc, wykonuję algorytm xorowania kolejnych spójnych fragmentów
	; danych z wartością w [WARTOSC] (xorowanie w słupku),. Po wykonaniu pętli resztę zapisuje w buforze RESZTA.
	; Osobno rozpatruję przypadek gdy wielomian jest stopnia 65, aby nie było overflowow i program poprawnie działał.
.petla_dane:
	mov dl, byte [DANE + r11]
	mov al, 0b10000000                 ; Rejestr al służy do sprawdzania zapaleń bitów 8-bitowej liczby w dl.
.petla_bajt:
	cmp rdi, rsi
	jae .przepelnienie                 ; Przypadek - wielomian 65 stopnia.
	shl rdi, 1

	test al, dl
	jz .pomin_dodanie_1                ; Bit nie jest zapalony.
	inc rdi

.pomin_dodanie_1:
	test rdi, [POTEGA]                 ; Jeżeli ten bit w rdi jest zapalony to oznacza, że należy wykonać operację xor.
        jz .brak_xor
        sub rdi, [POTEGA]                  ; Usuwam wiodącą jedynkę, aby wartość zgadzała się z wielomianem podanym na wejściu.
	xor rdi, [WARTOSC]
	jmp .brak_xor
.przepelnienie:
	sub rdi, rsi                       ; Najpierw odejmuję wiodącą jedynkę (odejmuję 2^63), a następnie przesuwam binarnie
	shl rdi, 1                         ; o jeden w lewo i sprawdzam czy najmniejszy bit zapalony, aby unknąć overflow.
	test al, dl
	jz .przepelnienie_xor
	inc rdi
.przepelnienie_xor:
	xor rdi, [WARTOSC]
.brak_xor:
	shr al, 1                          ; Pętla petla_bajt wykonuję się 8 razy aż rejestr al bedzie wynosił 0b00000000.
	cmp al, 0
	jne .petla_bajt

	inc r11
	cmp r11, r9
	jne .petla_dane

	mov [RESZTA], rdi

.koniec_petli_dane:
	mov r9, 4096                      ; Po wykonaniu pierwszej iteracji rozmiar danych powinien być wielokrotnością liczby 4096.
	dec rcx
	cmp rcx, 0
	jne .podziel_dane

.koniec_podzialu_danych:

	mov rax, SYS_READ
	mov rdi, r8
	mov rsi, SHIFT
	mov rdx, 4                      ; Przesunięcie jest liczba 4-bajtową.
	syscall

	test rax, rax                   ; Błąd - SYS_READ zakończyło się kodem mniejszym lub równym zeru.
        js .error
	jz .error

	mov eax, [SHIFT]                ; Używam więc rejestru 32-bitowego.
	cdqe                            ; Rozszerzam liczbę która jest typu signed do 64-bitowej.
	mov r9, rax

	mov rax, SYS_LSEEK
	mov rdi, r8
	mov rsi, r9                     ; Przesuwam file offset o odczytane przesunięcie.
	mov rdx, 1                      ; Tryb przesunięcia, 1 oznacza że przesuwam od aktualnego offsetu o ilość bajtów w r9.
	syscall

	test rax, rax                   ; Błąd - SYS_LSEEK zakończyło się kodem mniejszym od zera.
        js .error

	xor r10, rax                    ; Przesunięcie na identyczny offset co ostatnio - koniec pętli.
	jnz .poczatek

	movzx r9, byte [DLUGOSC]
	mov rdi, [RESZTA]
	mov rcx, DUZY

	; Pętla dokłada liczbę zer równą [DLUGOSC], aby poprawnie policzyć crc, proces analogiczny co powyżej.
.wyznacz_crc:
	cmp rdi, rcx
	jae .przepelnienie_crc          ; Przypadek wielomianu stopnia 65.
	shl rdi, 1

	test rdi, [POTEGA]              ; Sprawdzam czy najbardziej znaczący bit zapalony.
	jz .brak_xor_crc
        sub rdi, [POTEGA]
	xor rdi, [WARTOSC]
	jmp .brak_xor_crc
.przepelnienie_crc:
	sub rdi, rcx
	shl rdi, 1
        xor rdi, [WARTOSC]
.brak_xor_crc:
	dec r9
	cmp r9, 0
	jne .wyznacz_crc
                                          ; Wynik czyli rdi w systemie binarnym zapisze w miejsu bufora WIELOMIAN.

	movzx r10, byte [DLUGOSC]
	mov byte [WIELOMIAN + r10], 10    ; Dopisuję kod znaku końca linni do napisu.
	mov rax, 1
	mov r11, 2
	mov rcx, r10

        ; Sprawdzam zapalenie kolejnych bitów w rdi i wpisuję do bufora WIELOMIAN odpowiednie znaki.
.na_binarny:
	test rdi, rax                    ; Oczywiście porównuję z kolejnymi potęgami dwójki.
	jz .bit_0                        ; Bit nie jest zapalony na 'rcx - 1' pozycjji wpisuję znak '0'.
.bit_1:
	mov byte [WIELOMIAN + rcx - 1], '1'
	jmp .bit_dodany
.bit_0:
	mov byte [WIELOMIAN + rcx - 1], '0'
.bit_dodany:
	mul r11
	inc r9
	loop .na_binarny

	mov rax, SYS_WRITE
	mov rdi, 1                       ; Jedynka bo wypisuję na standardowe wyjściowe.
	mov rsi, WIELOMIAN
	movzx rdx, byte [DLUGOSC]
	inc rdx                          ; Wypisuję ilość znaków równą wielomianowi crc + znak końca linii ('\n').
	syscall

	test rax, rax
	js .error

.koniec_programu:
	mov rax, SYS_CLOSE
	mov rdi, r8
	syscall

	test rax, rax                   ; Błąd - SYS_CLOSE zakończyło się kodem mniejszym od zera.
        js .error_no_file               ; Po niedanym zamknięciu kończę program.

	mov rax, SYS_EXIT               ; Kończe program z kodem 0, jeżeli wszystko wykonało sie jak powinno.
	mov rdi, 0
	syscall
.error:                                 ; Wykonuje się wtw. jeżeli wcześniej w programie wystąpił bład.
	mov rax, SYS_CLOSE
        mov rdi, r8
        syscall
	                                ; Nie sprawdzam poprawności zakończenia SYS_CLOSE, ponieważ i tak zakończe program z kodem 1.
.error_no_file:
        mov rax, SYS_EXIT
        mov rdi, 1
        syscall
