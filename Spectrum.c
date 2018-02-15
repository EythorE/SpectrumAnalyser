#include <stdint.h>
#include <math.h>
#include "BSP.h"
#include "Profile.h"
#include "Texas.h"
#include "CortexM.h"
#include "FFT.h"

uint32_t sqrt32(uint32_t s);

//---------------- Global variables shared between tasks ----------------
uint16_t SoundData;         // raw data sampled from the microphone

int ReDrawAxes;
enum plotstate{
  FFTplot,
  Microphone,
	Bins
};
enum plotstate PlotState = FFTplot;

//color constants
#define BGCOLOR     LCD_BLACK
#define AXISCOLOR   LCD_ORANGE
#define MAGCOLOR    LCD_YELLOW
#define EWMACOLOR   LCD_CYAN
#define SOUNDCOLOR  LCD_CYAN
#define LIGHTCOLOR  LCD_LIGHTGREEN
#define TOPTXTCOLOR LCD_WHITE
#define TOPNUMCOLOR LCD_ORANGE
//------------ end of Global variables shared between tasks -------------

//---------------- Task0 samples sound from microphone ----------------
//#define SOUNDRMSLENGTH 10   // number of samples to collect before calculating RMS (may overflow if greater than 4104)
#define SOUNDRMSLENGTH 1000 // number of samples to collect before calculating RMS (may overflow if greater than 4104)
#define fftLength 512
#define logfft 9
int kfft;
int16_t SoundArray[SOUNDRMSLENGTH];
double real[fftLength];
double imag[fftLength];
double V[10] = {3,4,5,6,7,7,6,5,4,3};

// *********Task0_Init*********
// initializes microphone
// Task0 measures sound intensity
// Inputs:  none
// Outputs: none
void Task0_Init(void){
  BSP_Microphone_Init();
}

// *********Task0*********
// calculate fft bins
// Inputs:  none
// Outputs: none
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
/* ****************************************** */
/*          End of Task0 Section              */
/* ****************************************** */

//---------------- Task1 Sample FFT ----------------
uint32_t sampleTime, FFTtime;

// *********Task1*********
// Calculate fft in real[]
// Inputs:  none
// Outputs: none
void Task1(void){
  TExaS_Task1();     // record system time in array, toggle virtual logic analyzer
  Profile_Toggle1(); // viewed by the logic analyzer to know Task1 started
	
	uint32_t t0, t1;
	
	t0 = BSP_Time_Get();
	for(kfft = 0; kfft < fftLength; kfft++){
		BSP_Microphone_Input(&SoundData);
		real[kfft] = (double)SoundData;
	}
	t1 = BSP_Time_Get();
	sampleTime = t1-t0;

	t0 = BSP_Time_Get();
	FFT(1, logfft, real, imag);
	for ( int i = 0; i < fftLength; i++){
		real[i] =(sqrt((double)real[i] * (double)real[i] + (double)imag[i] * (double)imag[i]))*8+420;        
	}
	for(int j = 0; j<fftLength; j++){
		imag[j] = 0;
	}
	t1 = BSP_Time_Get();
	FFTtime = t1-t0;
}


/* ****************************************** */
/*          End of Task1 Section              */
/* ****************************************** */

//------------Task3 handles switch input -------
// *********Task3_Init*********
// initializes switches
// Task3 checks the switches, updates the mode
// Inputs:  none
// Outputs: none
void Task3_Init(void){
  BSP_Button1_Init();
  BSP_Button2_Init();
}
// *********Task3*********
// non-real-time task
// checks the switches, updates the mode
// Inputs:  none
// Outputs: none
void Task3(void){
  static uint8_t prev1 = 0, prev2 = 0;
  uint8_t current;
  TExaS_Task3();     // record system time in array, toggle virtual logic analyzer
  Profile_Toggle3(); // viewed by the logic analyzer to know Task3 started


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
/* ****************************************** */
/*          End of Task3 Section              */
/* ****************************************** */

//---------------- Task4 plots data on LCD ----------------
#define SOUND_MAX 900
#define SOUND_MIN 300
void drawaxes(void){
  if(PlotState == FFTplot){
		BSP_LCD_Drawaxes(AXISCOLOR, BGCOLOR, "freq", "Mag", MAGCOLOR, "", 0, SoundData+100, SoundData-100);
	} else if(PlotState == Microphone){
    BSP_LCD_Drawaxes(AXISCOLOR, BGCOLOR, "Time", "Sound", SOUNDCOLOR, "", 0, SoundData+100, SoundData-100);
  }
}
// return the number of digits
int numlength(uint32_t n){
  if(n < 10) return 1;
  if(n < 100) return 2;
  if(n < 1000) return 3;
  if(n < 10000) return 4;
  if(n < 100000) return 5;
  if(n < 1000000) return 6;
  if(n < 10000000) return 7;
  if(n < 100000000) return 8;
  if(n < 1000000000) return 9;
  return 10;
}

// *********Task4_Init*********
// initializes LCD
// Task4 updates the plot and Task5 updates the text at the top of the plot
// Inputs:  none
// Outputs: none
void Task4_Init(void){
  BSP_LCD_Init();
  BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
  drawaxes();
  ReDrawAxes = 1;
}
// *********Task4*********
// updates the plot
// Inputs:  none
// Outputs: none
void Task4(void){
	TExaS_Task4();     // record system time in array, toggle virtual logic analyzer
  Profile_Toggle4(); // viewed by the logic analyzer to know Task4 started

  if(ReDrawAxes){
    ReDrawAxes = 0;
    drawaxes();
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
  }else if(PlotState == Microphone){
		BSP_Microphone_Input(&SoundData);
    BSP_LCD_PlotPoint(SoundData, SOUNDCOLOR);
		BSP_LCD_PlotIncrement();
  }
	else if(PlotState == Bins){
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
/* ****************************************** */
/*          End of Task4 Section              */
/* ****************************************** */


int main(void){
  DisableInterrupts();
  BSP_Clock_InitFastest();
  Profile_Init();               // initialize the 7 hardware profiling pins
  // change 1000 to 4-digit number from edX 
  TExaS_Init(GRADER, 1000 );         // initialize the Lab 1 grader
//  TExaS_Init(LOGICANALYZER, 1000);  // initialize the Lab 1 logic analyzer
  Task0_Init();    // microphone init
	Task3_Init();    // buttons init
  Task4_Init();    // LCD graphics init

  EnableInterrupts(); // interrupts needed for grader to run
	//for(int i = 0; i<128; i++){
	//	Sinewave[i] = 0.0;
	//}
	//Sinewave[64] = 500.0;
	ReDrawAxes = 1;
	while(1){
		
		if(PlotState == FFTplot){
			Task1(); // samp mic and calc fft
		}
		if(PlotState == Bins){
			Task1(); // samp mic and calc fft
			Task0();
		}
		Task3();  // check the buttons and change mode if pressed
		Task4();  // update the plot
		
    Profile_Toggle6();
  }
}

// Newton's method
// s is an integer
// sqrt(s) is an integer
uint32_t sqrt32(uint32_t s){
uint32_t t;   // t*t will become s
int n;             // loop counter
  t = s/16+1;      // initial guess
  for(n = 16; n; --n){ // will finish
    t = ((t*t+s)/t)/2;
  }
  return t;
}




