                section         .text

                global          _start
                
_start:
		sub             rsp, 4 * 128 * 8	; allocate memory for 2 long numbers and answer
		
                lea		rdi, [rsp + 2 * 128 * 8]; set the pointer to the beginning of the answer
                mov		rcx, 2 * 128		; size of answer can be up to 2 times greater than size of input numbers
                call		set_zero		; get rid of any trash in this part of memory
                lea		r9, [rsp + 2 * 128 * 8]	; put answer register to needed place
                
                mov             rcx, 128		; max size of input numbers in qwords
                
                lea             rdi, [rsp + 128 * 8]	; prepare for reading
                call            read_long		; read input #1
                mov             rdi, rsp		; prepare for reading
                call            read_long		; read input #2
                mov		rsi, rdi		; move rsi to the beginning of the second input number
                lea             rdi, [rsp + 128 * 8]	; move rdi to the beginning of the first input number
                
                call            mul_long_long		; do the multiplication
                
                mov             rcx, 2 * 128		; prepare for writing
                call            write_long		; write the answer

                mov             al, 0x0a
                call            write_char		; write the new line symbol

                jmp             exit
                
; multiplies two long numbers
;    rdi -- address of multiplier #1 (long number)
;    rsi -- address of multiplier #2 (long number)
;    rcx -- length of long number in qwords
; result:
;    product is written to rdi
mul_long_long:
		push 		rsi
		push 		rcx
	
		mov 		r14, rcx 		; start value of counter
		xor 		r13, r13 		; rdi offset
.outer_loop:
		xor 		r10, r10 		; will be used to save carry
		xor 		r11, r11 		; rsi offset
		mov 		rbp, r14 		; restart the counter for inner loop
		clc
.inner_loop:
		mov 		rax, [rsi + r11]	; get a piece of rsi
		mov 		rbx, [rdi + r13]	; get a piece of rdi
		mul 		rbx 			; mul rax and rbx, result in rax; overflow in rdx
		add 		rax, r10 		; add carry from prev circle
		adc 		rdx, 0 			; recalc new carry
		lea 		r15, [r13 + r11]	; result offset
		add 		[r9 + r15], rax 	; count result
		adc 		rdx, 0 			; recalc new carry again
		mov 		r10, rdx 		; save new carry
		
 		add 		r11, 8			; recalc rsi offset
		dec 		rbp			; recalc inner loop counter
		jnz 		.inner_loop		; go to next circle
		
		add 		r13, 8			; recalc rdi offset
		dec 		rcx			; recalc outer loop counter
		jnz 		.outer_loop		; go to next circle
		
		pop 		rcx
		pop		rsi
		
		mov 		rdi, r9			; move rdi to the beginning of the answer
		
		ret

; adds two long number
;    rdi -- address of summand #1 (long number)
;    rsi -- address of summand #2 (long number)
;    rcx -- length of long numbers in qwords
; result:
;    sum is written to rdi
add_long_long:
                push            rdi
                push            rsi
                push            rcx

                clc
.loop:
                mov             rax, [rsi]
                lea             rsi, [rsi + 8]
                adc             [rdi], rax
                lea             rdi, [rdi + 8]
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rsi
                pop             rdi
                ret

; adds 64-bit number to long number
;    rdi -- address of summand #1 (long number)
;    rax -- summand #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    sum is written to rdi
add_long_short:
                push            rdi
                push            rcx
                push            rdx

                xor             rdx,rdx
.loop:
                add             [rdi], rax
                adc             rdx, 0
                mov             rax, rdx
                xor             rdx, rdx
                add             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rdx
                pop             rcx
                pop             rdi
                ret
   
; multiplies long number by a short
;    rdi -- address of multiplier #1 (long number)
;    rbx -- multiplier #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    product is written to rdi
mul_long_short:
                push            rax
                push            rdi
                push            rcx

                xor             rsi, rsi
.loop:
                mov             rax, [rdi]
                mul             rbx
                add             rax, rsi
                adc             rdx, 0
                mov             [rdi], rax
                add             rdi, 8
                mov             rsi, rdx
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rdi
                pop             rax
                ret

; divides long number by a short
;    rdi -- address of dividend (long number)
;    rbx -- divisor (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    quotient is written to rdi
;    rdx -- remainder
div_long_short:
                push            rdi
                push            rax
                push            rcx

                lea             rdi, [rdi + 8 * rcx - 8]
                xor             rdx, rdx

.loop:
                mov             rax, [rdi]
                div             rbx
                mov             [rdi], rax
                sub             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rax
                pop             rdi
                ret

; assigns a zero to long number
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
set_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep stosq

                pop             rcx
                pop             rdi
                pop             rax
                ret

; checks if a long number is a zero
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
; result:
;    ZF=1 if zero
is_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep scasq

                pop             rcx
                pop             rdi
                pop             rax
                ret

; read long number from stdin
;    rdi -- location for output (long number)
;    rcx -- length of long number in qwords
read_long:
                push            rcx
                push            rdi

                call            set_zero
.loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              .done
                cmp             rax, '0'
                jb              .invalid_char
                cmp             rax, '9'
                ja              .invalid_char

                sub             rax, '0'
                mov             rbx, 10
                call            mul_long_short
                call            add_long_short
                jmp             .loop

.done:
                pop             rdi
                pop             rcx
                ret

.invalid_char:
                mov             rsi, invalid_char_msg
                mov             rdx, invalid_char_msg_size
                call            print_string
                call            write_char
                mov             al, 0x0a
                call            write_char

.skip_loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              exit
                jmp             .skip_loop

; write long number to stdout
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
write_long:
                push            rax
                push            rcx

                mov             rax, 20
                mul             rcx
                mov             rbp, rsp
                sub             rsp, rax

                mov             rsi, rbp

.loop:
                mov             rbx, 10
                call            div_long_short
                add             rdx, '0'
                dec             rsi
                mov             [rsi], dl
                call            is_zero
                jnz             .loop

                mov             rdx, rbp
                sub             rdx, rsi
                call            print_string

                mov             rsp, rbp
                pop             rcx
                pop             rax
                ret

; read one char from stdin
; result:
;    rax == -1 if error occurs
;    rax \in [0; 255] if OK
read_char:
                push            rcx
                push            rdi

                sub             rsp, 1
                xor             rax, rax
                xor             rdi, rdi
                mov             rsi, rsp
                mov             rdx, 1
                syscall

                cmp             rax, 1
                jne             .error
                xor             rax, rax
                mov             al, [rsp]
                add             rsp, 1

                pop             rdi
                pop             rcx
                ret
.error:
                mov             rax, -1
                add             rsp, 1
                pop             rdi
                pop             rcx
                ret

; write one char to stdout, errors are ignored
;    al -- char
write_char:
                sub             rsp, 1
                mov             [rsp], al

                mov             rax, 1
                mov             rdi, 1
                mov             rsi, rsp
                mov             rdx, 1
                syscall
                add             rsp, 1
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

; print string to stdout
;    rsi -- string
;    rdx -- size
print_string:
                push            rax

                mov             rax, 1
                mov             rdi, 1
                syscall

                pop             rax
                ret


                section         .rodata
invalid_char_msg:
                db              "Invalid character: "
invalid_char_msg_size: equ             $ - invalid_char_msg
