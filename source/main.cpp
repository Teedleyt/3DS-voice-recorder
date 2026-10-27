#include "3ds.h"
#include <stdio.h>
#include "citro2d.h"
#include "citro3d.h"
#include <malloc.h>

ndspWaveBuf waveBuf{}; //creates waveBuff structure for playing audio
//destination source size
void playAudio(u8* &rec, u8* &mic, u32 &size)
{
    memcpy(rec, mic, size); //Copies mic buffer into recorded buffer
    waveBuf.nsamples = size/2;
    DSP_FlushDataCache(rec, size); //Find out
    ndspChnWaveBufAdd(0, &waveBuf); //adds the waveBuf to the audio channel (plays audio)
}

int main()
{
    //init gfx  and console
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);
    printf("A: Start Recording\nB: Stop Recording\nX: Play Recording\n\nPress START to exit");
    consoleInit(GFX_BOTTOM, nullptr);

    //values to check if Speaker is on, if audio is playing, and if audio buffer points to something
    bool speakerOn = false;
    bool audioPlaying = false;
    bool recBuf = true;
    
    //values to check if the mic got initialized, if the mic is recording, if the mic buffer points to something, and if audio is playing
    bool initMic = false;
    bool micRecording = false;
    bool micBuff = true;
    
    //Create speaker buffer
    u32 rec_buf_size = 0x30000; //buffer size for recorded audio
    u8* rec_buf = static_cast<u8*>(linearAlloc(rec_buf_size)); //points to audio buffer and allocates it so speaker can play it


    //Check if recording buffer points to null
    if(rec_buf == nullptr)
    {
        recBuf = false;
        printf("Failed to create audio buffer! Speaker will not function!");
    }

    //First attempts to enable speakers then configures waveBuff settings to match microphone sample rate and other things
    if(R_SUCCEEDED(ndspInit()))
    {
        speakerOn = true;
        ndspChnReset(0); //Reset channel with id 0
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR); //Sets audio interpolation with channel id 0
        ndspChnSetRate(0, 16360); //set sample rate for channe id 0 (same as mic smaple rate so audio doesn't sound weird)
        ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16); //mono audio with same encoding method as microphone for id 0
        waveBuf.data_vaddr = rec_buf; //address of where the waveBuf gets it's data
        waveBuf.looping = false; //Disable audio looping
        printf("Speakers initialized!\n");
    }
    else printf("Failed to initialize speakers!\n");
    
    
    
    //Create a microphone buffer
    u32 mic_buf_size = 0x30000; //max size of recording in bytes (last few bytes reserved for bookkeeping)
    u8* mic_buf = static_cast<u8*>(memalign(0x1000, mic_buf_size)); //points to where the sound system writes to for recording audio
    u32 buf_pos = 0; //position in the microphone buffer


    //Check if microphone buffer points to null
    if(mic_buf == nullptr)
    {
        micBuff = false;
        printf("Failed to create microphone buffer! Mic will record to nothing!"); //Check if buffer points to null
    } 

    //Initilize the microphone
    if(R_SUCCEEDED(micInit(mic_buf, mic_buf_size)) && micBuff) //initilizes mic to write to mic_buf and stop when it reaches the size of the buffer
    {
        initMic = true;
        printf("Microphone initialized!\n");
    }
    else printf("Failed to initialize microphone! \n");

    
    
    while(aptMainLoop())
    {
        //Scan for inputs
        hidScanInput();
        
        //Exit while loop if start key is pressed
        u32 kDown = hidKeysDown();
        if(kDown & KEY_START) break;

        if(micRecording) //Check if recording limit gets exceeded
        {
            buf_pos = micGetLastSampleOffset(); //buffer number position equals the last part of buffer written to during recording

            if(buf_pos >= mic_buf_size -10) //if the buffer position passes the buffer size that means the recording limit is reacched
                {
                    MICU_StopSampling();
                    micRecording = false; //forcibly disable mic recording
                    printf("Recording limit exceeded!\n Press X to play recording\n");
                }
        } 

        //Attempt to start recording if A is pressed and not currently recording
        if((kDown & KEY_A) && !micRecording && !audioPlaying)
        {
            if(R_SUCCEEDED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, mic_buf_size -4, false))) //Attempt to start rec audio
            {
                micRecording = true;
                printf("Recording started!\n");
                
            }
            else printf("Failed to start recording!\n");
        }

        //Attempt to stop recording if B is pressed and currently recording
        if((kDown & KEY_B) && micRecording && !audioPlaying)
        {
           if(R_SUCCEEDED(MICU_StopSampling())) //attemps to stop rec audio
           {
            micRecording = false;
            printf("Recording stopped!\n Buffer Size: %d\n Last Sample: %d\n Press X to play recording\n", static_cast<int>(mic_buf_size-4), static_cast<int>(buf_pos));
           }
           else printf("Failed to stop recording!\n"); 
        }

        if(waveBuf.status == NDSP_WBUF_DONE) //Sets audio playing to false if speaker isn't playing anything
        {
            audioPlaying = false;
        }

        if((kDown & KEY_X) && speakerOn && !micRecording) //Plays recorded audio if X is pressed
        {
           if(buf_pos > 1)
           {
            consoleClear();
            printf("Playing audio...\n");
            audioPlaying = true;
            playAudio(rec_buf, mic_buf, buf_pos);
           } 
        }
        
    }
        if(initMic) micExit(); //Exit the microphone if it was initialized
        if(mic_buf != nullptr) free(mic_buf); //Free microphone buffer if it isn't null
        if(speakerOn) ndspExit();
        if(recBuf) linearFree(rec_buf);
        gfxExit(); //exit graphics
        return 0;
    





}



