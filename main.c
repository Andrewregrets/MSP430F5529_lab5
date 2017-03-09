#include <msp430f5529.h>

int mode = 1;
const int START_POSITION = 90;	//positions for right corner
const int END_POSITION = 100;

unsigned char number[11][10] = { {0x3f,0x3f,0x33,0x33,0x33,0x33,0x33,0x33,0x3f,0x3f}, // 0o
								 {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}, // 1
								 {0x3f,0x3f,0x30,0x30,0x3f,0x3f,0x03,0x03,0x3f,0x3f}, // 2
								 {0x3f,0x3f,0x03,0x03,0x3f,0x3f,0x03,0x03,0x3f,0x3f}, // 3
								 {0x03,0x03,0x03,0x03,0x3f,0x3f,0x33,0x33,0x33,0x33}, // 4
								 {0x3f,0x3f,0x03,0x03,0x3f,0x3f,0x30,0x30,0x3f,0x3f}, // 5
								 {0x3f,0x3f,0x33,0x33,0x3f,0x3f,0x30,0x30,0x3f,0x3f}, // 6
								 {0x30,0x30,0x0c,0x0c,0x03,0x03,0x03,0x03,0x3f,0x3f}, // 7
								 {0x3f,0x3f,0x33,0x33,0x3f,0x3f,0x33,0x33,0x3f,0x3f}, // 8
								 {0x3f,0x3f,0x03,0x03,0x3f,0x3f,0x33,0x33,0x3f,0x3f}, // 9
								 {0x00,0x00,0x00,0x00,0x3f,0x3f,0x00,0x00,0x00,0x00}};// -


void DOGS102_SPI(unsigned char byte)
{
	// Ожидаем готовности TXBUF к приему данных.
	while (!(UCB1IFG & UCTXIFG));
	// CS=0, Начало SPI операции.
	P7OUT		&= ~BIT4;
	// Начало передачи.
	UCB1TXBUF 	= byte;
	// Ожидаем пока интерфейс занят.
	while (UCB1STAT & UCBUSY);
	// CS=1
	P7OUT		|= BIT4;
}

char cma3000_SPI(unsigned char byte1, unsigned char byte2)
{
    char indata;
    //P3.5( CMA3000 PIN CSB ) SET "0" IS START SPI OPERATION
    P3OUT &= ~BIT5;
    indata = UCA0RXBUF;
    //WAIT TXIFG == TXBUF IS READY FOR NEW DATA
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = byte1; //START SPI TRANSMIT. SEND FIRST BYTE
    //WAIT RXIFG == RXBUF HAVE NEW DATA
    while(!(UCA0IFG & UCRXIFG));
    indata = UCA0RXBUF;
    //WAIT TXIFG == TXBUF IS READY FOR NEW DATA
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = byte2; //START SPI TRANSMIT. SEND SECOND BYTE
    //WAIT RXIFG == RXBUF HAVE NEW DATA
    while(!(UCA0IFG & UCRXIFG));
    indata = UCA0RXBUF; //READ SPI DATA FROM ACCEL. IN 2 BYTE IN READ COMMAND
    //WAIT UNTIL USCI_A0 SPI INTERFACE IS NO LONGER BUSY
    while(UCA0STAT & UCBUSY);
    P3OUT |= BIT5; //P3.5(CMA3000 PIN CSB) SET "1" IS STOP SPI OPERATION
    return indata;
}

void SetPos(char column, char page)
{
	P5OUT	&= ~BIT6;
	char 	low 	= column & 0xF;
	char	high	= column >> 4;
	DOGS102_SPI(low);
	DOGS102_SPI(0x10 | high);

	page	&= 0xF;
	DOGS102_SPI(0xB0 | page);
}

void SetData(char data)
{
    P5OUT	|= BIT6;
    DOGS102_SPI(data);
}

void SetCmd(char cmd)
{
	P5OUT	&= ~BIT6;
	DOGS102_SPI(cmd);
}

int reverseByte(int value)
{
	volatile int result = 0, i = 0;
	for(; i < 8; i++)
	{
		result <<= 1;
		result += value % 2;
		value >>= 1;
	}
	return result;
}

void PutSymbol(int symbol, int position)
{
	char page;
	page = mode? 4 + position : 7 - position;
	int i;
	for (i = START_POSITION; i < END_POSITION; ++i)
	{
		SetPos(i, page);
		int data = number[symbol][END_POSITION-1 - i];
		if (mode)
			data = reverseByte(data);
		SetData(data);
	}
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    TA0CCR0 = 0x2000;
    TA0CTL = TASSEL__SMCLK + MC__UP + TACLR + ID__1;

    TA1CCR0 = 0xFFFF;
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL__SMCLK + MC__UP + TACLR + ID__4;

	P1DIR &= ~BIT7;
	P1REN |= BIT7;
	P1OUT |= BIT7;
	P1IE |= BIT7;
	P1IES |= BIT7;
	P1IFG &= ~BIT7;

	// Сигнал прерывания
	P2DIR  &= ~BIT5;            //P2.5(CMA3000 PIN INT) INPUT
	P2OUT  |=  BIT5;            //P2.5(CMA3000 PIN INT) PULL-UP RESISTOR
	P2REN  |=  BIT5;            //P2.5(CMA3000 PIN INT) ENABLE RESISTOR
	P2IE   |=  BIT5;            //P2.5(CMA3000 PIN INT) INTERRUPT ENABLE
	P2IES  &= ~BIT5;            //P2.5(CMA3000 PIN INT) EDGE FOR INTERRUPT : LOW-TO-HIGH
	P2IFG  &= ~BIT5;            //P2.5(CMA3000 PIN INT) CLEAR INT FLAG
	// Выбор устройства
	P3DIR  |=  BIT5;            //P3.5(CMA3000 PIN CSB) SET AS OUTPUT
	P3OUT  |=  BIT5;            //P3.5(CMA3000 PIN CSB) SET "1" IS DISABLE CMA3000
	// Синхросигнал
	P2DIR  |=  BIT7;            //P3.5(CMA3000 PIN SCK) SET AS OUTPUT
	P2SEL  |=  BIT7;            //DEVICE MODE : P2.7 IS UCA0CLK
	// Линия передачи по SPI и Линия приема по SPI
	P3DIR  |= (BIT3 | BIT6);    //P3.5 & P3.6(CMA3000 PIN MOSI, PWR) SET AS OUTPUT
	P3DIR  &= ~BIT4;            //P3.4(CMA3000 PIN MISO) SET AS INPUT
	P3SEL  |= (BIT3 | BIT4);    //DEVICE MODE : P3.3 - UCA0SIMO, P3.4 - UCA0SOMI
	P3OUT  |= BIT6;             //P3.6(CMA3000 PIN PWR) SET "1" IS POWER CMA3000

    // CD и RST устанавливаем на выход.
    P5DIR	|= BIT6 | BIT7;
    // CS и ENA устанавливаем на выход.
    P7DIR	|= BIT4 | BIT6;
    // CS & ENA no select on bkLED.
    P7OUT	|= BIT4 | BIT6;
    // SCK и SDA устанавливаем на выход.
    P4DIR	|= BIT1 | BIT3;
    // Режим устройста для SCK и SDA = UCB1SIMO & UCB1CLK
    P4SEL	|= BIT1 | BIT3;

    // Сброс при нуле.
    P5OUT	&= ~BIT7;
    __delay_cycles(25000);
    // Снимаем флаг сброса.
    P5OUT	|= BIT7;
    __delay_cycles(125000);

    // для акселерометра
    UCA0CTL1 |= UCSWRST;
    // sync Master  MSB
    UCA0CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH;
    UCA0CTL1 |= UCSWRST | UCSSEL__SMCLK; // выбор источника ТИ
    UCA0BR0 = 0x30; // младший байт делителя частоты
    UCA0BR1 = 0;
    UCA0MCTL = 0;
    UCA0CTL1 &= ~UCSWRST;


    // для ЖКИ
    // Сброс логики интерфейса.
    UCB1CTL1 |= UCSWRST;
    // Синхронный режим, тактирование генератором USCI.
    UCB1CTL0 |= UCSYNC | UCMST | UCMSB | UCCKPH;
    UCB1CTL1 |= UCSWRST | UCSSEL__SMCLK;
    // Деление частоты, младшая и страшая части.
    UCB1BR0	= 0x30;
    UCB1BR1	= 0;
    // Снимаем бит сброса.
    UCB1CTL1 &= ~UCSWRST;

    P5OUT	&= ~BIT6; // Режим: команда
    DOGS102_SPI(0x2F); //Power On
    DOGS102_SPI(0xAF); //Display On
    DOGS102_SPI(0xA6);
    SetCmd(0xA1);
    int i, j;
    for (i = 30; i < 132; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
        	SetPos(i, j);
        	SetData(0);
        }
    }

    cma3000_SPI(0x4, 0);
    __delay_cycles(1250);
    cma3000_SPI(0xA, BIT7 | BIT4 | BIT2);
    __delay_cycles(25000);
    __bis_SR_register(GIE);
    __no_operation();
	return 0;
}

#pragma vector= TIMER0_A0_VECTOR
__interrupt void TIMER0_AO_ISR(void)
{
	if(!(P1IN & BIT7))
	{
		int i, j;
		for (i = START_POSITION; i < END_POSITION; ++i)
		{
			for (j = 4; j < 8; ++j)
			{
				 SetPos(i, j);
				 SetData(0);
			}
		}
		mode = !mode;
	}
	P1IE |= BIT7;
	TA0CCTL0 &= ~CCIE;
	TA0CCTL0 &= ~CCIFG;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
	P1IE &= ~BIT7;
    TA0CCTL0 = CCIE;
    TA0CTL |= TACLR;
    P1IFG &= ~BIT7;
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_AO_ISR(void)
{
	int symbol, i;
	signed char value;
	value = cma3000_SPI(0x18, 0); //0x18 - X, 0x1C - Y, 0x20 - Z
	if(value < 0)
	{
		PutSymbol(10,0);
		value *= -1;
	}

	for ( i = 3; i > 0; i--)
	{
		symbol = value % 10;
		value /= 10;
		PutSymbol(symbol,i);
	}
	P2IFG &= ~BIT5;
}
