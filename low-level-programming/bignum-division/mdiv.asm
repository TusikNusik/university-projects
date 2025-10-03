global mdiv

	; Zastosowania rejestrów: rdi - wskaźnik na wejściową tablicę, rsi - rozmiar,
	; tablicy, rcx - interator pętli programu, rdx - reszta z dzielenia,
	; r8 - dzielnik,  r9 - przechowuje informację o znakach dzielnej i dzielnika
	; (0 - oba dodatnie, 1 - dzielna ujemna, 2 - dzielnik ujemny, 3 - oba ujemne)
	; r10 - indykuje czy należy wykonać dzielenie tablicy, (0 - tak, pp. - nie).

mdiv:
	xor r10, r10
        mov r8, rdx	; Wartość dzielnika początkowo w rdx.
	xor r9, r9

	cmp qword [rdi + 8 * rsi - 8], 0	; Sprawdzam czy dzielna jest liczbą dodatnią.
	jge .divisor	; Jeżeli jest liczbą dodatnią to omijam proces zmiany znaku.
	inc r9

.set_values:
	xor rcx, rcx
	; Rejestr rax przechowuje 1, którą należy dodać po negacji bitów, po dodaniu
	; zmienia wartość na 0, lub zachowuje wartość 1 jeżeli nastąpiło przeniesienie.
	mov rax, 1

.change_sign_dividend:    ; Pętla zmienia zgodnie z regułami systemu U2 znak dzielnej.
	not qword [rdi + 8 * rcx]
	add [rdi + 8 * rcx], rax
	jz .skip	; Nastąpiło przeniesienie, nie zeruję rejestru rax.
	xor rax, rax
.skip:
	inc rcx
	cmp rcx, rsi
	jl .change_sign_dividend

	cmp r10, 1	; Sprawdzam czy dzielenie zostało już wykonane, mogę zwrócić wynik.
	je .negate_remainder 	; Dla r9 = 1 zwracam ujemną resztę.
	jg .return

.divisor:
	cmp r8, 0	; Sprawdzam czy należy zmienić znak dzielnika.
	je .return_overflow	; Dzielenie przez 0 powoduje overflow.
	jge .calculation

	add r9, 2	; Zapamiętuję, że dzielnik był ujemny i zmieniam mu znak.
	not r8
	add r8, 1

.calculation:
        xor rdx, rdx 	; Zeruję rdx, aby dzielenie zadziałało poprawnie.
	mov rcx, rsi
	; Pętla wykonuje się od rcx = rsi do 1 (aby dostać oczekiwany adres odejmuję 8 bajtów).
	; Dzieli kolejne 128 bitowe liczby typu unsigned.
	; Proces podobny do 'dzielenia pisemnego', reszta z dzielenia trafia do rdx.
.divide:
        mov rax, [rdi + 8 * rcx - 8]
        div r8
        mov [rdi + 8 * rcx - 8], rax	; W tablicy na bierząco zapisuję całkowity wynik dzielenia.
        loop .divide

	cmp r9, 0	; Dzieliliśmy dodatnią przez dodatnią nie ma możliwości na overflow.
	je .return
	cmp r9, 3
	je .check_overflow

	mov r10, r9 	; Po wykonaniu .divide iloraz jest dodatni, nawet dla r9 = 1, r9 = 2.
	jmp .set_values     ; Z tego powodu  wykonuję skok bezwarunkowy by zmienić znak.

	; Overflow występuję dla INT_MIN / - 1 (INT_MIN zależy ilu bitową liczbą jest dzielna).
.check_overflow:
	mov rax, [rdi + 8 * rsi - 8]	; Rozpatruje najbardziej znaczące bity.
	neg rax				; Zanegowanie INT_MIN spowoduje overflow.
	jo .return_overflow

.negate_remainder:
	neg rdx
.return:
	mov rax, rdx
	ret

.return_overflow:
	xor r9, r9
	div r9    ; Wypisuję SIGFPE
