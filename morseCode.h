
#ifndef MORSECODE_H
#define MORSECODE_H


//dah is three times dit duration
//between characters is 3 dits duration
//words (ascii space) seperated by 7 dits duration
#define dit true
#define daaaaaaah false

typedef struct{
	uint8_t lenSymbs;
	bool symbs[5];
}MorseCharacter;

#define MORSECHARS_LEN 36
MorseCharacter MorseChars[MORSECHARS_LEN] = {
	{ .lenSymbs = 2, .symbs = {dit, daaaaaaah} },																					//A // 0
	{ .lenSymbs = 4, .symbs = {daaaaaaah, dit, dit, dit} },																//B // 1
	{ .lenSymbs = 4, .symbs = {daaaaaaah, dit, daaaaaaah, dit} },													//C // 2
	{ .lenSymbs = 3, .symbs = {daaaaaaah, dit, dit} },																		//D // 3
	{ .lenSymbs = 1, .symbs = {dit} },																										//E // 4
	{ .lenSymbs = 4, .symbs = {dit, dit, daaaaaaah, dit} },																//F // 5
	{ .lenSymbs = 3, .symbs = {daaaaaaah, daaaaaaah, dit} },															//G // 6
	{ .lenSymbs = 4, .symbs = {dit, dit, dit, dit} },																			//H // 7
	{ .lenSymbs = 2, .symbs = {dit, dit} },																								//I // 8
	{ .lenSymbs = 4, .symbs = {dit, daaaaaaah, daaaaaaah, daaaaaaah} },										//J // 9
	{ .lenSymbs = 3, .symbs = {daaaaaaah, dit, daaaaaaah} },															//K //10
	{ .lenSymbs = 4, .symbs = {dit, daaaaaaah, dit, dit} },																//L //11
	{ .lenSymbs = 2, .symbs = {daaaaaaah, daaaaaaah} },																		//M //12
	{ .lenSymbs = 2, .symbs = {daaaaaaah, dit} },																					//N //13
	{ .lenSymbs = 3, .symbs = {daaaaaaah, daaaaaaah, daaaaaaah} },												//O //14
	{ .lenSymbs = 4, .symbs = {dit, daaaaaaah, daaaaaaah, dit} },													//P //15
	{ .lenSymbs = 4, .symbs = {daaaaaaah, daaaaaaah, dit, daaaaaaah} },										//Q //16
	{ .lenSymbs = 3, .symbs = {dit, daaaaaaah, dit} },																		//R //17
	{ .lenSymbs = 3, .symbs = {dit, dit, dit} },																					//S //18
	{ .lenSymbs = 1, .symbs = {daaaaaaah} },																							//T //19
	{ .lenSymbs = 3, .symbs = {dit, dit, daaaaaaah} },																		//U //20
	{ .lenSymbs = 4, .symbs = {dit, dit, dit, daaaaaaah} },																//V //21
	{ .lenSymbs = 3, .symbs = {dit, daaaaaaah, daaaaaaah} },															//W //22
	{ .lenSymbs = 4, .symbs = {daaaaaaah, dit, dit, daaaaaaah} },													//X //23
	{ .lenSymbs = 4, .symbs = {daaaaaaah, dit, daaaaaaah, daaaaaaah} },										//Y //24
	{ .lenSymbs = 4, .symbs = {daaaaaaah, daaaaaaah, dit, dit} },													//Z	//25
	{ .lenSymbs = 5, .symbs = {daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah} }, 	//0 //26
	{ .lenSymbs = 5, .symbs = {dit, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah} },				//1 //27
	{ .lenSymbs = 5, .symbs = {dit, daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah} },	 			//2 //28
	{ .lenSymbs = 5, .symbs = {dit, dit, dit, daaaaaaah, daaaaaaah} }, 										//3 //29
	{ .lenSymbs = 5, .symbs = {dit, dit, dit, dit, daaaaaaah} },	 												//4 //30
	{ .lenSymbs = 5, .symbs = {dit, dit, dit, dit, dit} },		 														//5 //31
	{ .lenSymbs = 5, .symbs = {daaaaaaah, dit, dit, dit, dit} },	  											//6 //32
	{ .lenSymbs = 5, .symbs = {daaaaaaah, daaaaaaah, dit, dit, dit} },	  								//7 //33
	{ .lenSymbs = 5, .symbs = {daaaaaaah, daaaaaaah, daaaaaaah, dit, dit} },	  					//8 //34
	{ .lenSymbs = 5, .symbs = {daaaaaaah, daaaaaaah, daaaaaaah, daaaaaaah, dit} }		  		//9 //35
};


#define MORSE_OUT_BUFF_LEN 256
uint16_t morseOutBuffIdx = 0;
char morseOutputBuffer[MORSE_OUT_BUFF_LEN];

uint8_t asciiToMorseCharIdx(char c){

	if(c ==' '){
		return MORSECHARS_LEN-1;
	}
	else if(c >= 'a' && c <= 'z'){ //converts lower case to upper
		return c - 'a';
	}
	else if(c >= 'A' && c <= 'Z'){
		return c - 'A';
	}
	else if(c >= '0' && c <= '9' ){
		return c - '0' + ('Z'-'A')+1;
	}
	else{ //display as space
		return ('Z'-'A')+1+('9'-'0')+1; //36
	}

}
char morseIdxToAscii(uint8_t mIdx){
	if( mIdx >= 36 ){
		return ' ';
	}
	else if( mIdx < 25 ){ //letter
		return mIdx + 'a';
	}
	else{ //digit
		return (mIdx-26) + '0';
	}
}

void convertAsciiToMorseCharAndCopy( char * outBuff, const char * src, uint16_t num ){
	//Serial.print("catmcac "); Serial.println( num );
	for( uint16_t i = 0; i < num; ++i ){
		uint8_t val = asciiToMorseCharIdx(src[i]);
		outBuff[i] = val;
		//Serial.print( src[i] ); Serial.print(":"); Serial.print(val); Serial.print( " " );
	}
}

bool queueStringForMorseLedOutput(const char * str, uint8_t strLen){
	//Serial.print( "qSfmlo " ); Serial.println( strLen );
	bool retVal = true;
	uint16_t writeEndIdx = morseOutBuffIdx+strLen;
	uint16_t firstWriteEndIdx = min( writeEndIdx, (uint16_t)(MORSE_OUT_BUFF_LEN -1) );
	//Serial.print( writeEndIdx ); Serial.print( " " ); Serial.println( firstWriteEndIdx );
	convertAsciiToMorseCharAndCopy( &morseOutputBuffer[morseOutBuffIdx], str, firstWriteEndIdx-morseOutBuffIdx );
	if( writeEndIdx > MORSE_OUT_BUFF_LEN-1 ){ //wrap around remaining str
		//Serial.print( " wei > mobl" );
		uint16_t remWriteLen = writeEndIdx - (MORSE_OUT_BUFF_LEN-1);
		if( remWriteLen > morseOutBuffIdx-1 ){ //prevent overwriting next character to output
			remWriteLen = morseOutBuffIdx-1;
			retVal = false;
		}

		convertAsciiToMorseCharAndCopy( &morseOutputBuffer[0], &str[strLen-remWriteLen], remWriteLen );
	}
	//Serial.println("\nqSfmlo end");
	return retVal;
}

//called from timer or main loop
//once per dit/dot time period when a transition
//between the start and end of a MorseSignal may occur
bool nextSignal = false;
uint8_t subSignalIdx = 0;
uint8_t outputSignalIdx = 0;
bool outputState = false;
void morseOutputLedUpdate( uint8_t mOutputPin, bool invertOutput=false ){
	char c = morseOutputBuffer[morseOutBuffIdx];

	uint8_t mCharLen = 1;
	bool sigType;

	if(c == 36){ //space
		mCharLen = 5;
		outputSignalIdx += 1;
		outputState = false;
	}
	else{ //lookup letter or digit
		MorseCharacter mChar = MorseChars[c];

		sigType = mChar.symbs[outputSignalIdx];
		mCharLen = mChar.lenSymbs;

		//if next signal and subSignalIdx == 0
		//output 3 dits duration character space
		if( outputSignalIdx < mCharLen ){
			if( sigType == dit ){
				if(subSignalIdx < 1)
					outputState = true;
				else
					outputState = false;
				if(subSignalIdx == 2){
					outputSignalIdx += 1;
					subSignalIdx = 0;
				}else{
					subSignalIdx += 1;
				}
			}else if( sigType == daaaaaaah ){
				if(subSignalIdx < 3)
					outputState = true;
				else
					outputState = false;
				if(subSignalIdx == 3){
					outputSignalIdx += 1;
					subSignalIdx = 0;
				}else{
					subSignalIdx += 1;
				}
			}
		}else{
			outputState = false;
			outputSignalIdx += 1;
		}
	}

	if( invertOutput )
		digitalWrite( mOutputPin, !outputState );
	else
		digitalWrite( mOutputPin, outputState );

	
	if( outputSignalIdx >= mCharLen+3 ){ //past the end of the character signals + (intra character space)
		subSignalIdx = 0;
		outputSignalIdx = 0;
		morseOutBuffIdx += 1;
		if( morseOutBuffIdx >= MORSE_OUT_BUFF_LEN )
			morseOutBuffIdx = 0;
		c = morseOutputBuffer[morseOutBuffIdx];
		Serial.print("m");Serial.print((uint8_t)c);Serial.print(":");Serial.println(morseIdxToAscii(c));
	}

/*
	Serial.print( "morUpd " );
	Serial.print( morseOutBuffIdx ); Serial.print( " " );
	Serial.print( (uint8_t)c ); Serial.print( " | " );
	Serial.print( mCharLen );Serial.print( " " );
	Serial.print( sigType ); Serial.print( " : " );
	Serial.print( subSignalIdx ); Serial.print( " " );
	Serial.print( outputSignalIdx ); Serial.print( " " );
	Serial.println( morseOutBuffIdx );
*/
}

#endif