#define ARM_MATH_CM4
#include <arm_math.h>

//Create objects
IntervalTimer sampler;
arm_cfft_radix4_instance_f32 fft_inst;

//Create global variables
const int FFT_SIZE = 256;
const int AUDIO_INPUT_PIN = 14;
const int RADIANS_TO_DEGREES = 57;
const int COMPRESS_FACTOR = 255;                //map values >255 into the 255 range, but will lose +- 255 in resolution
int SAMPLE_RATE_HZ = 20000;
float MicSamples[FFT_SIZE*2];                   //ping pong buffers -> fft are calculated in place
float MicSamples2[FFT_SIZE*2];                  //ping pong buffers -> fft are calculated in place
float* ActiveBuffer;                            //swaps off with the back buffer pointer
float* BackBuffer = MicSamples;                 //start with the back buffer sampling
int bufferSwitch = 0;                           //0 corresponds to MicSamples to fft, 1 corresponds to MicSamples2 to fft
int sampleCount;                                //corresponds to MicSamples sample count
float magnitudes[FFT_SIZE];
float phases[FFT_SIZE];
int greatestAmplitude = 0;
float greatestMagnitude = 0;
int greatestIndex = 0;
int greatestFrequency = 0;


void samplerCallback() {

  if(sampleCount < FFT_SIZE*2)
  {
    // Read from the ADC and store the sample data
    BackBuffer[sampleCount] = (float32_t)analogRead(AUDIO_INPUT_PIN);
    if(BackBuffer[sampleCount] > greatestAmplitude)
    {
      greatestAmplitude = BackBuffer[sampleCount];
    }
    // Complex FFT functions require a coefficient for the imaginary part of the input.
    // Since we only have real data, set this coefficient to zero.
    BackBuffer[sampleCount+1] = 0.0;
    // Update sample buffer position and stop after the buffer is filled
    sampleCount += 2;
  }
  
}

void samplingBegin() {
  // Reset sample buffer position and start callback at necessary rate.
  sampleCount = 0;
  sampler.begin(samplerCallback, 1000000/SAMPLE_RATE_HZ);
}

boolean samplingIsFull() {

  //swap MicSamples(2) arrays that Active and Back point to
  //don't sample any data past FFT_SIZE until after activebuffer is fft'd, to get the exact data main loop doesn't sample
  if(sampleCount >= FFT_SIZE*2)
  {
    //sampler stopped so no artifacts enter arrays during this function
    sampler.end();
    if(bufferSwitch == 0)
    {
      //give ActiveBuffer the finished MicSamples buffer to fft
      ActiveBuffer = MicSamples;
      //give BackBuffer the MicSamples2 buffer to fill
      BackBuffer = MicSamples2;
      bufferSwitch = 1;
      samplingBegin();
    }else
    {
      //give ActiveBuffer the finished MicSamples2 buffer to fft
      ActiveBuffer = MicSamples2;
      //give BackBuffer the MicSamples2 buffer to fill
      BackBuffer = MicSamples;
      bufferSwitch = 0;
      samplingBegin();
    }
    return true;
  }
  
  return false;
}

void phasing() {
  for(int i = 0; i< FFT_SIZE * 2; i += 2)
  {
    //calculate phase in preparation for phase detection
    phases[i/2] = abs(atan2(magnitudes[i+1], magnitudes[i]) * RADIANS_TO_DEGREES);
  }
}


void setup() {
  pinMode(13, OUTPUT);
  //we've got power and running!
  digitalWrite(13, HIGH);
  pinMode(AUDIO_INPUT_PIN, INPUT);
  Serial3.begin(115200);
  Serial.begin(115200);
  arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
  sampler.begin(samplerCallback, 1000000/SAMPLE_RATE_HZ);
}

void loop() {
  /*
   * we need to track and send a couple pieces of data
   * track : frequency_fft, magnitude_fft
   * send : magnitude_fft, phase_fft, magnitude_adc  
   * how to send a multi-byte data over easily and without trying to separate bytes ?
   * scale down the 1 > byte data into 0-255 range !
   */
  if (samplingIsFull()) {
    // Run FFT on sample data.
    arm_cfft_radix4_f32(&fft_inst, ActiveBuffer);
    // Calculate phase of complex numbers output by the FFT.
    phasing();
    // Calculate magnitude of complex numbers output by the FFT.
    arm_cmplx_mag_f32(ActiveBuffer, magnitudes, FFT_SIZE);
    // Output magnitudes in debug mode.
    for (int i = 1; i < FFT_SIZE/2; ++i) 
    {
      //looking for most prominent frequency
      //save array index to calculate frequency
      if(magnitudes[i] > greatestMagnitude)
      {
        greatestMagnitude = magnitudes[i];
        greatestIndex = i;
      }
    }
    
    greatestFrequency = greatestIndex*SAMPLE_RATE_HZ/FFT_SIZE;
    
    //frequency range of 800Hz to 4000Hz to reject airplane drone + human speech 
    //5000 is a tested value for what seemed to be a reasonable strong fft value to determine presence of frequency
    if(greatestFrequency > 1500 && greatestFrequency < 4000 && greatestMagnitude > 5000)
    {
      //debugging lines
      /*
      Serial.print(greatestFrequency);
      Serial.print(" ");
      Serial.print(magnitudes[greatestIndex]);
      Serial.print(" ");
      Serial.print(phases[greatestIndex]);
      Serial.print(" ");
      Serial.println(greatestAmplitude);
      Serial.print(static_cast<uint8_t>(greatestFrequency / COMPRESS_FACTOR));
      Serial.print(" ");
      Serial.print(static_cast<uint8_t>(magnitudes[greatestIndex] / COMPRESS_FACTOR));
      Serial.print(" ");
      if(phases[greatestIndex] < 0)
      {
        Serial.print(static_cast<uint8_t>((-1)*phases[greatestIndex]));
      }else
      {
        Serial.print(static_cast<uint8_t>(phases[greatestIndex]));
      }
      Serial.print(" ");
      Serial.println(static_cast<uint8_t>(greatestAmplitude / COMPRESS_FACTOR));
      */
      
      Serial3.write(static_cast<uint8_t>(greatestFrequency / COMPRESS_FACTOR));
      Serial3.write(static_cast<uint8_t>(magnitudes[greatestIndex] / COMPRESS_FACTOR));
      Serial3.write(static_cast<uint8_t>(phases[greatestIndex]));
      Serial3.write(static_cast<uint8_t>(greatestAmplitude / COMPRESS_FACTOR));
      
      //wait out the gunshot for 20 seconds to avoid reflections of the waveform
      digitalWrite(13, LOW);
      delay(20000);
      digitalWrite(13, HIGH);                                                         //turn on when ready to fft again
      sampler.end();
      //clear the backBuffer to have a clean start after detecting gunshot
      for(int i = 0; i < FFT_SIZE*2 ; i++)
      {
        BackBuffer[i] = 0;
      }
      samplingBegin();
    }

    //reset tracked and sent values for new start
    greatestAmplitude = 0;
    greatestIndex = 0;
    greatestFrequency = 0;
    greatestMagnitude = 0;
    // Restart audio sampling.
    //samplingBegin();
  }
}
