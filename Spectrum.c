#include <stdint.h>
#include <math.h>
#include "BSP.h"
#include "Profile.h"
#include "Texas.h"
#include "CortexM.h"
#include "fix_fft.h"

// Global Variables
//color constants
#define BGCOLOR     LCD_BLACK
#define AXISCOLOR   LCD_ORANGE
#define MAGCOLOR    LCD_YELLOW
#define EWMACOLOR   LCD_CYAN
#define SOUNDCOLOR  LCD_CYAN
#define LIGHTCOLOR  LCD_LIGHTGREEN
#define TOPTXTCOLOR LCD_WHITE
#define TOPNUMCOLOR LCD_ORANGE


#define fftLength 512
#define logfft 9
uint16_t SoundData;					// raw data sampled from the microphone
short real[fftLength]; 			// geymir sample fyrir fft og svo niðurstöðu
short imag[fftLength];
short V[10];						// FFT Bins
int ReDrawAxes = 1;					// endurteiknar Axes ef != 0
int32_t sampleTime, FFTtime;		// Stopwatch
int kfft; // skoða þetta

// Task 4 teiknar eftir hvað plotstate er
enum plotstate{
	FFTplot,
	Microphone,
	Bins
};
enum plotstate PlotState = FFTplot;

//---------------- Task1 Bins ----------------
// Calculate FFT bins
void Task0(void){
	V[0] = (real[1]-420)/10;
	V[1] = (real[2]-420)/10;
	V[2] = (real[3]-420)/10;
	V[3] = (real[4]-420)/10;
	V[4] = ((real[5]+real[6])/2-420)/10;
	V[5] = ((real[7]+real[8]+real[9])/3-420)/10;
	V[6] = 1.5*((real[10]+real[11]+real[12])/3-420)/10;
	V[7] = 2*((real[13]+real[14]+real[15]+real[16]+real[17])/5-420)/10;
	
	V[8] = 0;
	for(int i = 18; i < 28; i++){
		V[8] = V[8]+real[i];
	}
	V[8] = 2*(V[8]/10-420)/10;
	
	V[9] = 0;
	for(int i =28; i < 51; i++){
		V[9] = V[9]+real[i];
	}
	V[9] = 4*(V[9]/23-420)/10;
}


//---------------- Task1 FFT  ----------------
// Sample fftlength microphone samples
// Breytir real[] í FFT útreikninga,
// skrifar sample tíma í sampleTime
// og FFT tíma í FFTtime
void Task1(void){
	uint32_t t0, t1;
	
	//microphone sample
	t0 = BSP_Time_Get();
	for(kfft = 0; kfft < fftLength; kfft++){
		BSP_Microphone_Input(&SoundData);
		real[kfft] = ((int)SoundData)-512;
	}
	t1 = BSP_Time_Get();
	sampleTime = t1-t0;
	
	// FFT útreikningar
	t0 = BSP_Time_Get();
	short p2 = fix_fft(real, imag, logfft, 0);
	for ( int i = 0; i < 51; i++){
		real[i] = sqrt((real[i] * real[i] + imag[i] * imag[i]))*4+420;  //sleppi sqrt
	}
	for(int j = 0; j<fftLength; j++){
		imag[j] = 0;
	}
	t1 = BSP_Time_Get();
	FFTtime = t1-t0;
}

//---------------- Task3 Takkar ---------------
// Fylgist með tökkum og uppfærir mode
void Task3(void){
	static uint8_t prev1 = 0, prev2 = 0;
	uint8_t current;
	current = BSP_Button1_Input();
	if((current == 0) && (prev1 != 0)){
		// Button1 was pressed since last loop
		if(PlotState == FFTplot){
			BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
			PlotState = Bins;
		}else if(PlotState == Bins){
			BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
			PlotState = Microphone;
			ReDrawAxes = 1; 
		}else if(PlotState == Microphone){
			  PlotState = FFTplot;
				ReDrawAxes = 1; 
		}
	}
	prev1 = current;
	// button 2 gerir það sama og 1
	// geymi þetta fyrir future feature
	current = BSP_Button2_Input();
	if((current == 0) && (prev2 != 0)){
		// Button2 was pressed since last loop
		if(PlotState == FFTplot){
			BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
			PlotState = Bins;
		}else if(PlotState == Bins){
			BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
			PlotState = Microphone;
			ReDrawAxes = 1; 
		}else if(PlotState == Microphone){
			  PlotState = FFTplot;
				ReDrawAxes = 1; 
		}
	}
	prev2 = current;
}


//---------------- Task4 LCD ---------------
// Plottar  upplýsingar á skjáinn m.v PlotState
void Task4(void){
	if(ReDrawAxes){
		ReDrawAxes = 0;
		if(PlotState == FFTplot){
			BSP_LCD_Drawaxes(AXISCOLOR, BGCOLOR, "freq", "Mag", MAGCOLOR, "", 0, SoundData+100, SoundData-100);
		} else if(PlotState == Microphone){
			BSP_LCD_Drawaxes(AXISCOLOR, BGCOLOR, "Time", "Sound", SOUNDCOLOR, "", 0, SoundData+100, SoundData-100);
		}
	}
	if(PlotState == FFTplot){
		for ( int i = 1; i < 51; i++){
			BSP_LCD_PlotPoint(real[i], SOUNDCOLOR);
			BSP_LCD_PlotIncrement();
			BSP_LCD_PlotPoint(real[i], SOUNDCOLOR);
			BSP_LCD_PlotIncrement();
		}
 		BSP_LCD_DrawString(2, 0,  "Sampling:",  TOPTXTCOLOR);
		BSP_LCD_SetCursor(12, 0); BSP_LCD_OutUDec(sampleTime, LIGHTCOLOR);
		BSP_LCD_DrawString(2, 1,  "FFT=",  TOPTXTCOLOR);
		BSP_LCD_SetCursor(12, 1); BSP_LCD_OutUDec(FFTtime, LIGHTCOLOR);
	} else if(PlotState == Microphone){
		BSP_Microphone_Input(&SoundData);
		BSP_LCD_PlotPoint(SoundData, SOUNDCOLOR);
		BSP_LCD_PlotIncrement();
	} else if(PlotState == Bins){
		for(int i=0; i<=9; i++){ 
			for(int j=0; j<=9; j++){
				if(j <= V[i]){
					BSP_LCD_FillRect(5 + i*12,113 - 12*j,10,10,LCD_GREEN);
				}else{
					BSP_LCD_FillRect(5 + i*12,113 - 12*j,10,10,LCD_BLACK);
				}
			}
		}
	}
}


int main(void){
	DisableInterrupts();
	BSP_Clock_InitFastest();	// BSP_Clock_GetFreq(); gefur klukkuhraða
	Profile_Init();						// initialize the 7 hardware profiling pins

	// Virkar ekki án, ekki hugmynd hvað þetta gerir
	TExaS_Init(GRADER, 1000 ); // initialize the Lab 1 GRADER/LOGICANALYZER
	
	BSP_Button1_Init();
	BSP_Button2_Init();
	BSP_Microphone_Init();
	BSP_LCD_Init();
	BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
	EnableInterrupts(); // interrupts needed for grader to run

	while(1){
		if(PlotState == FFTplot){
			Task1(); // samp mic and calc fft
		}
		if(PlotState == Bins){
			Task1(); // samp mic and calc fft
			Task0(); // Calculate Bins
		}
		Task3();  // check the buttons and change mode if pressed
		Task4();  // update the plot
  }
}

