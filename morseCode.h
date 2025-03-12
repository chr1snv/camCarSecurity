
#ifndef MORSECODE_H
#define MORSECODE_H


//dah is three times dit duration
//between characters is 3 dits duration
//words (ascii space) seperated by 7 dits duration
#define dit true
#define daaaaaaah false


const bool[][] charNumIdxToITUMorseSignals= {
  {dit, daaaaaaah},																				//A //0
  {daaaaaaah,dit, dit, dit},															//B //1
  {daaaaaaah, dit, daaaaaaah, dit},												//C //2
  {daaaaaaah, dit, dit},																	//D //3
  {dit},																									//E //4
  {dit, dit, daaaaaaah, dit},															//F //5
  {daaaaaaah, daaaaaaah, dit},														//G //6
  {dit, dit, dit, dit},																		//H //7
  {dit, dit},																							//I //8
  {dit, daaaaaaah, daaaaaaah, daaaaaaah},									//J //9
  {daaaaaaah, dit, daaaaaaah},														//K //10
  {dit, daaaaaaah, dit, dit},															//L //11
  {daaaaaaah, daaaaaaah},																	//M //12
  {daaaaaaah, dit},																				//N //13
  {daaaaaaah, daaaaaaah, daaaaaaah}												//O //14
  {dit, daaaaaaah, daaaaaaah, dit},												//P //15
  {daaaaaaah, daaaaaaah, dit, daaaaaaah}									//Q //16
  {dit, daaaaaaah, dit},																	//R //17
  {dit, dit, dit},																				//S //18
  {daaaaaaah},																						//T //19
  {dit, dit, daaaaaaah},																	//U //20
  {dit, dit, dit, daaaaaaah},															//V //21
  {dit, daaaaaaah, daaaaaaah},														//W //22
  {daaaaaaah, dit, dit, daaaaaaah}, 											//X //23
  {daaaaaaah, dit, daaaaaaah, daaaaaaah}, 								//Y //24
  {daaaaaaah, daaaaaaah, dit, dit},  											//Z	//25
	{daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah} //0 //26
  {dit, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah}, 			//1 //27
  {dit, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah}, 			//2 //28
  {dit, dit, dit, daaaaaaah, daaaaaaah}, 									//3 //29
  {dit, dit, dit, dit, daaaaaaah}, 												//4 //30
  {dit, dit, dit, dit, dit},  														//5 //31
  {daaaaaaah, dit, dit, dit, dit},  											//6 //32
  {daaaaaaah, daaaaaaah, dit, dit, dit},  								//7 //33
  {daaaaaaah, daaaaaaah, daaaaaaah, dit, dit},  					//8 //34
  {daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah, dit},  		//9 //35
};

#define MORSE_OUT_BUFF_LEN 256
uint16_t morseOutBuffIdx = 0;
char morseOutputBuffer[MORSE_OUT_BUFF_LEN];

uint8_t asciiToMorseCharIdx(char c){

	if(c ==' '){
		return len(charNumIdxToITUMorseSignals)-1;
	}
	else if(c >= 'a' && c < 'z'){ //converts lower case to upper
		return c - 'a';
	}
	else if(c >= 'A' && c < 'Z'){
		return c - 'A';
	}
	else if(c - '0'){
		return c - '0' + ('Z'-'A')+1;
	}
	else{ //display as space
		return ('Z'-'A')+1+('9'-'0')+1; //36
	}

}

bool queueStringForMorseLedOutput(char * str, uint8_t strLen){
	bool retVal = true;
	uint16_t writeEndIdx = morseOutBuffIdx+strLen;
	uint16_t firstWriteEndIdx = min( firstWriteEndIdx, MORSE_OUT_BUFF_LEN -1 );
	memcpy( &morseOutputBuffer[morseOutBuffIdx], str, firstWriteEndIdx-morseOutBuffIdx );
	if( writeEndIdx > MORSE_OUT_BUFF_LEN-1 ){ //wrap around remaining str
		uint16_t remWriteLen = writeEndIdx - (MORSE_OUT_BUFF_LEN-1);
		if( remWriteLen > morseOutBuffIdx-1 ){ //prevent overwriting next character to output
			remWriteLen = morseOutBuffIdx-1;
			retVal = false;
		}
		memcpy( &morseOutputBuffer[0], &str[strLen-remWriteLen], remWriteLen );
	}
	return retVal;
}

//called from timer or main loop
//once per dit/dot time period when a transition
//between the start and end of a MorseSignal may occur
bool nextSignal = false;
uint8_t subSignalIdx = 0;
uint8_t outputSignalIdx = 0;
void morseOutputLedUpdate( uint8_t mOutputPin ){
	char c = morseOutputBuffer[morseOutBuffIdx];

	bool outputState = false;
	uint8_t mCharLen = 1;

	if(c == 36){ //space
		mCharLen = 7;
		outputSignalIdx += 1;
	}
	else{ //lookup character
		MorseSignal * mChar = charNumIdxToITUMorseSignals[c];

		bool sigType = mChar[outputSignalIdx];

		//if next signal and subSignalIdx == 0
		//output 3 dits duration character space

		if( sigType == dit ){
			if(subSignalIdx < 1)
				outputState = true;
			else if(subSignalIdx == 2)
				nextSignal = true;
		}else if( sigType == daaaaaaah ){
			if(subSignalIdx < 3)
				outputState = true;
			else if(subSignalIdx == 2)
				nextSignal = true;
		}

		if(nextSignal)
			outputSignalIdx += 1;
	}
		
	digitalWrite( mOutputPin, outputState );

	//check if past the end of the character signals
	if( mCharLen < outputSignalIdx ){
		subSignalIdx = 0;
		outputSignalIdx = 0;
	}
}

#endif