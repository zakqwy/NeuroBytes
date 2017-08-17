/*
 * bit_ops.h
 *
 * Created: 7/23/2016 7:16:34 PM
 *  Author: Pat
 */ 


#ifndef BIT_OPS_H_
#define BIT_OPS_H_

#define BIT0_MASK	0x01
#define BIT1_MASK	0x02
#define BIT2_MASK	0x04
#define BIT3_MASK	0x08
#define BIT4_MASK	0x10
#define BIT5_MASK	0x20
#define BIT6_MASK	0x40
#define BIT7_MASK	0x80

// grabbed the following from here:
// http://www.avrfreaks.net/forum/tut-c-bit-manipulation-aka-programming-101?page=all

#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))


#endif /* BIT_OPS_H_ */