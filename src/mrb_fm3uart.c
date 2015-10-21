/*
** mrb_fm3uart.c - FM3 UART
**
**
*/

#include "uart.h"

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/value.h"
#include "mruby/string.h"
#include "mruby/array.h"

#define MFS0 0
#define MFS1 1
#define MFS2 2
#define MFS3 3
#define MFS4 4
#define MFS5 5
#define MFS6 6
#define MFS7 7
#define PIN_LOC0 0
#define PIN_LOC1 1
#define PIN_LOC2 2

void mrb_fm3uart_free(mrb_state *mrb, void *ptr);

mrb_value mrb_FM3_uartInitialize(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_uartWrite(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_uartReceived(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_uartRead(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_uartFlush(mrb_state *mrb, mrb_value self);
mrb_value mrb_FM3_uartClose(mrb_state *mrb, mrb_value self);
void mrb_mruby_fm3uart_gem_init(mrb_state *mrb);
void mrb_mruby_fm3uart_gem_final(mrb_state *mrb);

static const struct mrb_data_type mrb_fm3uart_type = {"UART", mrb_fm3uart_free};

/* instance free */
void mrb_fm3uart_free(mrb_state *mrb, void *ptr)
{
	volatile UARTFIFO *uart = ptr;
	
	uart_detach_buffer(uart);
	
	mrb_free(mrb, uart);
}

/* UART.initialize(mfs, loc, bps) */
mrb_value mrb_FM3_uartInitialize(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	mrb_int mfs;
	mrb_int loc;
	mrb_int bps;
	
	uart = (UARTFIFO *)mrb_malloc(mrb, sizeof(UARTFIFO));
	DATA_TYPE(self) = &mrb_fm3uart_type;
	DATA_PTR(self) = uart;
	
	mrb_get_args(mrb, "iii", &mfs, &loc, &bps);
	uart->mfsch = (uint8_t)mfs;
	uart->locno = (uint8_t)loc;
	
	uart_init(uart, (uint32_t)bps);
	
	return self;
}

/* UART.write(buffer) */
mrb_value mrb_FM3_uartWrite(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	mrb_value obj;
	mrb_value *array;
	char *string;
	int arrsize, strsize;
	int i, j;
	
	mrb_get_args(mrb, "o", &obj);
	uart = DATA_PTR(self);
	
	switch(mrb_type(obj))
	{
		case MRB_TT_STRING:
			strsize = RSTRING_LEN(obj);
			string = RSTRING_PTR(obj);
			for(i=0; i<strsize; i++)
			{
				uart_putc(uart, (uint8_t)string[i]);
			}
			break;
		case MRB_TT_FIXNUM:
			uart_putc(uart, (uint8_t)mrb_fixnum(obj));
			break;
		case MRB_TT_ARRAY:
			arrsize = RARRAY_LEN(obj);
			array = RARRAY_PTR(obj);
			for(i=0; i<arrsize; i++)
			{
				switch(mrb_type(array[i]))
				{
					case MRB_TT_STRING:
						strsize = RSTRING_LEN(array[i]);
						string = RSTRING_PTR(array[i]);
						for(j=0; j<strsize; j++)
						{
							uart_putc(uart, (uint8_t)string[j]);
						}
						string = NULL;
						break;
					case MRB_TT_FIXNUM:
						uart_putc(uart, (uint8_t)mrb_fixnum(array[i]));
						break;
					default:
						break;
				}
			}
			break;
		default:
			break;
	}
	
	return mrb_nil_value();
}

/* UART.received? */
mrb_value mrb_FM3_uartReceived(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	uint8_t num;
	
	uart = DATA_PTR(self);
	
	num = uart_received(uart);
	
	if(num)		/* data received */
		return mrb_true_value();
	else
		return mrb_false_value();
}

/* UART.read */
mrb_value mrb_FM3_uartRead(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	mrb_int  dat;
	
	uart = DATA_PTR(self);
	
	dat = (mrb_int)uart_getc(uart);
	
	return mrb_fixnum_value(dat);
}

/* UART.flush */
mrb_value mrb_FM3_uartFlush(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	
	uart = DATA_PTR(self);
	
	uart_flush(uart);
	
	return mrb_nil_value();
}

/* UART.close */
mrb_value mrb_FM3_uartClose(mrb_state *mrb, mrb_value self)
{
	volatile UARTFIFO *uart;
	
	uart = DATA_PTR(self);
	
	uart_close(uart);
	
	return mrb_nil_value();
}

void mrb_mruby_fm3uart_gem_init(mrb_state *mrb)
{
	struct RClass *uart;
	
	/* define class */
	uart = mrb_define_class(mrb, "UART", mrb->object_class);
	MRB_SET_INSTANCE_TT(uart, MRB_TT_DATA);
	
	/* define method */
	mrb_define_method(mrb, uart, "initialize", mrb_FM3_uartInitialize, MRB_ARGS_REQ(3));
	mrb_define_method(mrb, uart, "write", mrb_FM3_uartWrite, MRB_ARGS_REQ(1));
	mrb_define_method(mrb, uart, "received?", mrb_FM3_uartReceived, MRB_ARGS_NONE());
	mrb_define_method(mrb, uart, "read", mrb_FM3_uartRead, MRB_ARGS_NONE());
	mrb_define_method(mrb, uart, "flush", mrb_FM3_uartFlush, MRB_ARGS_NONE());
	mrb_define_method(mrb, uart, "close", mrb_FM3_uartClose, MRB_ARGS_NONE());

	/* define constracts */
	mrb_define_const(mrb, uart, "MFS0", mrb_fixnum_value(MFS0));
	mrb_define_const(mrb, uart, "MFS1", mrb_fixnum_value(MFS1));
	mrb_define_const(mrb, uart, "MFS2", mrb_fixnum_value(MFS2));
	mrb_define_const(mrb, uart, "MFS3", mrb_fixnum_value(MFS3));
	mrb_define_const(mrb, uart, "MFS4", mrb_fixnum_value(MFS4));
	mrb_define_const(mrb, uart, "MFS5", mrb_fixnum_value(MFS5));
	mrb_define_const(mrb, uart, "MFS6", mrb_fixnum_value(MFS6));
	mrb_define_const(mrb, uart, "MFS7", mrb_fixnum_value(MFS7));
	mrb_define_const(mrb, uart, "PIN_LOC0", mrb_fixnum_value(PIN_LOC0));
	mrb_define_const(mrb, uart, "PIN_LOC1", mrb_fixnum_value(PIN_LOC1));
	mrb_define_const(mrb, uart, "PIN_LOC2", mrb_fixnum_value(PIN_LOC2));
}

void mrb_mruby_fm3uart_gem_final(mrb_state *mrb)
{
	
}